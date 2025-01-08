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
		COUNT
	};

	Kind kind;
	size_t line = 0;
	size_t column = 0;
	Read_String text;

	Read_String user_defined_keyword;
	Read_String standard_keyword;
};

struct parse_express_from_memory_result {
	DynArray<u8> diagnostic;
	bool error = false;

	DynArray<Token> tokens;
};
[[nodiscard]]
extern parse_express_from_memory_result parse_express_from_memory(Read_String file);

