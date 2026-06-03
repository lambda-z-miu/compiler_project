#pragma once

#include "Translation.h"

#include <string>

struct KeywordInfo {
	enum class Kind {
		Atom,
		Bond
	};

	Kind kind = Kind::Atom;
	int bond = 1;
	Fragment frag;
};

KeywordInfo translateKeyword(const std::string& lexeme);
