#include "SvgRenderer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef COMPILER_PROJECT_HAVE_RDKIT
#include <GraphMol/Atom.h>
#include <GraphMol/Bond.h>
#include <GraphMol/Depictor/RDDepictor.h>
#include <GraphMol/MolDraw2D/MolDraw2DSVG.h>
#include <GraphMol/RDKitBase.h>
#include <GraphMol/RWMol.h>
#endif

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kBondLength = 58.0;

struct Vec2 {
	double x = 0.0;
	double y = 0.0;
};

struct RenderNode {
	int id = 0;
	std::string atom;
	Vec2 pos;
};

struct RenderEdge {
	int a = 0;
	int b = 0;
	int order = 1;
};

std::string number(double value) {
	std::ostringstream out;
	out << std::fixed << std::setprecision(2) << value;
	return out.str();
}

double length(Vec2 v) {
	return std::sqrt(v.x * v.x + v.y * v.y);
}

Vec2 add(Vec2 a, Vec2 b) {
	return Vec2{a.x + b.x, a.y + b.y};
}

Vec2 sub(Vec2 a, Vec2 b) {
	return Vec2{a.x - b.x, a.y - b.y};
}

Vec2 mul(Vec2 a, double k) {
	return Vec2{a.x * k, a.y * k};
}

bool isVisibleAtom(const RenderNode& node, int degree) {
	return node.atom != "C" || degree == 0;
}

std::string atomColor(const std::string& atom) {
	if (atom == "H") {
		return "#444444";
	}
	if (atom == "O") {
		return "#d12c2c";
	}
	if (atom == "N") {
		return "#1f5fbf";
	}
	if (atom == "P") {
		return "#b45f06";
	}
	if (atom == "S") {
		return "#c49a00";
	}
	if (atom == "F" || atom == "Cl" || atom == "Br" || atom == "I") {
		return "#167a3a";
	}
	return "#111111";
}

std::vector<RenderNode> makeNodes(const Fragment& frag) {
	std::vector<RenderNode> nodes;
	nodes.reserve(frag.ir.size());
	for (const auto& node : frag.ir) {
		nodes.push_back(RenderNode{node.id, node.atom, {}});
	}
	std::sort(nodes.begin(), nodes.end(), [](const RenderNode& a, const RenderNode& b) {
		return a.id < b.id;
	});
	return nodes;
}

std::vector<RenderEdge> makeEdges(const Fragment& frag) {
	std::map<std::pair<int, int>, int> orderByEdge;
	std::set<int> ids;
	for (const auto& node : frag.ir) {
		ids.insert(node.id);
	}

	for (const auto& node : frag.ir) {
		for (int conn : node.conn) {
			if (conn == 0 || !ids.contains(conn) || node.id >= conn) {
				continue;
			}
			++orderByEdge[{node.id, conn}];
		}
	}

	std::vector<RenderEdge> edges;
	edges.reserve(orderByEdge.size());
	for (const auto& [edge, order] : orderByEdge) {
		edges.push_back(RenderEdge{edge.first, edge.second, std::clamp(order, 1, 3)});
	}
	return edges;
}

#ifdef COMPILER_PROJECT_HAVE_RDKIT
int atomicNumberFor(const std::string& atom) {
	if (atom == "H") {
		return 1;
	}
	if (atom == "C") {
		return 6;
	}
	if (atom == "N") {
		return 7;
	}
	if (atom == "O") {
		return 8;
	}
	if (atom == "F") {
		return 9;
	}
	if (atom == "P") {
		return 15;
	}
	if (atom == "S") {
		return 16;
	}
	if (atom == "Cl") {
		return 17;
	}
	if (atom == "Br") {
		return 35;
	}
	if (atom == "I") {
		return 53;
	}
	throw std::runtime_error("Unsupported atom for RDKit renderer: " + atom);
}

RDKit::Bond::BondType bondTypeForOrder(int order) {
	if (order == 1) {
		return RDKit::Bond::SINGLE;
	}
	if (order == 2) {
		return RDKit::Bond::DOUBLE;
	}
	if (order == 3) {
		return RDKit::Bond::TRIPLE;
	}
	throw std::runtime_error("Unsupported bond order for RDKit renderer: " + std::to_string(order));
}

std::string renderSvgWithRdkit(const Fragment& frag) {
	RDKit::RWMol mol;
	std::vector<RenderNode> nodes = makeNodes(frag);
	std::vector<RenderEdge> edges = makeEdges(frag);
	std::map<int, unsigned int> atomIndexById;

	for (const auto& node : nodes) {
		auto* atom = new RDKit::Atom(atomicNumberFor(node.atom));
		atom->setNoImplicit(true);
		unsigned int atomIndex = mol.addAtom(atom, false, true);
		atomIndexById[node.id] = atomIndex;
	}

	for (const auto& edge : edges) {
		mol.addBond(atomIndexById.at(edge.a), atomIndexById.at(edge.b), bondTypeForOrder(edge.order));
	}

	mol.updatePropertyCache(false);
	RDDepict::compute2DCoords(mol);

	RDKit::MolDraw2DSVG drawer(450, 320);
	drawer.drawMolecule(mol);
	drawer.finishDrawing();
	return drawer.getDrawingText();
}
#endif

std::map<int, int> makeIndexById(const std::vector<RenderNode>& nodes) {
	std::map<int, int> indexById;
	for (size_t i = 0; i < nodes.size(); ++i) {
		indexById[nodes[i].id] = static_cast<int>(i);
	}
	return indexById;
}

std::vector<int> makeDegrees(const std::vector<RenderNode>& nodes, const std::vector<RenderEdge>& edges, const std::map<int, int>& indexById) {
	std::vector<int> degrees(nodes.size(), 0);
	for (const auto& edge : edges) {
		++degrees[indexById.at(edge.a)];
		++degrees[indexById.at(edge.b)];
	}
	return degrees;
}

void initializeLayout(std::vector<RenderNode>& nodes, const std::vector<RenderEdge>& edges, const std::map<int, int>& indexById) {
	const size_t n = nodes.size();
	if (n == 0) {
		return;
	}
	if (n == 1) {
		nodes[0].pos = Vec2{0.0, 0.0};
		return;
	}
	if (n == 2 && edges.size() == 1) {
		nodes[0].pos = Vec2{-kBondLength / 2.0, 0.0};
		nodes[1].pos = Vec2{kBondLength / 2.0, 0.0};
		return;
	}

	double radius = std::max(70.0, static_cast<double>(n) * 9.0);
	for (size_t i = 0; i < n; ++i) {
		double angle = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(n);
		nodes[i].pos = Vec2{std::cos(angle) * radius, std::sin(angle) * radius};
	}

	// Put direct neighbors near each other before the force pass starts.
	for (size_t i = 0; i < edges.size(); ++i) {
		const auto& edge = edges[i];
		int a = indexById.at(edge.a);
		int b = indexById.at(edge.b);
		double angle = 2.0 * kPi * static_cast<double>(i) / std::max<size_t>(1, edges.size());
		Vec2 delta{std::cos(angle) * kBondLength * 0.3, std::sin(angle) * kBondLength * 0.3};
		nodes[b].pos = add(nodes[a].pos, delta);
	}
}

void relaxLayout(std::vector<RenderNode>& nodes, const std::vector<RenderEdge>& edges, const std::map<int, int>& indexById) {
	const size_t n = nodes.size();
	if (n <= 1) {
		return;
	}

	std::vector<Vec2> disp(n);
	for (int iter = 0; iter < 700; ++iter) {
		std::fill(disp.begin(), disp.end(), Vec2{});

		for (size_t i = 0; i < n; ++i) {
			for (size_t j = i + 1; j < n; ++j) {
				Vec2 delta = sub(nodes[i].pos, nodes[j].pos);
				double dist = std::max(1.0, length(delta));
				Vec2 dir = mul(delta, 1.0 / dist);
				double force = 1800.0 / (dist * dist);
				disp[i] = add(disp[i], mul(dir, force));
				disp[j] = add(disp[j], mul(dir, -force));
			}
		}

		for (const auto& edge : edges) {
			int a = indexById.at(edge.a);
			int b = indexById.at(edge.b);
			Vec2 delta = sub(nodes[b].pos, nodes[a].pos);
			double dist = std::max(1.0, length(delta));
			Vec2 dir = mul(delta, 1.0 / dist);
			double target = kBondLength * (edge.order == 3 ? 0.95 : 1.0);
			double force = (dist - target) * 0.055;
			disp[a] = add(disp[a], mul(dir, force));
			disp[b] = add(disp[b], mul(dir, -force));
		}

		double step = 8.0 * (1.0 - static_cast<double>(iter) / 700.0) + 0.05;
		for (size_t i = 0; i < n; ++i) {
			double d = length(disp[i]);
			if (d > step) {
				disp[i] = mul(disp[i], step / d);
			}
			nodes[i].pos = add(nodes[i].pos, disp[i]);
		}
	}
}

void normalizeLayout(std::vector<RenderNode>& nodes) {
	if (nodes.empty()) {
		return;
	}

	double minX = nodes.front().pos.x;
	double maxX = nodes.front().pos.x;
	double minY = nodes.front().pos.y;
	double maxY = nodes.front().pos.y;
	for (const auto& node : nodes) {
		minX = std::min(minX, node.pos.x);
		maxX = std::max(maxX, node.pos.x);
		minY = std::min(minY, node.pos.y);
		maxY = std::max(maxY, node.pos.y);
	}

	double cx = (minX + maxX) / 2.0;
	double cy = (minY + maxY) / 2.0;
	for (auto& node : nodes) {
		node.pos.x -= cx;
		node.pos.y -= cy;
	}
}

std::string lineElement(Vec2 a, Vec2 b, const std::string& klass = "bond") {
	std::ostringstream out;
	out << "<line class=\"" << klass << "\" x1=\"" << number(a.x) << "\" y1=\"" << number(a.y)
		<< "\" x2=\"" << number(b.x) << "\" y2=\"" << number(b.y) << "\" />\n";
	return out.str();
}

void appendBond(std::ostringstream& out, Vec2 a, Vec2 b, int order, bool labelA, bool labelB) {
	Vec2 delta = sub(b, a);
	double dist = std::max(1.0, length(delta));
	Vec2 dir = mul(delta, 1.0 / dist);
	Vec2 normal{-dir.y, dir.x};

	double trimA = labelA ? 12.0 : 2.0;
	double trimB = labelB ? 12.0 : 2.0;
	a = add(a, mul(dir, trimA));
	b = add(b, mul(dir, -trimB));

	if (order <= 1) {
		out << lineElement(a, b);
		return;
	}

	if (order == 2) {
		double offset = 3.2;
		out << lineElement(add(a, mul(normal, offset)), add(b, mul(normal, offset)));
		out << lineElement(add(a, mul(normal, -offset)), add(b, mul(normal, -offset)));
		return;
	}

	double offset = 4.5;
	out << lineElement(a, b);
	out << lineElement(add(a, mul(normal, offset)), add(b, mul(normal, offset)));
	out << lineElement(add(a, mul(normal, -offset)), add(b, mul(normal, -offset)));
}

std::string renderAtomLabel(const RenderNode& node, Vec2 p) {
	std::ostringstream out;
	double radius = node.atom.size() == 1 ? 9.5 : 12.5;
	out << "<circle class=\"atom-bg\" cx=\"" << number(p.x) << "\" cy=\"" << number(p.y)
		<< "\" r=\"" << number(radius) << "\" />\n";
	out << "<text class=\"atom\" fill=\"" << atomColor(node.atom) << "\" x=\"" << number(p.x)
		<< "\" y=\"" << number(p.y) << "\">" << node.atom << "</text>\n";
	return out.str();
}

std::string renderSvgFallback(const Fragment& frag) {
	std::vector<RenderNode> nodes = makeNodes(frag);
	std::vector<RenderEdge> edges = makeEdges(frag);
	std::map<int, int> indexById = makeIndexById(nodes);
	std::vector<int> degrees = makeDegrees(nodes, edges, indexById);

	initializeLayout(nodes, edges, indexById);
	relaxLayout(nodes, edges, indexById);
	normalizeLayout(nodes);

	double minX = 0.0;
	double maxX = 0.0;
	double minY = 0.0;
	double maxY = 0.0;
	if (!nodes.empty()) {
		minX = maxX = nodes.front().pos.x;
		minY = maxY = nodes.front().pos.y;
		for (const auto& node : nodes) {
			minX = std::min(minX, node.pos.x);
			maxX = std::max(maxX, node.pos.x);
			minY = std::min(minY, node.pos.y);
			maxY = std::max(maxY, node.pos.y);
		}
	}

	double padding = 36.0;
	double width = std::max(96.0, maxX - minX + padding * 2.0);
	double height = std::max(96.0, maxY - minY + padding * 2.0);
	for (auto& node : nodes) {
		node.pos.x = node.pos.x - minX + padding;
		node.pos.y = node.pos.y - minY + padding;
	}

	std::ostringstream out;
	out << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " << number(width) << " " << number(height)
		<< "\" width=\"" << number(width) << "\" height=\"" << number(height) << "\">\n";
	out << "<style>"
		<< ".bond{stroke:#111;stroke-width:2.2;stroke-linecap:round;stroke-linejoin:round}"
		<< ".atom{font-family:Arial,Helvetica,sans-serif;font-size:16px;font-weight:700;text-anchor:middle;dominant-baseline:central}"
		<< ".atom-bg{fill:#fff;stroke:none}"
		<< "</style>\n";
	out << "<rect width=\"100%\" height=\"100%\" fill=\"#fff\" />\n";

	for (const auto& edge : edges) {
		int a = indexById.at(edge.a);
		int b = indexById.at(edge.b);
		bool labelA = isVisibleAtom(nodes[a], degrees[a]);
		bool labelB = isVisibleAtom(nodes[b], degrees[b]);
		appendBond(out, nodes[a].pos, nodes[b].pos, edge.order, labelA, labelB);
	}

	for (size_t i = 0; i < nodes.size(); ++i) {
		if (isVisibleAtom(nodes[i], degrees[i])) {
			out << renderAtomLabel(nodes[i], nodes[i].pos);
		}
	}

	out << "</svg>";
	return out.str();
}

} // namespace

std::string renderSvg(const Fragment& frag) {
#ifdef COMPILER_PROJECT_HAVE_RDKIT
	return renderSvgWithRdkit(frag);
#else
	return renderSvgFallback(frag);
#endif
}
