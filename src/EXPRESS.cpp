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
bool eat_iso_10303_21(parse_express_from_memory_result& res, Read_String file);
bool eat_semicolon(parse_express_from_memory_result& res, Read_String file);
bool eat_end_iso_10303_21(parse_express_from_memory_result& res, Read_String file);
bool eat_header(parse_express_from_memory_result& res, Read_String file);
bool eat_header_section(parse_express_from_memory_result& res, Read_String file);
bool eat_whitespace(parse_express_from_memory_result& res, Read_String file);
bool eat_header_entity(parse_express_from_memory_result& res, Read_String file);
bool eat_left_parenthesis(parse_express_from_memory_result& res, Read_String file);
bool eat_right_parenthesis(parse_express_from_memory_result& res, Read_String file);
bool eat_keyword(parse_express_from_memory_result& res, Read_String file);
bool eat_user_defined_keyword(
	parse_express_from_memory_result& res, Read_String file
);
bool eat_standard_keyword(parse_express_from_memory_result& res, Read_String file);
bool eat_list(parse_express_from_memory_result& res, Read_String file, eat_f f);
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

bool next_upper(Read_String file, size_t* cursor);
bool next_digit(Read_String file, size_t* cursor);
bool next_hex(Read_String file, size_t* cursor);
bool take_digit(Read_String file, size_t* cursor, u8* out);


bool eat_left_parenthesis(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected left parenthesis, but reached end of file"));
		return false;
	}

	if (begins_with_at(file, fromcstr<Read_String>("("), res.cursor)) {
		Token token;
		token.kind = Token::Kind::LEFT_PARENTHESIS;
		token.text = { file.data + res.cursor, 1 };
		res.tokens.push(token);

		res.cursor += 1;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected left parenthesis"));
	}
	return !res.error;
}

bool eat_right_parenthesis(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		Read_String str = fromcstr<Read_String>(
			"Expected right parenthesis, but reached end of file"
		);
		report_error(res, str);
		return false;
	}

	if (begins_with_at(file, fromcstr<Read_String>(")"), res.cursor)) {
		Token token;
		token.kind = Token::Kind::LEFT_PARENTHESIS;
		token.text = { file.data + res.cursor, 1 };
		res.tokens.push(token);

		res.cursor += 1;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected right parenthesis"));
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

bool eat_semicolon(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected semicolon, but reached end of file"));
		return false;
	}

	if (begins_with_at(file, fromcstr<Read_String>(";"), res.cursor)) {
		Token token;
		token.kind = Token::Kind::SEMICOLON;
		token.text = { file.data + res.cursor, 1 };
		res.tokens.push(token);

		res.cursor += 1;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected semicolon"));
	}
	return !res.error;
}

bool eat_iso_10303_21(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (begins_with_at(file, fromcstr<Read_String>("ISO-10303-21;"), res.cursor)) {
		Token token;
		token.kind = Token::Kind::ISO_10303_21;
		token.text = { file.data + res.cursor, 12 };
		res.tokens.push(token);

		res.cursor += 12;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected ISO-10303-21;"));
	}

	return !res.error;
}

bool eat_end_iso_10303_21(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;
	eat_whitespace(res, file);
	if (begins_with_at(file, fromcstr<Read_String>("END-ISO-10303-21;"), res.cursor)) {
		Token token;
		token.kind = Token::Kind::END_ISO_10303_21;
		token.text = { file.data + res.cursor, 16 };
		res.tokens.push(token);

		res.cursor += 16;
		return true;
	} else {
		report_error(res, fromcstr<Read_String>("Expected END-ISO-10303-21;"));
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
		if (exp == 0) {
			return 1;
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

bool eat_list(parse_express_from_memory_result& res, Read_String file, eat_f f) {
	if (res.error)
		return false;
	eat_whitespace(res, file);

	eat_left_parenthesis(res, file);

	f(res, file);
	
	eat_whitespace(res, file);
	if (res.cursor >= file.size)
		return false;

	while (true) {
		res.new_branch();
		if (eat_comma(res, file)) {
			res.commit_branch();
			f(res, file);
			if (res.error)
				break;
		} else {
			res.pop_branch();
			break;
		}
	}

	eat_right_parenthesis(res, file);

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
	eat_whitespace(res, file);
	if (res.cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected omitted parameter, but reached end of file")
		);
		return false;
	}

	if (file[res.cursor] != '*') {
		report_error(
			res, fromcstr<Read_String>("Expected omitted parameter, but didn't find an '*'")
		);
		return false;
	}

	Token token;
	token.kind = Token::Kind::OMITTED_PARAMETER;
	token.text = { file.data + res.cursor, 1 };
	res.tokens.push(token);

	res.cursor += 1;
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

	
	if (file[res.cursor] == '$') {
		Token token;
		token.kind = Token::Kind::DOLLAR;
		token.text = { file.data + res.cursor, 1 };
		res.cursor += 1;
		return true;
	}
	
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
	
	if (eat_list(res, file, eat_parameter)) {
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

	eat_left_parenthesis(res, file);

	eat_parameter(res, file);

	eat_right_parenthesis(res, file);
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
	eat_whitespace(res, file);

	eat_keyword(res, file);

	eat_list(res, file, eat_parameter);
	
	eat_semicolon(res, file);
	return !res.error;
}

bool eat_header_section(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_whitespace(res, file);
	eat_header(res, file);

	eat_semicolon(res, file);

	eat_header_entity(res, file);
	return !res.error;
}

bool eat_exchange_file(parse_express_from_memory_result& res, Read_String file) {
	if (res.error)
		return false;

	eat_whitespace(res, file);
	eat_iso_10303_21(res, file);

	eat_semicolon(res, file);

	eat_header_section(res, file);
		
	// TODO: Implement

	eat_end_iso_10303_21(res, file);
	
	eat_semicolon(res, file);
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


