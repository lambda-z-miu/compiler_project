#include "ChemRenderer.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::string extractSource(const std::string& json) {
	return extractJsonStringField(json, "source");
}

bool readMessage(std::string& message) {
	uint32_t length = 0;
	std::cin.read(reinterpret_cast<char*>(&length), sizeof(length));
	if (!std::cin) {
		return false;
	}
	if (length > 16 * 1024 * 1024) {
		throw std::runtime_error("Native message is too large");
	}

	message.resize(length);
	std::cin.read(message.data(), length);
	if (!std::cin) {
		throw std::runtime_error("Failed to read native message");
	}
	return true;
}

void writeMessage(const std::string& message) {
	uint32_t length = static_cast<uint32_t>(message.size());
	std::cout.write(reinterpret_cast<const char*>(&length), sizeof(length));
	std::cout.write(message.data(), message.size());
	std::cout.flush();
}

std::string okResponse(const std::string& svg) {
	return "{\"ok\":true,\"svg\":\"" + jsonEscape(svg) + "\"}";
}

std::string errorResponse(const std::string& error) {
	return "{\"ok\":false,\"error\":\"" + jsonEscape(error) + "\"}";
}

} // namespace

int main() {
	try {
		std::string message;
		while (readMessage(message)) {
			try {
				std::string source = extractSource(message);
				writeMessage(okResponse(renderChemSvg(source)));
			} catch (const std::exception& ex) {
				writeMessage(errorResponse(ex.what()));
			}
		}
	} catch (const std::exception& ex) {
		writeMessage(errorResponse(ex.what()));
		return 1;
	}
	return 0;
}
