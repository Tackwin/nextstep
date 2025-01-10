#include "EXPRESS.hpp"
#include "Common.hpp"

extern "C" {
	int _fltused = 0;
}

void report_error(parse_express_from_memory_result& res, Read_String msg) {
	for (size_t i = 0; i < msg.size; ++i) {
		res.diagnostic.push(msg[i]);
	}
	res.error = true;
}

using eat_f = bool(parse_express_from_memory_result&, Read_String);
bool eat_empty(parse_express_from_memory_result& res, Read_String file) {
	return !res.error;
}

bool macro_list(
	parse_express_from_memory_result& res, Read_String file, eat_f item, eat_f sep, bool non_empty
);
bool macro_LITTERAL(
	parse_express_from_memory_result& res, Read_String file, Read_String litteral, Token::Kind kind
);
bool eat_header(parse_express_from_memory_result& res, Read_String file);
bool eat_header_section(parse_express_from_memory_result& res, Read_String file);
bool eat_whitespace(parse_express_from_memory_result& res, Read_String file);
bool eat_header_entity(parse_express_from_memory_result& res, Read_String file);
bool eat_keyword(parse_express_from_memory_result& res, Read_String file);
bool eat_user_defined_keyword(
	parse_express_from_memory_result& res, Read_String file
);
bool eat_standard_keyword(parse_express_from_memory_result& res, Read_String file);
bool eat_parameter(parse_express_from_memory_result& res, Read_String file);
bool eat_omitted_parameter(parse_express_from_memory_result& res, Read_String file);
bool eat_untyped_parameter(parse_express_from_memory_result& res, Read_String file);
bool eat_typed_parameter(parse_express_from_memory_result& res, Read_String file);
bool eat_comma(parse_express_from_memory_result& res, Read_String file);
bool eat_number(parse_express_from_memory_result& res, Read_String file);
bool eat_string(parse_express_from_memory_result& res, Read_String file);
bool eat_entity_instance_name(
	parse_express_from_memory_result& res, Read_String file
);
bool eat_enumeration(parse_express_from_memory_result& res, Read_String file);
bool eat_binary(parse_express_from_memory_result& res, Read_String file);
bool eat_endsec(parse_express_from_memory_result& res, Read_String file);
bool eat_data(parse_express_from_memory_result& res, Read_String file);
bool eat_data_section(parse_express_from_memory_result& res, Read_String file);
bool eat_entity_instance_list(parse_express_from_memory_result& res, Read_String file);
bool eat_entity_instance(parse_express_from_memory_result& res, Read_String file);
bool eat_simple_entity_instance(parse_express_from_memory_result& res, Read_String file);
bool eat_complex_entity_instance(parse_express_from_memory_result& res, Read_String file);
bool eat_scope(parse_express_from_memory_result& res, Read_String file);
bool eat_ampersand(parse_express_from_memory_result& res, Read_String file);
bool eat_SCOPE(parse_express_from_memory_result& res, Read_String file);
bool eat_endscope(parse_express_from_memory_result& res, Read_String file);
bool eat_export_list(parse_express_from_memory_result& res, Read_String file);
bool eat_forward_slash(parse_express_from_memory_result& res, Read_String file);

bool next_upper(Read_String file, size_t* cursor);
bool next_digit(Read_String file, size_t* cursor);
bool next_hex(Read_String file, size_t* cursor);
bool take_digit(Read_String file, size_t* cursor, u8* out);

bool macro_LITTERAL(
	parse_express_from_memory_result& res, Read_String file, Read_String litteral, Token::Kind kind
) {
	if (res.error)
		return false;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected litteral, but reached end of file"));
		report_error(res, fromcstr<Read_String>("Expected litteral: "));
		report_error(res, litteral);
		return false;
	}

	if (begins_with_at(file, litteral, res.cursor)) {
		Token token;
		token.kind = kind;
		token.text = { file.data + res.cursor, litteral.size };
		res.tokens.push(token);
		res.cursor += litteral.size;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected litteral: "));
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
bool eat_header(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (begins_with_at(file, fromcstr<Read_String>("HEADER"), res.cursor)) {
		Token token;
		token.kind = Token::Kind::HEADER;
		token.text = { file.data + res.cursor, 6 };
		res.tokens.push(token);

		res.cursor += 6;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected HEADER"));
	}
	return !res.error;
}

bool eat_equal(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected '=', but reached end of file"));
		return false;
	}

	if (file[res.cursor] != '=') {
		report_error(res, fromcstr<Read_String>("Expected '='"));
		return false;
	}

	Token token;
	token.kind = Token::Kind::EQUAL;
	token.text = { file.data + res.cursor, 1 };
	res.tokens.push(token);

	res.cursor += 1;
	return true;

}

bool eat_ampersand(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected '&', but reached end of file"));
		return false;
	}

	if (file[res.cursor] != '&') {
		report_error(res, fromcstr<Read_String>("Expected '&'"));
		return false;
	}

	Token token;
	token.kind = Token::Kind::AMPERSAND;
	token.text = { file.data + res.cursor, 1 };
	res.tokens.push(token);

	res.cursor += 1;
	return true;
}

bool eat_SCOPE(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (begins_with_at(file, fromcstr<Read_String>("SCOPE"), res.cursor)) {
		Token token;
		token.kind = Token::Kind::SCOPE;
		token.text = { file.data + res.cursor, 5 };
		res.tokens.push(token);

		res.cursor += 5;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected SCOPE"));
	}
	return !res.error;
}

bool eat_forward_slash(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected '/', but reached end of file"));
		return false;
	}

	if (file[res.cursor] != '/') {
		report_error(res, fromcstr<Read_String>("Expected '/'"));
		return false;
	}

	Token token;
	token.kind = Token::Kind::FORWARD_SLASH;
	token.text = { file.data + res.cursor, 1 };
	res.tokens.push(token);

	res.cursor += 1;
	return true;
}

bool eat_export_list(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);

	eat_forward_slash(res, file);
	eat_entity_instance_name(res, file);

	res.new_branch();
	while (eat_comma(res, file)) {
		res.commit_branch();

		eat_entity_instance_name(res, file);
		res.new_branch();
	}
	res.pop_branch();

	eat_forward_slash(res, file);

	return !res.error;
}

bool eat_endscope(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (begins_with_at(file, fromcstr<Read_String>("ENDSCOPE"), res.cursor)) {
		Token token;
		token.kind = Token::Kind::ENDSCOPE;
		token.text = { file.data + res.cursor, 8 };
		res.tokens.push(token);

		res.cursor += 8;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected ENDSCOPE"));
	}
	return !res.error;
}

bool eat_scope(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_ampersand(res, file);
	eat_SCOPE(res, file);
	eat_entity_instance_list(res, file);
	eat_endscope(res, file);

	res.new_branch();
	if (eat_export_list(res, file)) {
		res.commit_branch();
	} else {
		res.pop_branch();
	}

	return !res.error;
}

bool eat_simple_record(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_keyword(res, file);
	macro_list(res, file, eat_parameter, eat_comma, false);
	return !res.error;
}

bool eat_simple_entity_instance(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_entity_instance_name(res, file);
	if (res.error)
		return false;

	eat_equal(res, file);

	res.new_branch();
	if (eat_scope(res, file)) {
		res.commit_branch();
	} else {
		res.pop_branch();
	}

	eat_simple_record(res, file);

	macro_LITTERAL(res, file, ";", Token::Kind::SEMICOLON);

	return !res.error;
}

bool eat_subsuper_record(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	macro_list(res, file, eat_simple_record, eat_empty, false);
	return !res.error;
}

bool eat_complex_entity_instance(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_entity_instance_name(res, file);
	eat_equal(res, file);
	res.new_branch();
	if (eat_scope(res, file)) {
		res.commit_branch();
	} else {
		res.pop_branch();
	}

	eat_subsuper_record(res, file);
	macro_LITTERAL(res, file, ";", Token::Kind::SEMICOLON);

	return !res.error;
}

bool eat_entity_instance_name(
	parse_express_from_memory_result& res, Read_String file
) {
	if (res.error)
		return false;
	Token token;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected entity instance name, but reached end of file")
		);
		return false;
	}
	if (file[res.cursor] != '#') {
		report_error(
			res, fromcstr<Read_String>("Expected entity instance name, but didn't find a '#'")
		);
		return false;
	}

	size_t beg = res.cursor;
	res.cursor += 1;
	while (next_digit(file, &res.cursor));

	size_t end = res.cursor;
	token.kind = Token::Kind::ENTITY_INSTANCE_NAME;
	token.user_defined_keyword = { file.data + beg, end - beg };
	res.tokens.push(token);
	return !res.error;
}

bool eat_binary(parse_express_from_memory_result& res, Read_String file)
{
	if (res.error)
		return false;
	Token token;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected list, but reached end of file"));
		return false;
	}

	if (file[res.cursor] != '"') {
		report_error(res, fromcstr<Read_String>("Expected list, but didn't find a '('"));
		return false;
	}
	res.cursor += 1;
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected list, but reached end of file"));
		return false;
	}

	if (
		file[res.cursor] != '0' && file[res.cursor] != '1' && file[res.cursor] != '2' && file[res.cursor] != '3'
	) {
		report_error(res, fromcstr<Read_String>("Expected list, but didn't find a '('"));
		return false;
	}
	
	size_t beg = res.cursor;
	res.cursor += 1;

	while (next_hex(file, &res.cursor));

	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected list, but reached end of file"));
		return false;
	}

	if (file[res.cursor] != '"') {
		report_error(res, fromcstr<Read_String>("Expected list, but didn't find a '('"));
		return false;
	}
	res.cursor += 1;

	size_t end = res.cursor;
	token.kind = Token::Kind::BINARY;
	token.text = { file.data + beg, end - beg };
	res.tokens.push(token);
	return !res.error;
}

bool eat_endsec(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (begins_with_at(file, fromcstr<Read_String>("ENDSEC"), res.cursor)) {
		Token token;
		token.kind = Token::Kind::END_SEC;
		token.text = { file.data + res.cursor, 6 };
		res.tokens.push(token);

		res.cursor += 6;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected ENDSEC"));
	}
	return !res.error;
}

bool eat_data(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (begins_with_at(file, fromcstr<Read_String>("DATA"), res.cursor)) {
		Token token;
		token.kind = Token::Kind::DATA;
		token.text = { file.data + res.cursor, 4 };
		res.tokens.push(token);

		res.cursor += 4;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected DATA"));
	}

	return !res.error;
}

bool eat_entity_instance(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);

	res.new_branch();
	if (eat_simple_entity_instance(res, file)) {
		res.commit_branch();
		return true;
	}

	res.pop_branch();
	res.new_branch();
	if (eat_complex_entity_instance(res, file)) {
		res.commit_branch();
		return true;
	}

	res.pop_branch();

	report_error(
		res, fromcstr<Read_String>("Expected entity instance, but found neither simple nor complex")
	);

	return !res.error;
}

bool eat_entity_instance_list(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);

	eat_entity_instance(res, file);
	if (res.error)
		return false;
	
	res.new_branch();
	while (eat_entity_instance(res, file)) {
		res.commit_branch();
		res.new_branch();
	}
	res.pop_branch();

	return !res.error;
}

bool eat_data_section(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_data(res, file);
	macro_LITTERAL(res, file, ";", Token::Kind::SEMICOLON);
	eat_entity_instance_list(res, file);
	eat_endsec(res, file);
	macro_LITTERAL(res, file, ";", Token::Kind::SEMICOLON);

	return !res.error;
}

bool eat_enumeration(parse_express_from_memory_result& res, Read_String file)
{
	if (res.error)
		return false;

	Token token;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected enumeration, but reached end of file"));
		return false;
	}

	if (file[res.cursor] != '.') {
		report_error(res, fromcstr<Read_String>("Expected enumeration, but didn't find a '.'"));
		return false;
	}

	size_t beg = res.cursor;
	res.cursor += 1;

	if (!next_upper(file, &res.cursor)) {
		report_error(
			res, fromcstr<Read_String>("Expected enumeration, but didn't find an uppercase letter")
		);
		return false;
	}

	while (next_upper(file, &res.cursor) || next_digit(file, &res.cursor));

	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected enumeration, but reached end of file"));
		return false;
	}

	if (file[res.cursor] != '.') {
		report_error(res, fromcstr<Read_String>("Expected enumeration, but didn't find a '.'"));
		return false;
	}

	size_t end = res.cursor;
	res.cursor += 1;

	token.kind = Token::Kind::USER_DEFINED_KEYWORD;
	token.user_defined_keyword = { file.data + beg, end - beg };
	res.tokens.push(token);
	return !res.error;
}

bool eat_string(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	Token token;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected string, but reached end of file"));
		return false;
	}

	if (file[res.cursor] != '\'') {
		report_error(res, fromcstr<Read_String>("Expected string, but didn't find a single quote"));
		return false;
	}

	size_t beg = res.cursor;
	res.cursor += 1;

	while (
		res.cursor < file.size &&
		(file[res.cursor] != '\'' && !(res.cursor > 0 && file[res.cursor - 1] == '\\'))
	) {
		res.cursor += 1;
	}

	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected string, but reached end of file"));
		return false;
	}

	if (file[res.cursor] != '\'') {
		report_error(
			res, fromcstr<Read_String>("Expected end of string, but didn't find a single quote")
		);
		return false;
	}
	res.cursor += 1;

	size_t end = res.cursor;
	token.kind = Token::Kind::STRING;
	token.text = { file.data + beg, end - beg };
	res.tokens.push(token);
	return !res.error;
}

bool eat_number(parse_express_from_memory_result& res, Read_String file)
{
	if (res.error)
		return false;
	Token token;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected number, but reached end of file"));
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
		report_error(res, fromcstr<Read_String>("Expected number, but didn't find a digit"));
		return false;
	}

	if (res.cursor >= file.size) {
		token.kind = Token::Kind::INTEGER;
		token.number = sign * integer;
		token.text = { file.data + beg, res.cursor - beg };
		res.tokens.push(token);
		return true;
	}

	if (file[res.cursor] == '.') {
		res.cursor += 1;
	} else {
		token.kind = Token::Kind::INTEGER;
		token.number = sign * integer;
		token.text = { file.data + beg, res.cursor - beg };
		res.tokens.push(token);
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
		token.number = sign * (integer + frac);
		token.text = { file.data + beg, res.cursor - beg };
		res.tokens.push(token);
		return true;
	}

	if (file[res.cursor] == 'E' || file[res.cursor] == 'e') {
		res.cursor += 1;
	} else {
		token.kind = Token::Kind::REAL;
		token.number = sign * (integer + frac);
		token.text = { file.data + beg, res.cursor - beg };
		res.tokens.push(token);
		return true;
	}

	if (res.cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected exponent number or sign, but reached end of file")
		);
		return false;
	}

	size_t exp = 0;
	sign = +1;
	if (file[res.cursor] == '-') {
		sign = -1;
		res.cursor += 1;
	}

	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>(
			"Expected exponent number after the sign, but reached end of file"
		));
		return false;
	}

	while (take_digit(file, &res.cursor, &digit)) {
		exp = exp * 10 + (digit - '0');
	}

	auto pow10 = [](size_t exp, auto& pow10) {
		if (exp == 1) {
			return 10;
		}
		// divide by 2
		size_t half = exp / 2;
		size_t other = exp - half;
		return pow10(half, pow10) * pow10(other, pow10);
	};

	token.kind = Token::Kind::REAL;
	token.number = sign * (integer + frac) * pow10(exp, pow10);
	token.text = { file.data + beg, res.cursor - beg };
	res.tokens.push(token);
	return !res.error;
}

bool eat_comma(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size)
		return false;

	if (file[res.cursor] != ',') {
		report_error(res, fromcstr<Read_String>("Expected comma"));
		return false;
	}

	Token token;
	token.kind = Token::Kind::COMMA;
	token.text = { file.data + res.cursor, 1 };
	res.tokens.push(token);

	res.cursor += 1;

	return true;
}

bool macro_list(
	parse_express_from_memory_result& res, Read_String file, eat_f item, eat_f sep, bool non_empty
) {
	if (res.error)
		return false;
	eat_whitespace(res, file);

	macro_LITTERAL(res, file, "(", Token::Kind::LEFT_PARENTHESIS);

	if (non_empty)
	{
		item(res, file);
		
		while (true) {
			res.new_branch();
			if (sep(res, file)) {
				res.commit_branch();
				item(res, file);
				if (res.error)
					break;
			} else {
				res.pop_branch();
				break;
			}
		}
	} else
	{
		while (true)
		{
			res.new_branch();
			if (item(res, file)) {
				res.commit_branch();

				res.new_branch();
				if (sep(res, file)) {
					res.commit_branch();
					continue;
				} else {
					res.pop_branch();
					break;
				}

			} else {
				res.pop_branch();
				break;
			}
		}
	}

	macro_LITTERAL(res, file, ")", Token::Kind::RIGHT_PARENTHESIS);

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

bool eat_user_defined_keyword(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected user-defined keyword, but reached end of file")
		);
		return false;
	}

	if (file[res.cursor] != '!') {
		report_error(
			res, fromcstr<Read_String>("Expected user-defined keyword, but didn't find an '!'")
		);
		return false;
	}

	Token token;
	token.user_defined_keyword.data = file.data + res.cursor;
	size_t beg = res.cursor;

	res.cursor += 1;
	if (!next_upper(file, &res.cursor)) {
		report_error(res, fromcstr<Read_String>(
			"Expected user-defined keyword, but didn't find an uppercase letter"
		));
		return false;
	}
	while (next_upper(file, &res.cursor) || next_digit(file, &res.cursor));
	
	size_t end = res.cursor;
	token.user_defined_keyword.size = end - beg;
	token.kind = Token::Kind::USER_DEFINED_KEYWORD;
	res.tokens.push(token);
	return !res.error;
}

bool eat_standard_keyword(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected standard keyword, but reached end of file")
		);
		return false;
	}

	if (!(file[res.cursor] >= 'A' && file[res.cursor] <= 'Z')) {
		report_error(res, fromcstr<Read_String>(
			"Expected standard keyword, but didn't find an uppercase letter"
		));
		return false;
	}

	Token token;
	token.standard_keyword.data = file.data + res.cursor;
	size_t beg = res.cursor;

	res.cursor += 1;
	while (next_upper(file, &res.cursor) || next_digit(file, &res.cursor));
	
	size_t end = res.cursor;
	token.standard_keyword.size = end - beg;
	token.kind = Token::Kind::USER_DEFINED_KEYWORD;
	res.tokens.push(token);
	return !res.error;
}

bool eat_omitted_parameter(parse_express_from_memory_result& res, Read_String file)
{
	if (res.error)
		return false;
	macro_LITTERAL(res, file, "*", Token::Kind::OMITTED_PARAMETER);
	return !res.error;
}

bool eat_untyped_parameter(
	parse_express_from_memory_result& res, Read_String file)
{
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected untyped parameter, but reached end of file")
		);
		return false;
	}

	res.new_branch();
	if (macro_LITTERAL(res, file, "$", Token::Kind::DOLLAR)) {
		res.commit_branch();
		return true;
	}
	
	res.pop_branch();
	res.new_branch();

	if (eat_number(res, file)) {
		res.commit_branch();
		return true;
	}

	res.pop_branch();
	res.new_branch();
	
	if (eat_string(res, file)) {
		res.commit_branch();
		return true;
	}
	
	res.pop_branch();
	res.new_branch();

	if (eat_entity_instance_name(res, file)) {
		res.commit_branch();
		return true;
	}
	
	res.pop_branch();
	res.new_branch();
	
	if (eat_enumeration(res, file)) {
		res.commit_branch();
		return true;
	}
	
	res.pop_branch();
	res.new_branch();
	
	if (eat_binary(res, file)) {
		res.commit_branch();
		return true;
	}
	
	res.pop_branch();
	res.new_branch();
	
	if (macro_list(res, file, eat_parameter, eat_comma, false)) {
		res.commit_branch();
		return true;
	}
	return !res.error;
}

bool eat_typed_parameter(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_whitespace(res, file);
	eat_keyword(res, file);
	macro_LITTERAL(res, file, "(", Token::Kind::LEFT_PARENTHESIS);
	eat_parameter(res, file);
	macro_LITTERAL(res, file, ")", Token::Kind::RIGHT_PARENTHESIS);

	return !res.error;
}

bool eat_parameter(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected parameter, but reached end of file"));
		return false;
	}

	
	if (file[res.cursor] == '*') {
		eat_omitted_parameter(res, file);
		return !res.error;
	}
	
	res.new_branch();

	if (eat_untyped_parameter(res, file)) {
		res.commit_branch();
		return !res.error;
	}
	
	res.pop_branch();
	res.new_branch();

	if (eat_typed_parameter(res, file)) {
		res.commit_branch();
		return !res.error;
	}
	return !res.error;
}

bool eat_keyword(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);

	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected keyword, but reached end of file"));
		return false;
	}
	if (file[res.cursor] == '!') {
		eat_user_defined_keyword(res, file);
	} else {
		eat_standard_keyword(res, file);
	}
	return !res.error;
}

bool eat_header_entity(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_keyword(res, file);

	macro_list(res, file, eat_parameter, eat_comma, false);
	
	macro_LITTERAL(res, file, ";", Token::Kind::SEMICOLON);
	return !res.error;
}

bool eat_header_section(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_header(res, file);

	macro_LITTERAL(res, file, ";", Token::Kind::SEMICOLON);

	eat_header_entity(res, file);
	eat_header_entity(res, file);
	eat_header_entity(res, file);

	while (true)
	{
		res.new_branch();
		if (eat_header_entity(res, file))
		{
			res.commit_branch();
		}
		else
		{
			res.pop_branch();
			break;
		}
	}

	eat_endsec(res, file);
	macro_LITTERAL(res, file, ";", Token::Kind::SEMICOLON);

	return !res.error;
}

bool eat_exchange_file(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	macro_LITTERAL(res, file, "ISO-10303-21", Token::Kind::ISO_10303_21);
	macro_LITTERAL(res, file, ";", Token::Kind::SEMICOLON);

	eat_header_section(res, file);
	eat_data_section(res, file);

	macro_LITTERAL(res, file, "END-ISO-10303-21", Token::Kind::END_ISO_10303_21);
	macro_LITTERAL(res, file, ";", Token::Kind::SEMICOLON);
	return !res.error;
}

parse_express_from_memory_result parse_express_from_memory(Read_String file)
{
	auto_release_scratch();

	parse_express_from_memory_result res;

	eat_exchange_file(res, file);
	if (res.error)
		return res;

	return {};
}


