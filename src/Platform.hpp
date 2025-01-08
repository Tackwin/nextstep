#pragma once

#include "Common.hpp"

template<typename T>
T* alloc(size_t n) {
	return (T*)malloc(n * sizeof(T));
}

extern void* malloc(size_t n);
extern void free(void* p);

// =====================

extern void print(Read_String str);

// =====================

enum class read_entire_file_into_result {
	success,
	buffer_too_small,
	file_not_found,
	access_denied,
	other_error
};

[[nodiscard]]
extern read_entire_file_into_result read_entire_file_into(Read_String path, Write_String& file);

extern Read_String get_file_from_command_line(Read_String line);

// =====================
extern size_t widen(Read_String in, Write_String16 out);
extern size_t narrow(Read_String16 in, Write_String out);

extern void get_command_line(Write_String& out);