#pragma once

#include <array>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

struct Cpd;

struct IrNode {
	std::string atom;
	int id = 0;
	std::array<int, 6> conn{};
};

struct Fragment {
	std::vector<IrNode> ir;
	int entry = 0;
	int exit = 0;
	int entryBond = 1;
	std::vector<int> interfaceL;
	std::vector<int> interfaceR;
	std::map<int, int> defaultNext;
};

IrNode makeNode(const std::string& atom, int id, std::initializer_list<int> conns);

Fragment translateCpd(const Cpd& cpd);
void printIr(const Fragment& frag, std::ostream& out);
