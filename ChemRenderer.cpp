#include "ChemRenderer.h"

#include "Parser.h"
#include "SvgRenderer.h"
#include "Translation.h"
#include "tokenizer.h"

#include <stdexcept>
#include <string>

std::string renderChemSvg(const std::string& source) {
	Tokenizer tokenizer(source);
	auto tokens = tokenizer.tokenize();
	Parser parser(std::move(tokens));
	Cpd ast = parser.parseCpd();
	Fragment ir = translateCpd(ast);
	return renderSvg(ir);
}

std::string jsonEscape(const std::string& value) {
	std::string out;
	out.reserve(value.size() + 16);
	for (unsigned char c : value) {
		switch (c) {
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\b': out += "\\b"; break;
			case '\f': out += "\\f"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			default:
				if (c < 0x20) {
					const char* hex = "0123456789abcdef";
					out += "\\u00";
					out += hex[(c >> 4) & 0x0F];
					out += hex[c & 0x0F];
				} else {
					out += static_cast<char>(c);
				}
		}
	}
	return out;
}

namespace {

std::string decodeJsonStringAt(const std::string& json, size_t pos) {
	if (pos >= json.size() || json[pos] != '"') {
		throw std::runtime_error("Expected JSON string");
	}

	std::string out;
	for (++pos; pos < json.size(); ++pos) {
		char c = json[pos];
		if (c == '"') {
			return out;
		}
		if (c != '\\') {
			out += c;
			continue;
		}
		if (++pos >= json.size()) {
			throw std::runtime_error("Invalid JSON escape");
		}
		char esc = json[pos];
		switch (esc) {
			case '"': out += '"'; break;
			case '\\': out += '\\'; break;
			case '/': out += '/'; break;
			case 'b': out += '\b'; break;
			case 'f': out += '\f'; break;
			case 'n': out += '\n'; break;
			case 'r': out += '\r'; break;
			case 't': out += '\t'; break;
			default:
				throw std::runtime_error("Unsupported JSON escape");
		}
	}
	throw std::runtime_error("Unterminated JSON string");
}

} // namespace

std::string extractJsonStringField(const std::string& json, const std::string& fieldName) {
	const std::string key = "\"" + fieldName + "\"";
	size_t keyPos = json.find(key);
	if (keyPos == std::string::npos) {
		throw std::runtime_error("JSON message is missing " + fieldName);
	}

	size_t colon = json.find(':', keyPos + key.size());
	if (colon == std::string::npos) {
		throw std::runtime_error("Invalid JSON message");
	}

	size_t valuePos = json.find_first_not_of(" \t\r\n", colon + 1);
	if (valuePos == std::string::npos) {
		throw std::runtime_error("Invalid JSON message");
	}
	return decodeJsonStringAt(json, valuePos);
}
