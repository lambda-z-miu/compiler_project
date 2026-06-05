#include "Ast.h"
#include "Parser.h"
#include "SvgRenderer.h"
#include "Translation.h"
#include "tokenizer.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

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
		std::cout << "SVG:\n" << renderSvg(ir) << "\n";

		std::ofstream svgFile("output.svg");
		if (!svgFile) {
			std::cerr << "Failed to open output.svg for writing\n";
		} else {
			svgFile << renderSvg(ir) << "\n";
			svgFile.close();
		}

	} catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << "\n";
		return 1;
	}

	return 0;
}
