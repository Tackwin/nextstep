#pragma once

#include "Common.hpp"
#include "Memory.hpp"


struct Token {
	enum class Kind : u8 {
		ISO_10303_21 = 0,
		END_ISO_10303_21,
		SEMICOLON,
		COMMA,
		HEADER,
		LEFT_PARENTHESIS,
		USER_DEFINED_KEYWORD,
		OMITTED_PARAMETER,
		INTEGER,
		REAL,
		STRING,
		ENTITY_INSTANCE_NAME,
		BINARY,
		DOLLAR,
		END_SEC,
		COUNT
	};

	Kind kind;
	size_t line = 0;
	size_t column = 0;
	Read_String text;

	double number = 0;

	Read_String user_defined_keyword;
	Read_String standard_keyword;
};

struct parse_express_from_memory_result {
	DynArray<u8> diagnostic;
	bool error = false;

	size_t cursor = 0;

	DynArray<Token> tokens;
	struct Branch {
		size_t token_index = 0;
		size_t diagnostic_index = 0;
		size_t cursor = 0;
		bool error = false;
	};
	DynArray<Branch> branches;

	void new_branch() {
		branches.push({ tokens.size, diagnostic.size, cursor, error });
	}
	void pop_branch() {
		auto& branch = branches[branches.size - 1];
		tokens.size = branch.token_index;
		diagnostic.size = branch.diagnostic_index;
		cursor = branch.cursor;
		error = branch.error;
		branches.size -= 1;
	}
	void commit_branch() {
		branches.size -= 1;
	}
};
[[nodiscard]]
extern parse_express_from_memory_result parse_express_from_memory(Read_String file);

