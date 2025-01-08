#include "Platform.hpp"
#include <Windows.h>

void* malloc(size_t n) {
	return VirtualAlloc(nullptr, n, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}
void free(void* p) {
	VirtualFree(p, 0, MEM_RELEASE);
}

void print(Read_String str)
{
	auto_release_scratch();
	Write_String16 str16 = { (u16*)g_scratch_buffer, g_scratch_buffer_size };
	str16.size = widen(str, str16);
	g_scratch_buffer += str16.size * 2;

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleW(h, str16.data, str16.size, nullptr, nullptr);
}

[[nodiscard]]
read_entire_file_into_result read_entire_file_into(Read_String path, Write_String& file) {
	auto_release_scratch();

	size_t path16n = widen(path, { (u16*)g_scratch_buffer, g_scratch_buffer_size });
	Read_String16 path16 = { (u16*)g_scratch_buffer, path16n };
	g_scratch_buffer += path16n * 2;

	HANDLE h = CreateFileW(
		(LPCWSTR)path16.data, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
	);
	if (h == INVALID_HANDLE_VALUE) {
		size_t err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND)
			return read_entire_file_into_result::file_not_found;
		if (err == ERROR_PATH_NOT_FOUND)
			return read_entire_file_into_result::file_not_found;
		if (err == ERROR_ACCESS_DENIED)
			return read_entire_file_into_result::access_denied;
		return read_entire_file_into_result::other_error;
	}
	defer {
		CloseHandle(h);
	};

	LARGE_INTEGER size;
	GetFileSizeEx(h, &size);

	if (size.QuadPart > file.size)
		return read_entire_file_into_result::buffer_too_small;

	DWORD read;
	ReadFile(h, file.data, (DWORD)size.QuadPart, &read, nullptr);

	if (read != size.QuadPart)
		return read_entire_file_into_result::other_error;

	file.size = read;

	return read_entire_file_into_result::success;
}

Read_String get_file_from_command_line(Read_String line) {
	size_t quote_counter = 0;
	size_t counter = 0;
	while(counter < line.size && !((quote_counter % 2) == 0 && line[counter] == ' ')) {
		if (line[counter] == '"' && (counter == 0 || line[counter - 1] != '\\'))
			quote_counter += 1;
		counter += 1;
	}

	while (counter < line.size && line[counter] == ' ')
		counter += 1;

	return { line.data + counter, line.size - counter };
}

void get_command_line(Write_String& out) {
	Read_String16 arg_line16 = fromcstr<Read_String16>((u16*)GetCommandLineW());
	out.size = narrow(arg_line16, out);
}

size_t widen(Read_String in, Write_String16 out) {
	DWORD result = MultiByteToWideChar(
		CP_UTF8, 0, (LPCCH)in.data, in.size, (LPWSTR)out.data, out.size
	);
	if (result == 0) {
		size_t err = GetLastError();
		switch (err) {
			case ERROR_INSUFFICIENT_BUFFER:
				WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"Insufficient buffer\n", 21, nullptr, nullptr);
				break;
			case ERROR_INVALID_FLAGS:
				WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"Invalid flags\n", 15, nullptr, nullptr);
				break;
			case ERROR_INVALID_PARAMETER:
				WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"Invalid parameter\n", 18, nullptr, nullptr);
				break;
			case ERROR_NO_UNICODE_TRANSLATION:
				WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"No unicode translation\n", 23, nullptr, nullptr);
				break;
		}
		return 0;
	}

	result += 1;
	result -= 1;
	return result;
}
size_t narrow(Read_String16 in, Write_String out) {
	DWORD result = WideCharToMultiByte(
		CP_UTF8, 0, (LPCWCH)in.data, in.size, (LPSTR)out.data, out.size, nullptr, nullptr
	);
	return result;
}