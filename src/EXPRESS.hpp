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
		RIGHT_PARENTHESIS,
		USER_DEFINED_KEYWORD,
		OMITTED_PARAMETER,
		INTEGER,
		REAL,
		STRING,
		ENTITY_INSTANCE_NAME,
		BINARY,
		DOLLAR,
		END_SEC,
		DATA,
		EQUAL,
		AMPERSAND,
		SCOPE,
		ENDSCOPE,
		FORWARD_SLASH,
		ENUMERATION,
		STANDARD_KEYWORD,
		COUNT
	};

	Read_String text;
	double number = 0;
	Kind kind;
};


struct Node {
	enum class Kind {
		ENTITY_INSTANCE,
		SIMPLE_RECORD,
	};
	Kind kind;
	union {
		struct {
			size_t instance_name;
			size_t scope = SIZE_MAX;
			size_t simple_record = SIZE_MAX;
			size_t subsuper_record = SIZE_MAX;
		} entity_instance;
		union {
			size_t keyword;
			SSO_Array<size_t, 3> parameters;
		} simple_record;
	};

	Node() {}
	Node(const Node& other) {
		*this = other;
	}
	Node& operator=(const Node& other) {
		if (this == &other) return *this;
		memcpy(this, &other, sizeof(Node));
		return *this;
	}
	Node(Node&& other) {
		*this = (Node&&)other;
	}
	Node& operator=(Node&& other) {
		if (this == &other) return *this;
		memcpy(this, &other, sizeof(Node));
		return *this;
	}
	~Node() {}
};
struct parse_express_from_memory_result {
	DynArray<Read_String> diagnostic;
	bool error = false;

	size_t cursor = 0;

	DynArray<Node> nodes;
	DynArray<Node> temp_nodes;

	DynArray<Token> tokens;
	struct Branch {
		size_t token_index = 0;
		size_t node_index = 0;
		size_t diagnostic_index = 0;
		size_t cursor = 0;
		bool error = false;
	};
	DynArray<Branch> branches;

	void new_branch() {
		branches.push({
			tokens.size, nodes.size, diagnostic.size, cursor, error
		});
	}
	void pop_branch() {
		auto& branch = branches[branches.size - 1];
		tokens.size = branch.token_index;
		diagnostic.size = branch.diagnostic_index;
		nodes.size = branch.node_index;
		cursor = branch.cursor;
		error = branch.error;
		branches.size -= 1;
	}
	void commit_branch() {
		branches.size -= 1;
	}
	void clear() {
		tokens.size = 0;
		diagnostic.size = 0;
		cursor = 0;
		nodes.size = 0;
		error = false;
		branches.size = 0;
	}
};
[[nodiscard]]
extern parse_express_from_memory_result parse_express_from_memory(Read_String file);
extern void parse_express_from_memory(Read_String file, parse_express_from_memory_result& out);
