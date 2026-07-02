#pragma once

#include "Ast.h"

#include <array>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct IrNode {
	std::string atom;
	int id = 0;
	std::array<int, 6> conn{};
};

struct Fragment {
	std::vector<IrNode> ir;
	int entry = 0;
	std::optional<int> exit = 0; // std::nullopt means terminal: attach it without advancing the current CORE tail.
	int entryBond = 1;
	std::vector<int> interfaceL;
	std::vector<int> interfaceR;
	InterfaceKind interfaceLKind = InterfaceKind::Connect;
	InterfaceKind interfaceRKind = InterfaceKind::Connect;
	std::map<int, int> defaultNext;
};

IrNode makeNode(const std::string& atom, int id, std::initializer_list<int> conns);

Fragment translateCpd(const Cpd& cpd);
void printIr(const Fragment& frag, std::ostream& out);
