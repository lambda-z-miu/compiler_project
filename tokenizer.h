#pragma once

#include <cstddef>
#include <string>
#include <vector>

enum class TokenType {
	Sep,        // ,
	Semi,       // ;
	LParen,     // (
	RParen,     // )
	LStr,       // [
	RStr,       // ]
	Keyword,    // keywords list
	Identifier, // [A-Z][A-Z]*
	Digit,      // [0-9][0-9]*
	Haf,        // -
	End
};

struct Token {
	TokenType type;
	std::string lexeme;
	size_t pos;
};

class Tokenizer {
public:
	explicit Tokenizer(std::string input);

	std::vector<Token> tokenize();

private:
	std::string input_;
	size_t pos_ = 0;

	bool isAtEnd() const;
	char peek() const;
	void advance();
	void advanceN(size_t n);
	void skipWhitespace();
	Token make(TokenType type, const std::string& lexeme) const;
	std::string matchKeyword() const;
};

const char* tokenTypeName(TokenType type);
