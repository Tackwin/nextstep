#pragma once

#include "Common.hpp"
#include "Memory.hpp"
#include "String.hpp"

template<typename T>
struct DynArray;

template<typename T>
T* alloc(size_t n) {
	return (T*)malloc(n * sizeof(T));
}
template<typename T>
T* talloc(size_t n) {
	T* ptr = (T*)g_scratch_buffer;
	g_scratch_buffer += n * sizeof(T);
	return ptr;
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


extern size_t get_file_size(Read_String path);
[[nodiscard]]
extern read_entire_file_into_result read_entire_file_into(Read_String path, Write_String& file);
extern bool is_file(Read_String file);
extern bool is_directory(Read_String file);
extern void list_files_in_directory(Read_String path, DynArray<Owned_String>& out);
extern void recursive_list_files_in_directory(Read_String path, DynArray<Owned_String>& out);
extern size_t canonize(Read_String path, Write_String& out);

extern Read_String get_file_from_command_line(Read_String line);

// =====================
extern size_t widen(Read_String in, Write_String16 out);
extern size_t narrow(Read_String16 in, Write_String out);

extern void get_command_line(Write_String& out);