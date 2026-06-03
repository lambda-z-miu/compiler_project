#include "tokenizer.h"

#include <iostream>
#include <string>

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
		for (const auto& tok : tokens) {
			std::cout << tokenTypeName(tok.type) << "\t" << tok.lexeme << "\t" << tok.pos << "\n";
		}
	} catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << "\n";
		return 1;
	}

	return 0;
}
