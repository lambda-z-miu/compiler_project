#include "Translation.h"

#include "Ast.h"
#include "Keyword.h"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct SubEntry {
	std::vector<int> poses;
	Fragment frag;
	bool replace = false;
};

Fragment translateCpo(const Cpo& cpo);
Fragment translateCore(const Core& core);
Fragment translateSub(const Sub& sub);
std::vector<SubEntry> translateSubs(const Subs& subs);

int maxId(const std::vector<IrNode>& ir) {
	int max = 0;
	for (const auto& node : ir) {
		if (node.id > max) {
			max = node.id;
		}
	}
	return max;
}

int maxId(const Fragment& frag) {
	return maxId(frag.ir);
}

IrNode* findNode(std::vector<IrNode>& ir, int id) {
	for (auto& node : ir) {
		if (node.id == id) {
			return &node;
		}
	}
	return nullptr;
}

const IrNode* findNode(const std::vector<IrNode>& ir, int id) {
	for (const auto& node : ir) {
		if (node.id == id) {
			return &node;
		}
	}
	return nullptr;
}

bool hasNode(const std::vector<IrNode>& ir, int id) {
	return findNode(ir, id) != nullptr;
}

int firstNonZero(const std::vector<int>& values) {
	for (int value : values) {
		if (value != 0) {
			return value;
		}
	}
	return 0;
}

std::vector<int> nonZero(const std::vector<int>& values) {
	std::vector<int> out;
	for (int value : values) {
		if (value != 0) {
			out.push_back(value);
		}
	}
	return out;
}

bool allZero(const std::vector<int>& values) {
	if (values.empty()) {
		return true;
	}
	for (int value : values) {
		if (value != 0) {
			return false;
		}
	}
	return true;
}

int rewriteId(int id, const std::map<int, int>& aliases) {
	if (id == 0) {
		return 0;
	}
	auto it = aliases.find(id);
	return it == aliases.end() ? id : it->second;
}

void shiftInt(int& value, int offset) {
	if (value > 0) {
		value += offset;
	}
}

Fragment shiftFragment(Fragment frag, int offset) {
	if (offset == 0) {
		return frag;
	}

	for (auto& node : frag.ir) {
		shiftInt(node.id, offset);
		for (int& conn : node.conn) {
			shiftInt(conn, offset);
		}
	}
	shiftInt(frag.entry, offset);
	if (frag.exit) {
		shiftInt(*frag.exit, offset);
	}
	for (int& value : frag.interfaceL) {
		shiftInt(value, offset);
	}
	for (int& value : frag.interfaceR) {
		shiftInt(value, offset);
	}

	std::map<int, int> shifted;
	for (const auto& [from, to] : frag.defaultNext) {
		int shiftedFrom = from;
		int shiftedTo = to;
		shiftInt(shiftedFrom, offset);
		shiftInt(shiftedTo, offset);
		shifted[shiftedFrom] = shiftedTo;
	}
	frag.defaultNext = std::move(shifted);
	return frag;
}

std::vector<int> rewriteVector(const std::vector<int>& values, const std::map<int, int>& aliases) {
	std::vector<int> out;
	out.reserve(values.size());
	for (int value : values) {
		out.push_back(rewriteId(value, aliases));
	}
	return out;
}

std::map<int, int> rewriteDefaultNext(const std::map<int, int>& values, const std::map<int, int>& aliases) {
	std::map<int, int> out;
	for (const auto& [from, to] : values) {
		int rewrittenFrom = rewriteId(from, aliases);
		int rewrittenTo = rewriteId(to, aliases);
		if (rewrittenFrom != 0 && rewrittenTo != 0 && rewrittenFrom != rewrittenTo) {
			out.emplace(rewrittenFrom, rewrittenTo);
		}
	}
	return out;
}

void mergeDefaultNext(std::map<int, int>& into, const std::map<int, int>& from) {
	for (const auto& [key, value] : from) {
		if (key != 0 && value != 0 && key != value) {
			into.emplace(key, value);
		}
	}
}

void appendIr(std::vector<IrNode>& into, const std::vector<IrNode>& from) {
	for (const auto& node : from) {
		if (hasNode(into, node.id)) {
			throw std::runtime_error("Duplicate IR node id " + std::to_string(node.id));
		}
		into.push_back(node);
	}
}

void insertConn(Fragment& frag, int a, int b, int times) {
	IrNode* node = findNode(frag.ir, a);
	if (!node) {
		throw std::runtime_error("IR node " + std::to_string(a) + " does not exist");
	}

	for (int i = 0; i < times; ++i) {
		auto slot = std::find(node->conn.begin(), node->conn.end(), 0);
		if (slot == node->conn.end()) {
			throw std::runtime_error("Valence overflow at IR node " + std::to_string(a));
		}
		*slot = b;
	}
}

void link(Fragment& frag, int a, int b, int order) {
	if (a == 0 || b == 0) {
		throw std::runtime_error("Cannot link zero IR node id");
	}
	if (order < 1 || order > 3) {
		throw std::runtime_error("Unsupported bond order " + std::to_string(order));
	}
	if (!hasNode(frag.ir, a) || !hasNode(frag.ir, b)) {
		throw std::runtime_error("Cannot link missing IR node " + std::to_string(a) + " and " + std::to_string(b));
	}
	insertConn(frag, a, b, order);
	insertConn(frag, b, a, order);
}

int bondOrder(const Fragment& frag, int a, int b) {
	const IrNode* node = findNode(frag.ir, a);
	if (!node) {
		return 0;
	}

	int count = 0;
	for (int conn : node->conn) {
		if (conn == b) {
			++count;
		}
	}
	return count;
}

void ensureBond(Fragment& frag, int a, int b, int order) {
	if (a == 0 || b == 0) {
		throw std::runtime_error("Cannot ensure bond on zero IR node id");
	}
	if (!hasNode(frag.ir, a) || !hasNode(frag.ir, b)) {
		throw std::runtime_error("Cannot ensure bond between missing IR nodes");
	}

	int oldOrder = bondOrder(frag, a, b);
	if (oldOrder == 0) {
		link(frag, a, b, order);
		return;
	}
	if (oldOrder < order) {
		insertConn(frag, a, b, order - oldOrder);
		insertConn(frag, b, a, order - oldOrder);
	}
}

void validatePorts(const Fragment& frag, const std::vector<int>& ports, const char* label) {
	for (int port : ports) {
		if (port != 0 && !hasNode(frag.ir, port)) {
			throw std::runtime_error(std::string("Unknown ") + label + " interface node " + std::to_string(port));
		}
	}
}

Fragment buildCore(const std::vector<std::string>& keywords) {
	Fragment frag;
	int pendingBond = 1;
	int leadingBond = 1;

	for (const auto& lexeme : keywords) {
		KeywordInfo kw = translateKeyword(lexeme);
		if (kw.kind == KeywordInfo::Kind::Bond) {
			if (frag.ir.empty()) {
				leadingBond = kw.bond;
			} else {
				pendingBond = kw.bond;
			}
			continue;
		}

		Fragment next = shiftFragment(std::move(kw.frag), maxId(frag));
		if (frag.ir.empty()) {
			frag = std::move(next);
			frag.entryBond = std::max(leadingBond, frag.entryBond);
		} else {
			if (!frag.exit) {
				throw std::runtime_error("Cannot extend terminal fragment before keyword " + lexeme);
			}
			int oldExit = *frag.exit;
			int nextEntry = next.entry;
			int order = std::max(pendingBond, next.entryBond);
			appendIr(frag.ir, next.ir);
			link(frag, oldExit, nextEntry, order);
			frag.defaultNext.emplace(oldExit, nextEntry);
			if (next.exit) {
				frag.exit = next.exit;
			}
			mergeDefaultNext(frag.defaultNext, next.defaultNext);
		}
		pendingBond = 1;
	}

	if (frag.ir.empty()) {
		frag.entry = 0;
		frag.exit = 0;
		frag.entryBond = leadingBond;
	}
	return frag;
}

Fragment attachFragment(Fragment frag, int pose, const Fragment& sub) {
	if (pose == 0 || !hasNode(frag.ir, pose)) {
		throw std::runtime_error("Unknown attach pose " + std::to_string(pose));
	}

	if (sub.ir.empty()) {
		if (sub.entryBond <= 1) {
			return frag;
		}
		auto target = frag.defaultNext.find(pose);
		if (target == frag.defaultNext.end()) {
			throw std::runtime_error("No default bond target for pose " + std::to_string(pose));
		}
		ensureBond(frag, pose, target->second, sub.entryBond);
		return frag;
	}

	Fragment shifted = shiftFragment(sub, maxId(frag));
	std::vector<int> ports = nonZero(shifted.interfaceL);
	int entry = 0;
	if (!ports.empty()) {
		if (ports.size() != 1) {
			throw std::runtime_error("Group substituent must have exactly one non-zero left interface");
		}
		entry = ports.front();
	} else {
		entry = shifted.entry;
	}
	if (entry == 0) {
		throw std::runtime_error("Substituent has no entry node");
	}

	appendIr(frag.ir, shifted.ir);
	link(frag, pose, entry, shifted.entryBond);
	mergeDefaultNext(frag.defaultNext, shifted.defaultNext);
	return frag;
}

Fragment attachMany(Fragment frag, const std::vector<int>& poses, const Fragment& sub) {
	for (int pose : poses) {
		frag = attachFragment(std::move(frag), pose, sub);
	}
	return frag;
}
Fragment replaceFragment(Fragment frag, int pose, const Fragment& sub) {
	if (pose == 0 || !hasNode(frag.ir, pose)) {
		throw std::runtime_error("Unknown replace pose " + std::to_string(pose));
	}
	if (sub.ir.empty()) {
		throw std::runtime_error("Cannot replace an atom with an empty bond-only fragment");
	}

	Fragment shifted = shiftFragment(sub, maxId(frag));
	std::vector<int> ports = nonZero(shifted.interfaceL);
	int entry = 0;
	if (!ports.empty()) {
		if (ports.size() != 1) {
			throw std::runtime_error("Replacement group must have exactly one non-zero left interface");
		}
		entry = ports.front();
	} else {
		entry = shifted.entry;
	}
	if (entry == 0) {
		throw std::runtime_error("Replacement fragment has no entry node");
	}

	IrNode* target = findNode(frag.ir, pose);
	IrNode* replacement = findNode(shifted.ir, entry);
	if (!target || !replacement) {
		throw std::runtime_error("Replacement entry node is missing");
	}
	target->atom = replacement->atom;

	std::map<int, int> aliases{{entry, pose}};
	for (const auto& node : shifted.ir) {
		int rewrittenId = rewriteId(node.id, aliases);
		if (rewrittenId == pose) {
			continue;
		}
		if (hasNode(frag.ir, rewrittenId)) {
			throw std::runtime_error("Replacement creates duplicate IR node id " + std::to_string(rewrittenId));
		}
		frag.ir.push_back(makeNode(node.atom, rewrittenId, {}));
	}

	std::map<std::pair<int, int>, int> edgeOrder;
	for (const auto& node : shifted.ir) {
		int a = rewriteId(node.id, aliases);
		for (int conn : node.conn) {
			int b = rewriteId(conn, aliases);
			if (a == 0 || b == 0 || a >= b) {
				continue;
			}
			++edgeOrder[{a, b}];
		}
	}

	for (const auto& [edge, order] : edgeOrder) {
		ensureBond(frag, edge.first, edge.second, order);
	}
	mergeDefaultNext(frag.defaultNext, rewriteDefaultNext(shifted.defaultNext, aliases));
	return frag;
}

Fragment replaceMany(Fragment frag, const std::vector<int>& poses, const Fragment& sub) {
	for (int pose : poses) {
		frag = replaceFragment(std::move(frag), pose, sub);
	}
	return frag;
}

Fragment fuseFragments(Fragment left, Fragment right) {
	Fragment shifted = shiftFragment(std::move(right), maxId(left));
	if (left.interfaceR.size() != 2 || shifted.interfaceL.size() != 2) {
		throw std::runtime_error("Fuse requires two left and two right interface nodes");
	}

	std::map<int, int> aliases;
	for (size_t i = 0; i < 2; ++i) {
		if (left.interfaceR[i] == 0 || shifted.interfaceL[i] == 0) {
			throw std::runtime_error("Fuse interface cannot contain zero");
		}
		aliases[shifted.interfaceL[i]] = left.interfaceR[i];
	}
	if (aliases.size() != 2 || left.interfaceR[0] == left.interfaceR[1]) {
		throw std::runtime_error("Fuse interfaces must be distinct");
	}

	for (const auto& node : shifted.ir) {
		int rewrittenId = rewriteId(node.id, aliases);
		if (IrNode* existing = findNode(left.ir, rewrittenId)) {
			if (existing->atom != node.atom) {
				throw std::runtime_error("Fuse atom mismatch at IR node " + std::to_string(rewrittenId));
			}
		} else {
			left.ir.push_back(makeNode(node.atom, rewrittenId, {}));
		}
	}

	std::map<std::pair<int, int>, int> edgeOrder;
	for (const auto& node : shifted.ir) {
		int a = rewriteId(node.id, aliases);
		for (int conn : node.conn) {
			int b = rewriteId(conn, aliases);
			if (a == 0 || b == 0 || a >= b) {
				continue;
			}
			++edgeOrder[{a, b}];
		}
	}

	for (const auto& [edge, order] : edgeOrder) {
		ensureBond(left, edge.first, edge.second, order);
	}

	left.interfaceR = rewriteVector(shifted.interfaceR, aliases);
	left.exit = firstNonZero(left.interfaceR);
	mergeDefaultNext(left.defaultNext, rewriteDefaultNext(shifted.defaultNext, aliases));
	return left;
}

Fragment connectFragments(Fragment left, Fragment right) {
	if (left.ir.empty()) {
		return right;
	}
	if (right.ir.empty()) {
		return left;
	}
	if (allZero(left.interfaceR)) {
		throw std::runtime_error("Left fragment has no right interface");
	}
	if (allZero(right.interfaceL)) {
		throw std::runtime_error("Right fragment has no left interface");
	}

	std::vector<int> leftPorts = nonZero(left.interfaceR);
	std::vector<int> rightPorts = nonZero(right.interfaceL);
	if (leftPorts.size() == 1 && rightPorts.size() == 1) {
		Fragment shifted = shiftFragment(std::move(right), maxId(left));
		int shiftedRightPort = firstNonZero(shifted.interfaceL);
		appendIr(left.ir, shifted.ir);
		link(left, leftPorts.front(), shiftedRightPort, shifted.entryBond);
		left.interfaceR = shifted.interfaceR;
		left.exit = firstNonZero(left.interfaceR);
		mergeDefaultNext(left.defaultNext, shifted.defaultNext);
		return left;
	}
	if (leftPorts.size() == 2 && rightPorts.size() == 2) {
		return fuseFragments(std::move(left), std::move(right));
	}

	throw std::runtime_error("Interface size mismatch while connecting CPO fragments");
}

Fragment translateCore(const Core& core) {
	return buildCore(core.keywords);
}

Fragment translateSub(const Sub& sub) {
	if (sub.kind == Sub::Kind::Identifier) {
		throw std::runtime_error("Identifier lookup is not implemented for " + sub.identifier);
	}
	return translateCore(sub.core);
}

std::vector<SubEntry> translateSubs(const Subs& subs) {
	std::vector<SubEntry> entries;
	if (subs.kind == Subs::Kind::Empty) {
		return entries;
	}
	if (subs.kind == Subs::Kind::BareSub) {
		entries.push_back(SubEntry{{1}, translateSub(subs.bareSub), false});
		return entries;
	}

	Fragment frag;
	if (subs.isGroup) {
		frag = translateCpo(*subs.group);
		frag.entry = firstNonZero(frag.interfaceL);
		frag.exit = firstNonZero(frag.interfaceR);
		frag.entryBond = 1;
	} else {
		frag = translateSub(subs.sub);
	}
	entries.push_back(SubEntry{subs.poses.values, std::move(frag), subs.replace});

	if (subs.next) {
		std::vector<SubEntry> nextEntries = translateSubs(*subs.next);
		entries.insert(entries.end(), std::make_move_iterator(nextEntries.begin()), std::make_move_iterator(nextEntries.end()));
	}
	return entries;
}

Fragment translateCpo(const Cpo& cpo) {
	Fragment frag = translateCore(cpo.core);
	frag.interfaceL = cpo.left.poses.values;
	frag.interfaceR = cpo.right.poses.values;
	validatePorts(frag, frag.interfaceL, "left");
	validatePorts(frag, frag.interfaceR, "right");

	std::vector<SubEntry> entries = translateSubs(cpo.subs);
	for (const auto& entry : entries) {
		if (entry.replace) {
			frag = replaceMany(std::move(frag), entry.poses, entry.frag);
		} else {
			frag = attachMany(std::move(frag), entry.poses, entry.frag);
		}
	}

	frag.entry = firstNonZero(frag.interfaceL);
	frag.exit = firstNonZero(frag.interfaceR);
	return frag;
}

} // namespace

IrNode makeNode(const std::string& atom, int id, std::initializer_list<int> conns) {
	if (conns.size() > 6) {
		throw std::runtime_error("IR node has more than 6 connections");
	}

	IrNode node;
	node.atom = atom;
	node.id = id;
	size_t i = 0;
	for (int conn : conns) {
		node.conn[i++] = conn;
	}
	return node;
}

Fragment translateCpd(const Cpd& cpd) {
	Fragment result;
	for (const auto& cpo : cpd.items) {
		Fragment next = translateCpo(cpo);
		result = connectFragments(std::move(result), std::move(next));
	}
	return result;
}

void printIr(const Fragment& frag, std::ostream& out) {
	std::vector<IrNode> nodes = frag.ir;
	std::sort(nodes.begin(), nodes.end(), [](const IrNode& a, const IrNode& b) {
		return a.id < b.id;
	});

	out << "IR:\n";
	for (const auto& node : nodes) {
		out << node.atom << "(" << node.id;
		for (int conn : node.conn) {
			out << "," << conn;
		}
		out << ")\n";
	}
}
