#include <Windows.h>

using u8 = unsigned char;
using u16 = unsigned short;

// defer implementation
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }

#define DEFER_(LINE) _defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__COUNTER__) = defer_dummy{} *[&]()

constexpr size_t g_scratch_buffer_size = 4096 * 1024;
u8 g_scratch_buffer_data[g_scratch_buffer_size];
u8* g_scratch_buffer = g_scratch_buffer_data;

#define auto_release_scratch() auto __start_scratch = g_scratch_buffer; defer { g_scratch_buffer = __start_scratch; }

struct Write_String {
	u8* data = nullptr;
	size_t size = 0;
};
struct Read_String {
	const u8* data = nullptr;
	size_t size = 0;

	u8 operator[](size_t i) const {
		return data[i];
	}
};
struct Write_String16 {
	u16* data = nullptr;
	size_t size = 0;
};
struct Read_String16 {
	const u16* data = nullptr;
	size_t size = 0;
};
template<typename T>
T fromcstr(const u8* str) {
	size_t n = 0;
	while (str[n]) ++n;

	return T{ str, n };
}
template<typename T>
T fromcstr(const u16* str) {
	size_t n = 0;
	while (str[n]) ++n;

	return T{ str, n };
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

// narrow
size_t narrow(Read_String16 in, Write_String out) {
	DWORD result = WideCharToMultiByte(
		CP_UTF8, 0, (LPCWCH)in.data, in.size, (LPSTR)out.data, out.size, nullptr, nullptr
	);
	return result;
}

enum class read_entire_file_into_result {
	success,
	buffer_too_small,
	file_not_found,
	access_denied,
	other_error
};

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

	return { line.data + counter + 1, line.size - counter - 1 };
}

extern "C" void start() {
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	Read_String16 arg_line16 = fromcstr<Read_String16>((u16*)GetCommandLineW());
	Write_String arg_line = { g_scratch_buffer, g_scratch_buffer_size };
	arg_line.size = narrow(arg_line16, arg_line);
	g_scratch_buffer += arg_line.size;
	
	Read_String file = get_file_from_command_line({ arg_line.data, arg_line.size });
	Write_String16 file16 = { (u16*)g_scratch_buffer, g_scratch_buffer_size };
	file16.size = widen(file, file16);
	g_scratch_buffer += file16.size * 2;

	WriteConsoleW(h, L"Opening file: ", 14, nullptr, nullptr);
	WriteConsoleW(h, file16.data, file16.size, nullptr, nullptr);

	Write_String file_contents = { g_scratch_buffer, g_scratch_buffer_size };
	auto result = read_entire_file_into(file, file_contents);
	if (result != read_entire_file_into_result::success) {
		WriteConsoleW(h, L"\nFailed to open file\n", 20, nullptr, nullptr);
		return;
	}

	WriteConsoleW(h, L"\nFile contents:\n", 16, nullptr, nullptr);
	{
		Write_String16 file_contents16 = { (u16*)g_scratch_buffer, g_scratch_buffer_size };
		file_contents16.size = widen({ file_contents.data, arg_line.size }, file_contents16);
		g_scratch_buffer += file_contents16.size * 2;
		WriteConsoleW(h, file_contents16.data, file_contents16.size, nullptr, nullptr);
	}
}
