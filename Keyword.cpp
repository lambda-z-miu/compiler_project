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
		makeNode("C", 2, {3, 3, 7, 1}),
		makeNode("C", 3, {2, 2, 4}),
		makeNode("C", 4, {5, 5, 3}),
		makeNode("C", 5, {4, 4, 6}),
		makeNode("C", 6, {7, 7, 5}),
		makeNode("C", 7, {6, 6, 2}),
	};
	frag.entry = 1;
	frag.exit = 2;
	frag.defaultNext[1] = 2;
	for (int i = 2; i <= 7; ++i) {
		frag.defaultNext[i] = i == 7 ? 2 : i + 1;
	}
	return frag;
}


Fragment makeNH2() {
	Fragment frag;
	frag.ir = {
		makeNode("N", 1, {2, 3}),
		makeNode("H", 2, {1}),
		makeNode("H", 3, {1}),
	};
	frag.entry = 1;
	frag.exit = 1;
	return frag;
}

Fragment makeOH() {
	Fragment frag;
	frag.ir = {
		makeNode("O", 1, {2}),
		makeNode("H", 2, {1}),
	};
	frag.entry = 1;
	frag.exit = std::nullopt;
	return frag;
}


Fragment makeSH() {
	Fragment frag;
	frag.ir = {
		makeNode("S", 1, {2}),
		makeNode("H", 2, {1}),
	};
	frag.entry = 1;
	frag.exit = std::nullopt;
	return frag;
}

Fragment makeIm() {
	Fragment frag;
	frag.ir = {
		makeNode("C", 1, {2, 5, 5}),
		makeNode("N", 2, {1, 3, 3}),
		makeNode("C", 3, {2, 2, 4}),
		makeNode("N", 4, {3, 5, 6}),
		makeNode("C", 5, {4, 1, 1}),
		makeNode("H", 6, {4}),
	};
	frag.entry = 1;
	frag.exit = std::nullopt;
	return frag;
}

Fragment makeInd() {
	Fragment frag;
	frag.ir = {
		makeNode("C", 1, {2, 2, 5}),
		makeNode("C", 2, {1, 1, 3}),
		makeNode("N", 3, {2, 4, 10}),
		makeNode("C", 4, {3, 5, 5, 9}),
		makeNode("C", 5, {4, 4, 1, 6}),
		makeNode("C", 6, {5, 7, 7}),
		makeNode("C", 7, {6, 6, 8}),
		makeNode("C", 8, {7, 9, 9}),
		makeNode("C", 9, {8, 8, 4}),
		makeNode("H", 10, {3}),
	};
	frag.entry = 1;
	frag.exit = std::nullopt;
	return frag;
}

Fragment makePyr() {
	Fragment frag;
	frag.ir = {
		makeNode("N", 1, {2, 5, 6}),
		makeNode("C", 2, {1, 3}),
		makeNode("C", 3, {2, 4}),
		makeNode("C", 4, {3, 5}),
		makeNode("C", 5, {4, 1}),
		makeNode("H", 6, {1}),
	};
	frag.entry = 2;
	frag.exit = 2;
	for (int i = 1; i <= 5; ++i) {
		frag.defaultNext[i] = i == 5 ? 1 : i + 1;
	}
	return frag;
}


Fragment makeOx5() {
	Fragment frag;
	frag.ir = {
		makeNode("O", 1, {2, 5}),
		makeNode("C", 2, {1, 3}),
		makeNode("C", 3, {2, 4}),
		makeNode("C", 4, {3, 5}),
		makeNode("C", 5, {4, 1}),
	};
	frag.entry = 1;
	frag.exit = 1;
	for (int i = 1; i <= 5; ++i) {
		frag.defaultNext[i] = i == 5 ? 1 : i + 1;
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
		kw.frag.exit = std::nullopt;
		return kw;
	}
	if (lexeme == "Ac") {
		kw.frag = makeAc();
		kw.frag.exit = std::nullopt;
		return kw;
	}
	if (lexeme == "=O") {
		kw.frag = makeSingleAtom("O");
		kw.frag.entryBond = 2;
		kw.frag.exit = std::nullopt;
		return kw;
	}
	if (lexeme == "OMe") {
		kw.frag = makeOMe();
		kw.frag.exit = std::nullopt;
		return kw;
	}
	if (lexeme == "OPh") {
		kw.frag = makeOPh();
		kw.frag.exit = std::nullopt;
		return kw;
	}
	if (lexeme == "NH2") {
		kw.frag = makeNH2();
		return kw;
	}
	if (lexeme == "OH") {
		kw.frag = makeOH();
		return kw;
	}
	if (lexeme == "SH") {
		kw.frag = makeSH();
		return kw;
	}
	if (lexeme == "Im") {
		kw.frag = makeIm();
		return kw;
	}
	if (lexeme == "Ind") {
		kw.frag = makeInd();
		return kw;
	}
	if (lexeme == "Pyr") {
		kw.frag = makePyr();
		return kw;
	}
	if (lexeme == "Ox5") {
		kw.frag = makeOx5();
		return kw;
	}
	if (lexeme == "C" || lexeme == "P" || lexeme == "O" || lexeme == "N" || lexeme == "S") {
		kw.frag = makeSingleAtom(lexeme);
		return kw;
	}
	if (lexeme == "F" || lexeme == "Cl" || lexeme == "Br" || lexeme == "I") {
		kw.frag = makeSingleAtom(lexeme);
		kw.frag.exit = std::nullopt;
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
