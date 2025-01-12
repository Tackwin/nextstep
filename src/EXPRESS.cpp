#include "EXPRESS.hpp"
#include "Common.hpp"

extern "C" {
	int _fltused = 0;
}

void report_error(parse_express_from_memory_result& res, Read_String msg) {
	// for (size_t i = 0; i < msg.size; ++i) {
	// 	res.diagnostic.push(msg[i]);
	// }
	res.diagnostic.push(msg);
	res.error = true;
}

struct Node_t {
	size_t* ref = nullptr;
	xstd::optional<size_t>* value = nullptr;

	Node_t() {}
	Node_t(size_t& ref) : ref(&ref) {}
	Node_t(xstd::optional<size_t>& value) : value(&value) {}
	Node_t(Node_t& other) : ref(other.ref), value(other.value) {}

	Node_t& operator=(size_t a) {
		if (ref)
			*ref = a;
		if (value) {
			xstd::optional<size_t>& vvalue = *value;
			vvalue = a;
		}
		return *this;
	}

	explicit operator size_t() {
		if (ref)
			return *ref;
		if (value) {
			xstd::optional<size_t>& vvalue = *value;
			return vvalue.value;
		}
		return SIZE_MAX;
	}
};
Node_t dump_token;

using eat_f = bool(parse_express_from_memory_result&, Read_String, Node_t);
bool eat_empty(parse_express_from_memory_result& res, Read_String file, Node_t) {
	return !res.error;
}

bool eat_list(
	parse_express_from_memory_result& res,
	Read_String file,
	eat_f item,
	eat_f sep,
	Node_t out
);
template<size_t N>
bool eat_litteral(
	parse_express_from_memory_result& res,
	Read_String file,
	const char (&litteral)[], Token::Kind kind
);
bool eat_header_section(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_whitespace(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_header_entity(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_keyword(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_user_defined_keyword(
	parse_express_from_memory_result& res, Read_String file, Node_t out
);
bool eat_standard_keyword(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_parameter(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_untyped_parameter(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_typed_parameter(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_number(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_string(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_entity_instance_name(
	parse_express_from_memory_result& res, Read_String file, Node_t out
);
bool eat_enumeration(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_binary(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_data_section(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_entity_instance_list(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_entity_instance(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_simple_entity_instance(
	parse_express_from_memory_result& res, Read_String file, Node_t &out
);
bool eat_complex_entity_instance(
	parse_express_from_memory_result& res, Read_String file, Node_t &out
);
bool eat_scope(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_export_list(parse_express_from_memory_result& res, Read_String file, Node_t out);
bool eat_maybe(parse_express_from_memory_result& res, Read_String file, eat_f item, Node_t out);

bool next_upper(Read_String file, size_t* cursor);
bool next_digit(Read_String file, size_t* cursor);
bool next_hex(Read_String file, size_t* cursor);
bool take_digit(Read_String file, size_t* cursor, u8* out);

bool eat_maybe(parse_express_from_memory_result& res, Read_String file, eat_f item, Node_t out) {
	if (res.error)
		return false;
	res.new_branch();
	if (item(res, file, out)) {
		res.commit_branch();
		return true;
	}
	else {
		res.pop_branch();
		return false;
	}
}

template<size_t N>
bool eat_litteral(
	parse_express_from_memory_result& res,
	Read_String file,
	const char (&litteral)[N], Token::Kind kind
) {
	if (res.error)
		return false;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, "Expected litteral, but reached end of file");
		report_error(res, "Expected litteral: ");
		report_error(res, litteral);
		return false;
	}

	if (begins_with_at(file, litteral, res.cursor)) {
		Token token;
		token.kind = kind;
		token.text = { file.data + res.cursor, N - 1 };
		res.tokens.push(token);
		res.cursor += N - 1;
		return true;
	} else {
		report_error(res, "Expected litteral: ");
		report_error(res, litteral);
		return false;
	}
	return !res.error;
}

bool eat_whitespace(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	while (res.cursor < file.size && (
		file[res.cursor] == ' ' ||
		file[res.cursor] == '\t' ||
		file[res.cursor] == '\n' ||
		file[res.cursor] == '\r'
	)) {
		res.cursor += 1;
	}
	return !res.error;
}

bool eat_export_list(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	eat_litteral(res, file, "/", Token::Kind::FORWARD_SLASH);
	eat_entity_instance_name(res, file, dump_token);

	res.new_branch();
	while (eat_litteral(res, file, ",", Token::Kind::COMMA)) {
		res.commit_branch();

		eat_entity_instance_name(res, file, dump_token);
		res.new_branch();
	}
	res.pop_branch();

	eat_litteral(res, file, "/", Token::Kind::FORWARD_SLASH);

	return !res.error;
}

bool eat_scope(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	eat_litteral(res, file, "&", Token::Kind::AMPERSAND);
	eat_litteral(res, file, "SCOPE", Token::Kind::SCOPE);
	eat_entity_instance_list(res, file, dump_token);
	eat_litteral(res, file, "ENDSCOPE", Token::Kind::ENDSCOPE);
	eat_maybe(res, file, eat_export_list, dump_token);
	return !res.error;
}

bool eat_simple_record(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	auto eat_comma = [] (parse_express_from_memory_result& res, Read_String file, Node_t out) {
		return eat_litteral(res, file, ",", Token::Kind::COMMA);
	};

	Node node(Node::Kind::SIMPLE_RECORD);
	eat_keyword(res, file, node.simple_record.keyword_token);
	eat_list(res, file, eat_parameter, eat_comma, node.simple_record.parameters);
	out = res.nodes.size;
	res.nodes.push(xstd::move(node));
	return !res.error;
}

bool eat_simple_entity_instance(
	parse_express_from_memory_result& res, Read_String file, Node_t out
) {
	if (res.error)
		return false;

	Node node(Node::Kind::ENTITY_INSTANCE);

	eat_entity_instance_name(res, file, node.entity_instance.entity_instance_name);
	eat_litteral(res, file, "=", Token::Kind::EQUAL);
	eat_maybe(res, file, eat_scope, node.entity_instance.scope);

	eat_simple_record(res, file, node.entity_instance.simple_record);

	eat_litteral(res, file, ";", Token::Kind::SEMICOLON);

	out = res.nodes.size;
	res.nodes.push(xstd::move(node));

	return !res.error;
}

bool eat_subsuper_record(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	eat_list(res, file, eat_simple_record, eat_empty, out);
	return !res.error;
}

bool eat_complex_entity_instance(
	parse_express_from_memory_result& res, Read_String file, Node_t out
) {
	if (res.error)
		return false;

	Node node(Node::Kind::ENTITY_INSTANCE);

	eat_entity_instance_name(res, file, node.entity_instance.entity_instance_name);
	eat_litteral(res, file, "=", Token::Kind::EQUAL);

	eat_maybe(res, file, eat_scope, node.entity_instance.scope);

	if (res.error)
		return false;
	eat_subsuper_record(res, file, node.entity_instance.subsuper_record);
	eat_litteral(res, file, ";", Token::Kind::SEMICOLON);

	out = res.nodes.size;
	res.nodes.push(xstd::move(node));

	return !res.error;
}

bool eat_entity_instance_name(
	parse_express_from_memory_result& res, Read_String file, Node_t out
) {
	if (res.error)
		return false;
	Token token;
	Node node(Node::Kind::ENTITY_INSTANCE_NAME);
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, "Expected entity instance name, but reached end of file");
		return false;
	}
	if (file[res.cursor] != '#') {
		report_error(res, "Expected entity instance name, but didn't find a '#'");
		return false;
	}

	size_t beg = res.cursor;
	res.cursor += 1;
	while (next_digit(file, &res.cursor));

	size_t end = res.cursor;
	token.kind = Token::Kind::ENTITY_INSTANCE_NAME;
	token.text = { file.data + beg, end - beg };
	res.tokens.push(token);

	node.integer = parse_size_t({ token.text.data + 1, token.text.size - 1 });
	out = res.nodes.size;
	res.nodes.push(xstd::move(node));
	return !res.error;
}

bool eat_binary(parse_express_from_memory_result& res, Read_String file, Node_t out)
{
	if (res.error)
		return false;
	Token token;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, "Expected list, but reached end of file");
		return false;
	}

	if (file[res.cursor] != '"') {
		report_error(res, "Expected list, but didn't find a '('");
		return false;
	}
	res.cursor += 1;
	if (res.cursor >= file.size) {
		report_error(res, "Expected list, but reached end of file");
		return false;
	}

	if (
		file[res.cursor] != '0' && file[res.cursor] != '1' && file[res.cursor] != '2' && file[res.cursor] != '3'
	) {
		report_error(res, "Expected list, but didn't find a '('");
		return false;
	}
	
	size_t beg = res.cursor;
	res.cursor += 1;

	while (next_hex(file, &res.cursor));

	if (res.cursor >= file.size) {
		report_error(res, "Expected list, but reached end of file");
		return false;
	}

	if (file[res.cursor] != '"') {
		report_error(res, "Expected list, but didn't find a '('");
		return false;
	}
	res.cursor += 1;

	size_t end = res.cursor;
	token.kind = Token::Kind::BINARY;
	token.text = { file.data + beg, end - beg };
	res.tokens.push(token);

	Node node(Node::Kind::BINARY);
	node.binary = { file.data + beg + 1, end - beg - 2 };
	out = res.nodes.size;
	res.nodes.push(xstd::move(node));
	return !res.error;
}

bool eat_entity_instance(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	if (eat_maybe(res, file, eat_simple_entity_instance, dump_token))
		return true;

	if (eat_maybe(res, file, eat_complex_entity_instance, dump_token))
		return true;
	

	report_error(res, "Expected entity instance, but found neither simple nor complex");
	return !res.error;
}

bool eat_entity_instance_list(parse_express_from_memory_result& res, Read_String file, Node_t out)
{
	if (res.error)
		return false;

	eat_entity_instance(res, file, dump_token);
	while (eat_maybe(res, file, eat_entity_instance, dump_token));

	return !res.error;
}

bool eat_data_section(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	eat_litteral(res, file, "DATA;", Token::Kind::DATA);
	eat_entity_instance_list(res, file, dump_token);
	eat_litteral(res, file, "ENDSEC;", Token::Kind::END_SEC);

	return !res.error;
}

bool eat_enumeration(parse_express_from_memory_result& res, Read_String file, Node_t out)
{
	if (res.error)
		return false;

	Token token;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, "Expected enumeration, but reached end of file");
		return false;
	}

	if (file[res.cursor] != '.') {
		report_error(res, "Expected enumeration, but didn't find a '.'");
		return false;
	}

	size_t beg = res.cursor;
	res.cursor += 1;

	if (!next_upper(file, &res.cursor)) {
		report_error(res, "Expected enumeration, but didn't find an uppercase letter");
		return false;
	}

	while (next_upper(file, &res.cursor) || next_digit(file, &res.cursor));

	if (res.cursor >= file.size) {
		report_error(res, "Expected enumeration, but reached end of file");
		return false;
	}

	if (file[res.cursor] != '.') {
		report_error(res, "Expected enumeration, but didn't find a '.'");
		return false;
	}

	size_t end = res.cursor;
	res.cursor += 1;

	token.kind = Token::Kind::ENUMERATION;
	token.text = { file.data + beg, end - beg };
	res.tokens.push(token);

	Node node(Node::Kind::ENUMERATION);
	node.enumeration = { file.data + beg + 1, end - beg - 2 };
	out = res.nodes.size;
	res.nodes.push(xstd::move(node));
	return !res.error;
}

bool eat_string(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;
	Token token;

	Node node(Node::Kind::STRING);

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, "Expected string, but reached end of file");
		return false;
	}

	if (file[res.cursor] != '\'') {
		report_error(res, "Expected string, but didn't find a single quote");
		return false;
	}

	size_t beg = res.cursor;
	res.cursor += 1;

	while (res.cursor < file.size) {
		if (file[res.cursor] == '\'') {
			if (res.cursor + 1 < file.size && file[res.cursor + 1] == '\'') {
				res.cursor += 2;
				continue;
			}
			break;
		}
		res.cursor += 1;
	}

	if (res.cursor >= file.size) {
		report_error(res, "Expected string, but reached end of file");
		return false;
	}

	if (file[res.cursor] != '\'') {
		report_error(res, "Expected end of string, but didn't find a single quote");
		return false;
	}
	res.cursor += 1;

	size_t end = res.cursor;
	token.kind = Token::Kind::STRING;
	token.text = { file.data + beg, end - beg };
	res.tokens.push(token);

	node.string = { file.data + beg + 1, end - beg - 2 };
	out = res.nodes.size;
	res.nodes.push(xstd::move(node));
	return !res.error;
}

bool eat_number(parse_express_from_memory_result& res, Read_String file, Node_t out)
{
	if (res.error)
		return false;
	Token token;

	Node node(Node::Kind::NUMBER);

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, "Expected number, but reached end of file");
		return false;
	}

	size_t beg = res.cursor;
	double sign = +1;

	if (file[res.cursor] == '-') {
		sign = -1;
		res.cursor += 1;
	}

	double integer = 0;
	u8 digit = 0;
	while (take_digit(file, &res.cursor, &digit)) {
		integer = integer * 10 + (digit - '0');
	}

	if (res.cursor == beg) {
		report_error(res, "Expected number, but didn't find a digit");
		return false;
	}

	if (res.cursor >= file.size) {
		token.kind = Token::Kind::INTEGER;
		token.text = { file.data + beg, res.cursor - beg };
		res.tokens.push(token);

		out = res.nodes.size;
		node.number = sign * integer;
		res.nodes.push(xstd::move(node));
		return true;
	}

	if (file[res.cursor] == '.') {
		res.cursor += 1;
	} else {
		token.kind = Token::Kind::INTEGER;
		token.text = { file.data + beg, res.cursor - beg };
		res.tokens.push(token);

		node.number = sign * integer;
		out = res.nodes.size;
		res.nodes.push(xstd::move(node));
		return true;
	}

	double frac = 0;
	double power = 0.1;
	while (take_digit(file, &res.cursor, &digit)) {
		frac += power * (digit - '0');
		power *= 0.1;
	}

	if (res.cursor >= file.size) {
		token.kind = Token::Kind::REAL;
		token.text = { file.data + beg, res.cursor - beg };
		res.tokens.push(token);

		node.number = sign * (integer + frac);
		out = res.nodes.size;
		res.nodes.push(xstd::move(node));
		return true;
	}

	if (file[res.cursor] == 'E' || file[res.cursor] == 'e') {
		res.cursor += 1;
	} else {
		token.kind = Token::Kind::REAL;
		token.text = { file.data + beg, res.cursor - beg };
		res.tokens.push(token);

		node.number = sign * (integer + frac);
		out = res.nodes.size;
		res.nodes.push(xstd::move(node));
		return true;
	}

	if (res.cursor >= file.size) {
		report_error(res, "Expected exponent number or sign, but reached end of file");
		return false;
	}

	size_t exp = 0;
	sign = +1;
	if (file[res.cursor] == '-') {
		sign = -1;
		res.cursor += 1;
	}

	if (res.cursor >= file.size) {
		report_error(res, "Expected exponent number after the sign, but reached end of file");
		return false;
	}

	while (take_digit(file, &res.cursor, &digit)) {
		exp = exp * 10 + (digit - '0');
	}

	auto pow10 = [](size_t exp, auto& pow10) {
		if (exp == 0) {
			return 1;
		}
		if (exp == 1) {
			return 10;
		}
		// divide by 2
		size_t half = exp / 2;
		size_t other = exp - half;
		return pow10(half, pow10) * pow10(other, pow10);
	};

	token.kind = Token::Kind::REAL;
	token.text = { file.data + beg, res.cursor - beg };
	res.tokens.push(token);

	if (sign > 0)
		node.number = sign * (integer + frac) * pow10(exp, pow10);
	else
		node.number = sign * (integer + frac) / pow10(exp, pow10);
	out = res.nodes.size;
	res.nodes.push(xstd::move(node));
	return !res.error;
}

bool eat_list(
	parse_express_from_memory_result& res,
	Read_String file,
	eat_f item,
	eat_f sep,
	Node_t out
) {
	if (res.error)
		return false;

	eat_litteral(res, file, "(", Token::Kind::LEFT_PARENTHESIS);
	bool left_on_comma = false;
	Node node(Node::Kind::LIST);
	size_t item_node;
	while (eat_maybe(res, file, item, item_node)) {
		node.list.push(item_node);
		item_node = SIZE_MAX;
		left_on_comma = false;
		if (eat_maybe(res, file, sep, dump_token)) {
			left_on_comma = true;
			continue;
		}
		break;
	}
	eat_litteral(res, file, ")", Token::Kind::RIGHT_PARENTHESIS);
	out = res.nodes.size;
	res.nodes.push(xstd::move(node));
	return !res.error;
}

bool next_upper(Read_String file, size_t* cursor) {
	if (*cursor >= file.size) {
		return false;
	}

	if ((file[*cursor] >= 'A' && file[*cursor] <= 'Z') || file[*cursor] == '_') {
		*cursor += 1;
		return true;
	}

	return false;
}

bool take_digit(Read_String file, size_t* cursor, u8* out) {
	if (*cursor >= file.size) {
		return false;
	}

	if (file[*cursor] >= '0' && file[*cursor] <= '9') {
		*out = file[*cursor];
		*cursor += 1;
		return true;
	}

	return false;
}

bool next_hex(Read_String file, size_t* cursor) {
	if (*cursor >= file.size) {
		return false;
	}

	if (
		(file[*cursor] >= '0' && file[*cursor] <= '9') ||
		(file[*cursor] >= 'A' && file[*cursor] <= 'F')
	) {
		*cursor += 1;
		return true;
	}

	return false;
}

bool next_digit(Read_String file, size_t* cursor) {
	if (*cursor >= file.size) {
		return false;
	}

	if (file[*cursor] >= '0' && file[*cursor] <= '9') {
		*cursor += 1;
		return true;
	}

	return false;
}

bool eat_user_defined_keyword(parse_express_from_memory_result& res, Read_String file, Node_t out)
{
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, "Expected user-defined keyword, but reached end of file");
		return false;
	}

	if (file[res.cursor] != '!') {
		report_error(res, "Expected user-defined keyword, but didn't find an '!'");
		return false;
	}

	Token token;
	token.text.data = file.data + res.cursor;
	size_t beg = res.cursor;

	res.cursor += 1;
	if (!next_upper(file, &res.cursor)) {
		report_error(res, "Expected user-defined keyword, but didn't find an uppercase letter");
		return false;
	}
	while (next_upper(file, &res.cursor) || next_digit(file, &res.cursor));
	
	size_t end = res.cursor;
	token.text.size = end - beg;
	token.kind = Token::Kind::USER_DEFINED_KEYWORD;
	out = res.tokens.size;
	res.tokens.push(token);
	return !res.error;
}

bool eat_standard_keyword(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, "Expected standard keyword, but reached end of file");
		return false;
	}

	if (!(file[res.cursor] >= 'A' && file[res.cursor] <= 'Z')) {
		report_error(res, "Expected standard keyword, but didn't find an uppercase letter");
		return false;
	}

	Token token;
	token.text.data = file.data + res.cursor;
	size_t beg = res.cursor;

	res.cursor += 1;
	while (next_upper(file, &res.cursor) || next_digit(file, &res.cursor));
	
	size_t end = res.cursor;
	token.text.size = end - beg;
	token.kind = Token::Kind::STANDARD_KEYWORD;
	out = res.tokens.size;
	res.tokens.push(token);
	return !res.error;
}

bool eat_omitted_parameter(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;
	out = res.nodes.size;
	Node node(Node::Kind::TOKEN);
	node.token = res.tokens.size;
	res.nodes.push(xstd::move(node));
	eat_litteral(res, file, "*", Token::Kind::OMITTED_PARAMETER);
	return !res.error;
}

bool eat_untyped_parameter(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	res.new_branch();
	if (eat_litteral(res, file, "$", Token::Kind::DOLLAR)) {
		res.commit_branch();
		return true;
	}
	res.pop_branch();
	

	if (eat_maybe(res, file, eat_number, out)) {
		return true;
	}
	
	if (eat_maybe(res, file, eat_string, out)) {
		return true;
	}
	
	if (eat_maybe(res, file, eat_entity_instance_name, out)) {
		return true;
	}
	
	if (eat_maybe(res, file, eat_enumeration, out)) {
		return true;
	}
	
	if (eat_maybe(res, file, eat_binary, out)) {
		return true;
	}
	
	auto eat_comma = [] (parse_express_from_memory_result& res, Read_String file, Node_t) {
		return eat_litteral(res, file, ",", Token::Kind::COMMA);
	};
	if (eat_list(res, file, eat_parameter, eat_comma, out)) {
		return true;
	}
	return !res.error;
}

bool eat_typed_parameter(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	Node node(Node::Kind::TYPED_PARAMETER);
	eat_keyword(res, file, node.typed_parameter.keyword_token);
	eat_litteral(res, file, "(", Token::Kind::LEFT_PARENTHESIS);
	eat_parameter(res, file, node.typed_parameter.parameter);
	eat_litteral(res, file, ")", Token::Kind::RIGHT_PARENTHESIS);

	out = res.nodes.size;
	res.nodes.push(xstd::move(node));
	return !res.error;
}

bool eat_parameter(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;
	
	if (eat_maybe(res, file, eat_omitted_parameter, out)) {
		return !res.error;
	}
	
	if (eat_maybe(res, file, eat_untyped_parameter, out)) {
		return !res.error;
	}
	
	if (eat_maybe(res, file, eat_typed_parameter, out)) {
		return !res.error;
	}
	return !res.error;
}

bool eat_keyword(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;
	eat_whitespace(res, file);

	if (res.cursor >= file.size) {
		report_error(res, "Expected keyword, but reached end of file");
		return false;
	}

	if (file[res.cursor] == '!') {
		eat_user_defined_keyword(res, file, out);
	} else {
		eat_standard_keyword(res, file, out);
	}
	return !res.error;
}

bool eat_header_entity(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	eat_keyword(res, file, dump_token);

	auto eat_comma = [] (parse_express_from_memory_result& res, Read_String file, Node_t) {
		return eat_litteral(res, file, ",", Token::Kind::COMMA);
	};
	eat_list(res, file, eat_parameter, eat_comma, dump_token);
	
	eat_litteral(res, file, ";", Token::Kind::SEMICOLON);
	return !res.error;
}

bool eat_header_section(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	eat_litteral(res, file, "HEADER;", Token::Kind::HEADER);

	eat_header_entity(res, file, dump_token);
	eat_header_entity(res, file, dump_token);
	eat_header_entity(res, file, dump_token);

	while (eat_maybe(res, file, eat_header_entity, dump_token));

	eat_litteral(res, file, "ENDSEC;", Token::Kind::END_SEC);

	return !res.error;
}

bool eat_exchange_file(parse_express_from_memory_result& res, Read_String file, Node_t out) {
	if (res.error)
		return false;

	eat_litteral(res, file, "ISO-10303-21;", Token::Kind::ISO_10303_21);

	eat_header_section(res, file, dump_token);
	eat_data_section(res, file, dump_token);

	eat_litteral(res, file, "END-ISO-10303-21;", Token::Kind::END_ISO_10303_21);
	return !res.error;
}


void parse_express_from_memory(Read_String file, parse_express_from_memory_result& out) {
	auto_release_scratch();

	Node_t root;
	eat_exchange_file(out, file, root);
}

parse_express_from_memory_result parse_express_from_memory(Read_String file)
{
	auto_release_scratch();
	parse_express_from_memory_result res;
	parse_express_from_memory(file, res);
	return res;
}


