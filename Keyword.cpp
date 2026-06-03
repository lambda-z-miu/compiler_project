#include "Keyword.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace {

Fragment makeSingleAtom(const std::string& atom) {
	Fragment frag;
	frag.ir.push_back(makeNode(atom, 1, {}));
	frag.entry = 1;
	frag.exit = 1;
	return frag;
}

Fragment makePh() {
	Fragment frag;
	frag.ir = {
		makeNode("C", 1, {2 ,2, 6}),
		makeNode("C", 2, {3, 1 ,1}),
		makeNode("C", 3, {4, 4, 2}),
		makeNode("C", 4, {5, 3, 3}),
		makeNode("C", 5, {6, 6, 4}),
		makeNode("C", 6, {1, 5,}),
	};
	frag.entry = 1;
	frag.exit = 1;
	for (int i = 1; i <= 6; ++i) {
		frag.defaultNext[i] = i == 6 ? 1 : i + 1;
	}
	return frag;
}

Fragment makeAc() {
	Fragment frag;
	frag.ir = {
		makeNode("C", 1, {2, 3, 3}),
		makeNode("C", 2, {1}),
		makeNode("O", 3, {1, 1}),
	};
	frag.entry = 1;
	frag.exit = 1;
	return frag;
}

Fragment makeOMe() {
	Fragment frag;
	frag.ir = {
		makeNode("O", 1, {2}),
		makeNode("C", 2, {1}),
	};
	frag.entry = 1;
	frag.exit = 2;
	frag.defaultNext[1] = 2;
	return frag;
}

Fragment makeOPh() {
	Fragment frag;
	frag.ir = {
		makeNode("O", 1, {2}),
		makeNode("C", 2, {3, 7, 1}),
		makeNode("C", 3, {4, 2}),
		makeNode("C", 4, {5, 3}),
		makeNode("C", 5, {6, 4}),
		makeNode("C", 6, {7, 5}),
		makeNode("C", 7, {2, 6}),
	};
	frag.entry = 1;
	frag.exit = 2;
	frag.defaultNext[1] = 2;
	for (int i = 2; i <= 7; ++i) {
		frag.defaultNext[i] = i == 7 ? 2 : i + 1;
	}
	return frag;
}

Fragment makeRing(int n) {
	if (n < 3) {
		throw std::runtime_error("Ring size must be at least 3");
	}

	Fragment frag;
	for (int i = 1; i <= n; ++i) {
		int next = i == n ? 1 : i + 1;
		int prev = i == 1 ? n : i - 1;
		frag.ir.push_back(makeNode("C", i, {next, prev}));
		frag.defaultNext[i] = next;
	}
	frag.entry = 1;
	frag.exit = 1;
	return frag;
}

Fragment makeLine(int n) {
	if (n < 1) {
		throw std::runtime_error("Line size must be at least 1");
	}

	Fragment frag;
	if (n == 1) {
		frag.ir.push_back(makeNode("C", 1, {}));
	} else {
		frag.ir.push_back(makeNode("C", 1, {2}));
		for (int i = 2; i < n; ++i) {
			frag.ir.push_back(makeNode("C", i, {i - 1, i + 1}));
			frag.defaultNext[i] = i + 1;
		}
		frag.ir.push_back(makeNode("C", n, {n - 1}));
		frag.defaultNext[1] = 2;
	}
	frag.entry = 1;
	frag.exit = n;
	return frag;
}

bool parseNumberedKeyword(const std::string& lexeme, int& n, char& suffix) {
	if (lexeme.size() < 2) {
		return false;
	}
	suffix = lexeme.back();
	if (suffix != 'R' && suffix != 'L') {
		return false;
	}

	std::string digits = lexeme.substr(0, lexeme.size() - 1);
	if (digits.empty() || !std::all_of(digits.begin(), digits.end(), [](unsigned char c) {
			return std::isdigit(c);
		})) {
		return false;
	}
	n = std::stoi(digits);
	return true;
}

} // namespace

KeywordInfo translateKeyword(const std::string& lexeme) {
	KeywordInfo kw;

	if (lexeme == "=" || lexeme == "#") {
		kw.kind = KeywordInfo::Kind::Bond;
		kw.bond = lexeme == "=" ? 2 : 3;
		kw.frag.entryBond = kw.bond;
		return kw;
	}

	if (lexeme == "Ph") {
		kw.frag = makePh();
		return kw;
	}
	if (lexeme == "Me") {
		kw.frag = makeSingleAtom("C");
		return kw;
	}
	if (lexeme == "Ac") {
		kw.frag = makeAc();
		return kw;
	}
	if (lexeme == "=O") {
		kw.frag = makeSingleAtom("O");
		kw.frag.entryBond = 2;
		return kw;
	}
	if (lexeme == "OMe") {
		kw.frag = makeOMe();
		return kw;
	}
	if (lexeme == "OPh") {
		kw.frag = makeOPh();
		return kw;
	}
	if (lexeme == "C" || lexeme == "P" || lexeme == "N" || lexeme == "F" || lexeme == "Cl" || lexeme == "Br" || lexeme == "I") {
		kw.frag = makeSingleAtom(lexeme);
		return kw;
	}

	int n = 0;
	char suffix = '\0';
	if (parseNumberedKeyword(lexeme, n, suffix)) {
		kw.frag = suffix == 'R' ? makeRing(n) : makeLine(n);
		return kw;
	}

	throw std::runtime_error("Unknown keyword " + lexeme);
}
