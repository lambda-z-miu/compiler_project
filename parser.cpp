#include "Ast.h"
#include "Translation.h"
#include "tokenizer.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Parser {
public:
	explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

	Cpd parseCpd() {
		Cpd cpd;
		while (check(TokenType::LParen)) {
			cpd.items.push_back(parseCpo());
			expect(TokenType::Semi, "Expected ';' after CPO");
		}
		expect(TokenType::End, "Expected end of input");
		return cpd;
	}

private:
	std::vector<Token> tokens_;
	size_t pos_ = 0;

	const Token& peek(size_t offset = 0) const {
		size_t idx = pos_ + offset;
		if (idx >= tokens_.size()) {
			return tokens_.back();
		}
		return tokens_[idx];
	}

	bool check(TokenType type) const {
		return peek().type == type;
	}

	const Token& advance() {
		if (pos_ < tokens_.size()) {
			++pos_;
		}
		return tokens_[pos_ - 1];
	}

	const Token& expect(TokenType type, const char* message) {
		if (!check(type)) {
			throw std::runtime_error(std::string(message) + " at position " + std::to_string(peek().pos));
		}
		return advance();
	}

	bool isStartSub() const {
		return check(TokenType::Identifier) || check(TokenType::Keyword);
	}

	Cpo parseCpo() {
		Cpo cpo;
		cpo.left = parseInterface();
		cpo.core = parseCore();
		expect(TokenType::Sep, "Expected ',' after CORE");
		cpo.subs = parseSubs();
		cpo.right = parseInterface();
		return cpo;
	}

	Interface parseInterface() {
		expect(TokenType::LParen, "Expected '('");
		Interface iface;
		iface.poses = parsePoses();
		expect(TokenType::RParen, "Expected ')' after poses");
		return iface;
	}

	Poses parsePoses() {
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

	Subs parseSubs() {
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
		Subs subs;
		subs.kind = Subs::Kind::Empty;
		return subs;
	}

	Subs parseSubsPrime(Poses poses) {
		Subs subs;
		subs.kind = Subs::Kind::Entry;
		subs.poses = std::move(poses);
		if (check(TokenType::LStr)) {
			advance();
			subs.isGroup = true;
			subs.group = std::make_unique<Cpo>(parseCpo());
			expect(TokenType::RStr, "Expected ']' after CPO");
			if (check(TokenType::Sep)) {
				advance();
			} else {
				return subs;
			}
		} else if (isStartSub()) {
			subs.isGroup = false;
			subs.sub = parseSub();
			expect(TokenType::Sep, "Expected ',' after SUB in SUBS'");
		} else {
			throw std::runtime_error("Expected SUB or [CPO] after POSES- at position " + std::to_string(peek().pos));
		}

		Subs nextSubs = parseSubs();
		if (nextSubs.kind != Subs::Kind::Empty) {
			subs.next = std::make_unique<Subs>(std::move(nextSubs));
		}
		return subs;
	}

	Sub parseSub() {
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

	Core parseCore() {
		Core core;
		const Token& first = expect(TokenType::Keyword, "Expected KEYWORDS");
		core.keywords.push_back(first.lexeme);
		while (check(TokenType::Keyword)) {
			core.keywords.push_back(advance().lexeme);
		}
		return core;
	}
};

static void printIndent(int n) {
	for (int i = 0; i < n; ++i) {
		std::cout << ' ';
	}
}

static void printPoses(const Poses& poses, int indent) {
	printIndent(indent);
	std::cout << "Poses:";
	for (size_t i = 0; i < poses.values.size(); ++i) {
		std::cout << (i == 0 ? " " : ", ") << poses.values[i];
	}
	std::cout << "\n";
}

static void printCore(const Core& core, int indent) {
	printIndent(indent);
	std::cout << "Core:";
	for (size_t i = 0; i < core.keywords.size(); ++i) {
		std::cout << (i == 0 ? " " : " ") << core.keywords[i];
	}
	std::cout << "\n";
}

static void printSub(const Sub& sub, int indent) {
	if (sub.kind == Sub::Kind::Identifier) {
		printIndent(indent);
		std::cout << "Sub Identifier: " << sub.identifier << "\n";
		return;
	}
	printIndent(indent);
	std::cout << "Sub Core:\n";
	printCore(sub.core, indent + 2);
}

static void printSubs(const Subs& subs, int indent) {
	if (subs.kind == Subs::Kind::Empty) {
		printIndent(indent);
		std::cout << "Subs: empty\n";
		return;
	}
	if (subs.kind == Subs::Kind::BareSub) {
		printIndent(indent);
		std::cout << "Subs: bare\n";
		printSub(subs.bareSub, indent + 2);
		return;
	}

	printIndent(indent);
	std::cout << "Subs: entry\n";
	printPoses(subs.poses, indent + 2);
	if (subs.isGroup) {
		printIndent(indent + 2);
		std::cout << "Group:\n";
		const Cpo& cpo = *subs.group;
		printIndent(indent + 4);
		std::cout << "Interface L:\n";
		printPoses(cpo.left.poses, indent + 6);
		printCore(cpo.core, indent + 4);
		printSubs(cpo.subs, indent + 4);
		printIndent(indent + 4);
		std::cout << "Interface R:\n";
		printPoses(cpo.right.poses, indent + 6);
	} else {
		printSub(subs.sub, indent + 2);
	}

	if (subs.next) {
		printIndent(indent + 2);
		std::cout << "Next:\n";
		printSubs(*subs.next, indent + 4);
	}
}

static void printCpd(const Cpd& cpd) {
	std::cout << "AST:\n";
	for (size_t i = 0; i < cpd.items.size(); ++i) {
		std::cout << "CPO #" << (i + 1) << ":\n";
		const Cpo& cpo = cpd.items[i];
		printIndent(2);
		std::cout << "Interface L:\n";
		printPoses(cpo.left.poses, 4);
		printCore(cpo.core, 2);
		printSubs(cpo.subs, 2);
		printIndent(2);
		std::cout << "Interface R:\n";
		printPoses(cpo.right.poses, 4);
	}
}

int main() {
	std::string input;
	std::string line;
	while (std::getline(std::cin, line)) {
		if (!input.empty()) {
			input += '\n';
		}
		input += line;
	}
	if (input.empty()) {
		return 0;
	}

	try {
		Tokenizer tokenizer(input);
		auto tokens = tokenizer.tokenize();
		std::cout << "Tokens:\n";
		for (const auto& tok : tokens) {
			std::cout << tokenTypeName(tok.type) << "\t" << tok.lexeme << "\t" << tok.pos << "\n";
		}

		Parser parser(std::move(tokens));
		Cpd ast = parser.parseCpd();
		printCpd(ast);
		Fragment ir = translateCpd(ast);
		printIr(ir, std::cout);
	} catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << "\n";
		return 1;
	}

	return 0;
}
