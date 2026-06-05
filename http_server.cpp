#include "ChemRenderer.h"

#include <arpa/inet.h>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr int kDefaultPort = 17654;
constexpr size_t kMaxBodySize = 1024 * 1024;

bool g_running = true;

void onSignal(int) {
	g_running = false;
}

std::string okResponse(const std::string& svg) {
	return "{\"ok\":true,\"svg\":\"" + jsonEscape(svg) + "\"}";
}

std::string errorResponse(const std::string& error) {
	return "{\"ok\":false,\"error\":\"" + jsonEscape(error) + "\"}";
}

std::string oneLineForLog(const std::string& value) {
	std::string out;
	out.reserve(value.size());
	for (char c : value) {
		if (c == '\r' || c == '\n' || c == '\t') {
			out.push_back(' ');
		} else {
			out.push_back(c);
		}
	}
	return out;
}

std::string httpStatusText(int status) {
	switch (status) {
		case 200: return "OK";
		case 204: return "No Content";
		case 400: return "Bad Request";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		default: return "OK";
	}
}

std::string makeHttpResponse(int status, const std::string& body, const std::string& contentType = "application/json") {
	std::ostringstream out;
	out << "HTTP/1.1 " << status << " " << httpStatusText(status) << "\r\n"
		<< "Content-Type: " << contentType << "; charset=utf-8\r\n"
		<< "Content-Length: " << body.size() << "\r\n"
		<< "Access-Control-Allow-Origin: *\r\n"
		<< "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
		<< "Access-Control-Allow-Headers: Content-Type\r\n"
		<< "Connection: close\r\n"
		<< "\r\n"
		<< body;
	return out.str();
}

std::string makeNoContentResponse() {
	return "HTTP/1.1 204 No Content\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
		"Access-Control-Allow-Headers: Content-Type\r\n"
		"Connection: close\r\n"
		"\r\n";
}

size_t parseContentLength(const std::string& headers) {
	std::string lower = headers;
	for (char& c : lower) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}

	const std::string key = "content-length:";
	size_t pos = lower.find(key);
	if (pos == std::string::npos) {
		return 0;
	}
	pos += key.size();
	while (pos < lower.size() && (lower[pos] == ' ' || lower[pos] == '\t')) {
		++pos;
	}
	size_t end = pos;
	while (end < lower.size() && std::isdigit(static_cast<unsigned char>(lower[end]))) {
		++end;
	}
	if (end == pos) {
		throw std::runtime_error("Invalid Content-Length");
	}
	return static_cast<size_t>(std::stoul(lower.substr(pos, end - pos)));
}

std::string readHttpRequest(int clientFd) {
	std::string data;
	char buffer[4096];
	size_t headerEnd = std::string::npos;

	while (headerEnd == std::string::npos) {
		ssize_t n = recv(clientFd, buffer, sizeof(buffer), 0);
		if (n <= 0) {
			throw std::runtime_error("Failed to read HTTP request");
		}
		data.append(buffer, static_cast<size_t>(n));
		if (data.size() > kMaxBodySize + 8192) {
			throw std::runtime_error("HTTP request is too large");
		}
		headerEnd = data.find("\r\n\r\n");
	}

	size_t contentLength = parseContentLength(data.substr(0, headerEnd));
	if (contentLength > kMaxBodySize) {
		throw std::runtime_error("Request body is too large");
	}
	size_t totalNeeded = headerEnd + 4 + contentLength;
	while (data.size() < totalNeeded) {
		ssize_t n = recv(clientFd, buffer, sizeof(buffer), 0);
		if (n <= 0) {
			throw std::runtime_error("Failed to read HTTP body");
		}
		data.append(buffer, static_cast<size_t>(n));
	}
	return data.substr(0, totalNeeded);
}

void sendAll(int clientFd, const std::string& response) {
	const char* data = response.data();
	size_t left = response.size();
	while (left > 0) {
		ssize_t n = send(clientFd, data, left, 0);
		if (n <= 0) {
			return;
		}
		data += n;
		left -= static_cast<size_t>(n);
	}
}

std::string firstLine(const std::string& request) {
	size_t end = request.find("\r\n");
	if (end == std::string::npos) {
		throw std::runtime_error("Invalid HTTP request");
	}
	return request.substr(0, end);
}

std::string requestBody(const std::string& request) {
	size_t pos = request.find("\r\n\r\n");
	if (pos == std::string::npos) {
		return "";
	}
	return request.substr(pos + 4);
}

std::string handleRequest(const std::string& request) {
	std::istringstream line(firstLine(request));
	std::string method;
	std::string path;
	std::string version;
	line >> method >> path >> version;
	if (method.empty() || path.empty()) {
		return makeHttpResponse(400, errorResponse("Invalid HTTP request"));
	}

	if (method == "OPTIONS") {
		return makeNoContentResponse();
	}
	if (method == "GET" && path == "/health") {
		return makeHttpResponse(200, "{\"ok\":true,\"service\":\"my-chem\"}");
	}
	if (path != "/render") {
		return makeHttpResponse(404, errorResponse("Not found"));
	}
	if (method != "POST") {
		return makeHttpResponse(405, errorResponse("Method not allowed"));
	}

	try {
		std::string source = extractJsonStringField(requestBody(request), "source");
		std::cerr << "sketch " << oneLineForLog(source) << "\n";
		return makeHttpResponse(200, okResponse(renderChemSvg(source)));
	} catch (const std::exception& ex) {
		return makeHttpResponse(400, errorResponse(ex.what()));
	}
}

int makeServerSocket(int port) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw std::runtime_error("socket() failed");
	}

	int one = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(static_cast<uint16_t>(port));
	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
		close(fd);
		throw std::runtime_error("inet_pton() failed");
	}

	if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
		std::string msg = "bind(127.0.0.1:" + std::to_string(port) + ") failed: " + std::strerror(errno);
		close(fd);
		throw std::runtime_error(msg);
	}
	if (listen(fd, 16) != 0) {
		close(fd);
		throw std::runtime_error("listen() failed");
	}
	return fd;
}

int parsePort(int argc, char** argv) {
	if (argc < 2) {
		return kDefaultPort;
	}
	int port = std::atoi(argv[1]);
	if (port <= 0 || port > 65535) {
		throw std::runtime_error("Invalid port");
	}
	return port;
}

} // namespace

int main(int argc, char** argv) {
	try {
		std::signal(SIGINT, onSignal);
		std::signal(SIGTERM, onSignal);

		int port = parsePort(argc, argv);
		int serverFd = makeServerSocket(port);
		std::cerr << "my-Chem HTTP service listening on http://127.0.0.1:" << port << "\n";

		while (g_running) {
			sockaddr_in clientAddr{};
			socklen_t clientLen = sizeof(clientAddr);
			int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
			if (clientFd < 0) {
				if (errno == EINTR) {
					continue;
				}
				break;
			}

			try {
				sendAll(clientFd, handleRequest(readHttpRequest(clientFd)));
			} catch (const std::exception& ex) {
				sendAll(clientFd, makeHttpResponse(500, errorResponse(ex.what())));
			}
			close(clientFd);
		}
		close(serverFd);
	} catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << "\n";
		return 1;
	}
	return 0;
}
