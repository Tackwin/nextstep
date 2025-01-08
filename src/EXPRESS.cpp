#include "EXPRESS.hpp"
#include "Common.hpp"

void report_error(parse_express_from_memory_result& res, Read_String msg) {
	for (size_t i = 0; i < msg.size; ++i) {
		res.diagnostic.push(msg[i]);
	}
	res.error = true;
}

void eat_iso_10303_21(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_semicolon(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_end_iso_10303_21(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_header(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_header_section(parse_express_from_memory_result& res, Read_String file, size_t* cursor);
void eat_whitespace(parse_express_from_memory_result& res, Read_String file, size_t* cursor);

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


