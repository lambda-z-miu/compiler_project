#pragma once

#include "Ast.h"
#include "tokenizer.h"

#include <cstddef>
#include <vector>

class Parser {
public:
	explicit Parser(std::vector<Token> tokens);

	Cpd parseCpd();

private:
	std::vector<Token> tokens_;
	size_t pos_ = 0;

	const Token& peek(size_t offset = 0) const;
	bool check(TokenType type) const;
	const Token& advance();
	const Token& expect(TokenType type, const char* message);
	bool isStartSub() const;
	bool isStartInterface() const;
	bool isStartCpo() const;
	bool isEndOfCpo() const;

	Cpo parseCpo();
	Interface parseInterface();
	Interface defaultInterface() const;
	Poses parsePoses();
	Subs parseSubs();
	Subs parseSubsPrime(Poses poses);
	Sub parseSub();
	Core parseCore();
};
