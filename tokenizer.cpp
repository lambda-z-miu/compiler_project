#include "tokenizer.h"

#include <cctype>
#include <stdexcept>

Tokenizer::Tokenizer(std::string input) : input_(std::move(input)) {}

std::vector<Token> Tokenizer::tokenize() {
	std::vector<Token> out;
	while (true) {
		skipWhitespace();
		if (isAtEnd()) {
			out.push_back(Token{TokenType::End, "", pos_});
			break;
		}

		char c = peek();
		if (c == ',') { out.push_back(make(TokenType::Sep, ",")); advance(); continue; }
		if (c == ';') { out.push_back(make(TokenType::Semi, ";")); advance(); continue; }
		if (c == '(') { out.push_back(make(TokenType::LParen, "(")); advance(); continue; }
		if (c == ')') { out.push_back(make(TokenType::RParen, ")")); advance(); continue; }
		if (c == '[') { out.push_back(make(TokenType::LStr, "[")); advance(); continue; }
		if (c == ']') { out.push_back(make(TokenType::RStr, "]")); advance(); continue; }
		if (c == '-') { out.push_back(make(TokenType::Haf, "-")); advance(); continue; }

		if (auto kw = matchKeyword(); !kw.empty()) {
			out.push_back(make(TokenType::Keyword, kw));
			advanceN(kw.size());
			continue;
		}

		if (std::isdigit(static_cast<unsigned char>(c))) {
			size_t start = pos_;
			advance();
			while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
				advance();
			}
			out.push_back(Token{TokenType::Digit, input_.substr(start, pos_ - start), start});
			continue;
		}

		if (std::isupper(static_cast<unsigned char>(c))) {
			size_t start = pos_;
			advance();
			while (!isAtEnd() && std::isupper(static_cast<unsigned char>(peek()))) {
				advance();
			}
			out.push_back(Token{TokenType::Identifier, input_.substr(start, pos_ - start), start});
			continue;
		}

		throw std::runtime_error("Unexpected character " + std::string(1, c) + " at position " + std::to_string(pos_));
	}
	return out;
}

bool Tokenizer::isAtEnd() const {
	return pos_ >= input_.size();
}

char Tokenizer::peek() const {
	return isAtEnd() ? '\0' : input_[pos_];
}

void Tokenizer::advance() {
	if (!isAtEnd()) {
		++pos_;
	}
}

void Tokenizer::advanceN(size_t n) {
	pos_ += n;
}

void Tokenizer::skipWhitespace() {
	while (!isAtEnd()) {
		char c = peek();
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			advance();
			continue;
		}
		break;
	}
}

Token Tokenizer::make(TokenType type, const std::string& lexeme) const {
	return Token{type, lexeme, pos_};
}

std::string Tokenizer::matchKeyword() const {
	// longest match first
	static const std::string keywords[] = {
		"OMe", "OPh", "Ind", "Pyr", "NH2", "=O", "Ph", "Me", "Ac", "Cl", "Br", "OH", "SH", "Im", "=", "#", "C", "P", "N", "S", "F", "I"
	};

	// digitR / digitL (digit can be multiple characters)
	if (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
		size_t i = pos_;
		while (i < input_.size() && std::isdigit(static_cast<unsigned char>(input_[i]))) {
			++i;
		}
		if (i < input_.size()) {
			char next = input_[i];
			if (next == 'R' || next == 'L') {
				return input_.substr(pos_, i - pos_ + 1);
			}
		}
	}

	for (const auto& kw : keywords) {
		if (input_.compare(pos_, kw.size(), kw) == 0) {
			return kw;
		}
	}
	return "";
}

const char* tokenTypeName(TokenType type) {
	switch (type) {
		case TokenType::Sep: return "Sep";
		case TokenType::Semi: return "Semi";
		case TokenType::LParen: return "LParen";
		case TokenType::RParen: return "RParen";
		case TokenType::LStr: return "LStr";
		case TokenType::RStr: return "RStr";
		case TokenType::Keyword: return "Keyword";
		case TokenType::Identifier: return "Identifier";
		case TokenType::Digit: return "Digit";
		case TokenType::Haf: return "Haf";
		case TokenType::End: return "End";
	}
	return "Unknown";
}
