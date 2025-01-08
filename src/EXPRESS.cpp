#include "EXPRESS.hpp"
#include "Common.hpp"

void report_error(parse_express_from_memory_result& res, Read_String msg) {
	for (size_t i = 0; i < msg.size; ++i) {
		res.diagnostic.push(msg[i]);
	}
	res.error = true;
}

using eat_f = void(parse_express_from_memory_result&, Read_String, size_t*);
void eat_iso_10303_21(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_semicolon(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_end_iso_10303_21(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_header(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_header_section(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_whitespace(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_header_entity(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_left_parenthesis(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_right_parenthesis(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_keyword(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_user_defined_keyword(
	parse_express_from_memory_result& res, Read_String file, size_t* cursor
);
void eat_standard_keyword(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_list(parse_express_from_memory_result& res, Read_String file, size_t* cursor, eat_f f);
void eat_parameter(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_omitted_parameter(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_untyped_parameter(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_typed_parameter(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
bool try_comma(parse_express_from_memory_result& res, Read_String file, size_t* cursor);

bool next_upper(Read_String file, size_t* cursor);
bool next_digit(Read_String file, size_t* cursor);

void eat_left_parenthesis(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected left parenthesis, but reached end of file"));
		return;
	}

	if (begins_with_at(file, fromcstr<Read_String>("("), *cursor)) {
		Token token;
		token.kind = Token::Kind::LEFT_PARENTHESIS;
		token.text = { file.data + *cursor, 1 };
		res.tokens.push(token);

		*cursor += 1;
	} else {
		report_error(res, fromcstr<Read_String>("Expected left parenthesis"));
	}
}

void eat_right_parenthesis(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size) {
		Read_String str = fromcstr<Read_String>(
			"Expected right parenthesis, but reached end of file"
		);
		report_error(res, str);
		return;
	}

	if (begins_with_at(file, fromcstr<Read_String>(")"), *cursor)) {
		Token token;
		token.kind = Token::Kind::LEFT_PARENTHESIS;
		token.text = { file.data + *cursor, 1 };
		res.tokens.push(token);

		*cursor += 1;
	} else {
		report_error(res, fromcstr<Read_String>("Expected right parenthesis"));
	}
}

void eat_whitespace(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	while (*cursor < file.size && (
		file[*cursor] == ' ' ||
		file[*cursor] == '\t' ||
		file[*cursor] == '\n' ||
		file[*cursor] == '\r'
	)) {
		*cursor += 1;
	}
}

void eat_semicolon(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected semicolon, but reached end of file"));
		return;
	}

	if (begins_with_at(file, fromcstr<Read_String>(";"), *cursor)) {
		Token token;
		token.kind = Token::Kind::SEMICOLON;
		token.text = { file.data + *cursor, 1 };
		res.tokens.push(token);

		*cursor += 1;
	} else {
		report_error(res, fromcstr<Read_String>("Expected semicolon"));
	}
}

void eat_iso_10303_21(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	if (begins_with_at(file, fromcstr<Read_String>("ISO-10303-21;"), *cursor)) {
		Token token;
		token.kind = Token::Kind::ISO_10303_21;
		token.text = { file.data + *cursor, 12 };
		res.tokens.push(token);

		*cursor += 12;
	} else {
		report_error(res, fromcstr<Read_String>("Expected ISO-10303-21;"));
	}
}

void eat_end_iso_10303_21(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	if (begins_with_at(file, fromcstr<Read_String>("END-ISO-10303-21;"), *cursor)) {
		Token token;
		token.kind = Token::Kind::END_ISO_10303_21;
		token.text = { file.data + *cursor, 16 };
		res.tokens.push(token);

		*cursor += 16;
	} else {
		report_error(res, fromcstr<Read_String>("Expected END-ISO-10303-21;"));
	}
}

void eat_header(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	if (begins_with_at(file, fromcstr<Read_String>("HEADER"), *cursor)) {
		Token token;
		token.kind = Token::Kind::HEADER;
		token.text = { file.data + *cursor, 6 };
		res.tokens.push(token);

		*cursor += 6;
	} else {
		report_error(res, fromcstr<Read_String>("Expected HEADER"));
	}
}

bool try_comma(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size)
		return false;

	if (file[*cursor] == ',') {
		Token token;
		token.kind = Token::Kind::COMMA;
		token.text = { file.data + *cursor, 1 };
		res.tokens.push(token);

		*cursor += 1;

		return true;
	}
	return false;
}

void eat_list(parse_express_from_memory_result& res, Read_String file, size_t* cursor, eat_f f) {
	eat_whitespace(res, file, cursor);

	f(res, file, cursor);
	if (res.error)
		return;
	
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size)
		return;

	while (try_comma(res, file, cursor)) {
		eat_list(res, file, cursor, f);
		if (res.error)
			return;
	}

	return;
}

bool next_upper(Read_String file, size_t* cursor) {
	if (*cursor >= file.size) {
		return false;
	}

	if (file[*cursor] >= 'A' && file[*cursor] <= 'Z') {
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

void eat_user_defined_keyword(
	parse_express_from_memory_result& res, Read_String file, size_t* cursor
) {
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected user-defined keyword, but reached end of file")
		);
		return;
	}

	if (file[*cursor] != '!') {
		report_error(
			res, fromcstr<Read_String>("Expected user-defined keyword, but didn't find an '!'")
		);
		return;
	}

	Token token;
	token.user_defined_keyword.data = file.data + *cursor;
	size_t beg = *cursor;

	*cursor += 1;
	if (!next_upper(file, cursor)) {
		report_error(res, fromcstr<Read_String>(
			"Expected user-defined keyword, but didn't find an uppercase letter"
		));
		return;
	}
	while (next_upper(file, cursor) || next_digit(file, cursor));
	
	size_t end = *cursor;
	token.user_defined_keyword.size = end - beg;
	token.kind = Token::Kind::USER_DEFINED_KEYWORD;
	res.tokens.push(token);
}

void eat_standard_keyword(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected standard keyword, but reached end of file")
		);
		return;
	}

	if (!(file[*cursor] >= 'A' && file[*cursor] <= 'Z')) {
		report_error(res, fromcstr<Read_String>(
			"Expected standard keyword, but didn't find an uppercase letter"
		));
		return;
	}

	Token token;
	token.standard_keyword.data = file.data + *cursor;
	size_t beg = *cursor;

	*cursor += 1;
	while (next_upper(file, cursor) || next_digit(file, cursor));
	
	size_t end = *cursor;
	token.standard_keyword.size = end - beg;
	token.kind = Token::Kind::USER_DEFINED_KEYWORD;
	res.tokens.push(token);
}

void eat_omitted_parameter(parse_express_from_memory_result& res, Read_String file, size_t* cursor)
{
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected omitted parameter, but reached end of file")
		);
		return;
	}

	if (file[*cursor] != '*') {
		report_error(
			res, fromcstr<Read_String>("Expected omitted parameter, but didn't find an '*'")
		);
		return;
	}

	Token token;
	token.kind = Token::Kind::OMITTED_PARAMETER;
	token.text = { file.data + *cursor, 1 };
	res.tokens.push(token);

	*cursor += 1;
}

void eat_untyped_parameter(parse_express_from_memory_result& res, Read_String file, size_t* cursor)
{
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size) {
		report_error(
			res, fromcstr<Read_String>("Expected untyped parameter, but reached end of file")
		);
		return;
	}

	if (file[*cursor] != '$') {
		report_error(
			res, fromcstr<Read_String>("Expected untyped parameter, but didn't find an '$'")
		);
		return;
	}
	*cursor += 1;

	

	// >TODO
}

void eat_typed_parameter(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);

	eat_keyword(res, file, cursor);
	if (res.error)
		return;

	eat_left_parenthesis(res, file, cursor);
	if (res.error)
		return;

	eat_parameter(res, file, cursor);
	if (res.error)
		return;

	eat_right_parenthesis(res, file, cursor);
	if (res.error)
		return;
}

void eat_parameter(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	if (*cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected parameter, but reached end of file"));
		return;
	}

	if (file[*cursor] == '*') {
		eat_omitted_parameter(res, file, cursor);
		if (res.error)
			return;
	} else if (file[*cursor] == '$') {
		eat_untyped_parameter(res, file, cursor);
		if (res.error)
			return;
	} else {
		eat_typed_parameter(res, file, cursor);
		if (res.error)
			return;
	}
}

void eat_keyword(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);

	if (*cursor >= file.size) {
		report_error(res, fromcstr<Read_String>("Expected keyword, but reached end of file"));
		return;
	}
	if (file[*cursor] == '!') {
		eat_user_defined_keyword(res, file, cursor);
		if (res.error)
			return;
	} else {
		eat_standard_keyword(res, file, cursor);
		if (res.error)
			return;
	}
}

void eat_header_entity(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);

	eat_keyword(res, file, cursor);
	if (res.error)
		return;

	eat_left_parenthesis(res, file, cursor);
	if (res.error)
		return;

	eat_list(res, file, cursor, eat_parameter);
	if (res.error)
		return;

	eat_right_parenthesis(res, file, cursor);
	if (res.error)
		return;
	
	eat_semicolon(res, file, cursor);
	if (res.error)
		return;
}

void eat_header_section(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	eat_header(res, file, cursor);
	if (res.error)
		return;

	eat_semicolon(res, file, cursor);
	if (res.error)
		return;

	eat_header_entity(res, file, cursor);
}

void eat_exchange_file(parse_express_from_memory_result& res, Read_String file, size_t* cursor) {
	eat_whitespace(res, file, cursor);
	eat_iso_10303_21(res, file, cursor);
	if (res.error)
		return;

	eat_semicolon(res, file, cursor);
	if (res.error)
		return;

	eat_header_section(res, file, cursor);
	if (res.error)
		return;
		
	// TODO: Implement

	eat_end_iso_10303_21(res, file, cursor);
	if (res.error)
		return;
	
	eat_semicolon(res, file, cursor);
	if (res.error)
		return;
}

parse_express_from_memory_result parse_express_from_memory(Read_String file)
{
	auto_release_scratch();

	parse_express_from_memory_result res;

	size_t cursor = 0;
	eat_exchange_file(res, file, &cursor);
	if (res.error)
		return res;

	return {};
}


