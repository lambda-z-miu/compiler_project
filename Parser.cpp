#include "Parser.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

Cpd Parser::parseCpd() {
	Cpd cpd;
	while (isStartCpo()) {
		cpd.items.push_back(parseCpo());
		expect(TokenType::Semi, "Expected ';' after CPO");
	}
	expect(TokenType::End, "Expected end of input");
	return cpd;
}

const Token& Parser::peek(size_t offset) const {
	size_t idx = pos_ + offset;
	if (idx >= tokens_.size()) {
		return tokens_.back();
	}
	return tokens_[idx];
}

bool Parser::check(TokenType type) const {
	return peek().type == type;
}

const Token& Parser::advance() {
	if (pos_ < tokens_.size()) {
		++pos_;
	}
	return tokens_[pos_ - 1];
}

const Token& Parser::expect(TokenType type, const char* message) {
	if (!check(type)) {
		throw std::runtime_error(std::string(message) + " at position " + std::to_string(peek().pos));
	}
	return advance();
}

bool Parser::isStartSub() const {
	return check(TokenType::Identifier) || check(TokenType::Keyword);
}

bool Parser::isStartInterface() const {
	return check(TokenType::LParen) || check(TokenType::LBrace);
}

bool Parser::isStartCpo() const {
	return isStartInterface() || check(TokenType::Keyword);
}

bool Parser::isEndOfCpo() const {
	return check(TokenType::Semi) || check(TokenType::RStr) || check(TokenType::End);
}

Cpo Parser::parseCpo() {
	Cpo cpo;
	cpo.left = isStartInterface() ? parseInterface() : defaultInterface();
	cpo.core = parseCore();

	if (check(TokenType::Sep)) {
		advance();
		if (!isStartInterface() && !isEndOfCpo()) {
			cpo.subs = parseSubs();
		}
	}

	cpo.right = isStartInterface() ? parseInterface() : defaultInterface();
	return cpo;
}

Interface Parser::defaultInterface() const {
	Interface iface;
	iface.kind = InterfaceKind::Connect;
	iface.poses.values.push_back(0);
	return iface;
}

Interface Parser::parseInterface() {
	Interface iface;
	TokenType close = TokenType::RParen;
	if (check(TokenType::LParen)) {
		iface.kind = InterfaceKind::Connect;
		advance();
		close = TokenType::RParen;
	} else if (check(TokenType::LBrace)) {
		iface.kind = InterfaceKind::Fuse;
		advance();
		close = TokenType::RBrace;
	} else {
		throw std::runtime_error("Expected '(' or '{' at position " + std::to_string(peek().pos));
	}
	iface.poses = parsePoses();
	expect(close, close == TokenType::RParen ? "Expected ')' after poses" : "Expected '}' after poses");
	return iface;
}

Poses Parser::parsePoses() {
	Poses poses;
	const Token& first = expect(TokenType::Digit, "Expected digit in POSES");
	poses.values.push_back(std::stoi(first.lexeme));
	while (check(TokenType::Sep)) {
		advance();
		const Token& tok = expect(TokenType::Digit, "Expected digit after ',' in POSES");
		poses.values.push_back(std::stoi(tok.lexeme));
	}
	return poses;
}

Subs Parser::parseSubs() {
	if (isStartSub()) {
		Subs subs;
		subs.kind = Subs::Kind::BareSub;
		subs.bareSub = parseSub();
		return subs;
	}
	if (check(TokenType::Digit)) {
		Poses poses = parsePoses();
		expect(TokenType::Haf, "Expected '-' after POSES");
		return parseSubsPrime(std::move(poses));
	}
	if (check(TokenType::Caret)) {
		Poses poses;
		poses.values.push_back(1);
		return parseSubsPrime(std::move(poses));
	}
	Subs subs;
	subs.kind = Subs::Kind::Empty;
	return subs;
}

Subs Parser::parseSubsPrime(Poses poses) {
	Subs subs;
	subs.kind = Subs::Kind::Entry;
	subs.poses = std::move(poses);
	if (check(TokenType::Caret)) {
		subs.replace = true;
		advance();
	}
	if (check(TokenType::LStr)) {
		advance();
		subs.isGroup = true;
		subs.group = std::make_unique<Cpo>(parseCpo());
		expect(TokenType::RStr, "Expected ']' after CPO");
		if (check(TokenType::Sep)) {
			advance();
		} else if (isStartInterface() || isEndOfCpo()) {
			return subs;
		} else {
			throw std::runtime_error("Expected ',' after group SUBS' at position " + std::to_string(peek().pos));
		}
	} else if (isStartSub()) {
		subs.isGroup = false;
		subs.sub = parseSub();
		if (check(TokenType::Sep)) {
			advance();
		} else if (isStartInterface() || isEndOfCpo()) {
			return subs;
		} else {
			throw std::runtime_error("Expected ',' after SUB in SUBS' at position " + std::to_string(peek().pos));
		}
	} else {
		throw std::runtime_error("Expected SUB, ^SUB, [CPO], or ^[CPO] after POSES- at position " + std::to_string(peek().pos));
	}

	Subs nextSubs = parseSubs();
	if (nextSubs.kind != Subs::Kind::Empty) {
		subs.next = std::make_unique<Subs>(std::move(nextSubs));
	}
	return subs;
}

Sub Parser::parseSub() {
	Sub sub;
	if (check(TokenType::Identifier)) {
		sub.kind = Sub::Kind::Identifier;
		sub.identifier = advance().lexeme;
		return sub;
	}
	if (check(TokenType::Keyword)) {
		sub.kind = Sub::Kind::Core;
		sub.core = parseCore();
		return sub;
	}
	throw std::runtime_error("Expected SUB at position " + std::to_string(peek().pos));
}

Core Parser::parseCore() {
	Core core;
	const Token& first = expect(TokenType::Keyword, "Expected KEYWORDS");
	core.keywords.push_back(first.lexeme);
	while (check(TokenType::Keyword)) {
		core.keywords.push_back(advance().lexeme);
	}
	return core;
}
