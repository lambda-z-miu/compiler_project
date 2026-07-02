#pragma once

#include <memory>
#include <string>
#include <vector>

struct Poses {
	std::vector<int> values;
};

struct Core {
	std::vector<std::string> keywords;
};

struct Sub {
	enum class Kind {
		Identifier,
		Core
	};

	Kind kind = Kind::Identifier;
	std::string identifier;
	Core core;
};

struct Interface {
	Poses poses;
};

struct Cpo;

struct Subs {
	enum class Kind {
		Empty,
		BareSub,
		Entry
	};

	Kind kind = Kind::Empty;
	Sub bareSub;
	Poses poses;
	bool isGroup = false;
	bool replace = false;
	Sub sub;
	std::unique_ptr<Cpo> group;
	std::unique_ptr<Subs> next;
};

struct Cpo {
	Interface left;
	Core core;
	Subs subs;
	Interface right;
};

struct Cpd {
	std::vector<Cpo> items;
};
