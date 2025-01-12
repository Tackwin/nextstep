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

bool is_file(Read_String path) {
	auto_release_scratch();

	size_t path16n = widen(path, { (u16*)g_scratch_buffer, g_scratch_buffer_size });
	Read_String16 path16 = { (u16*)g_scratch_buffer, path16n };
	g_scratch_buffer += path16n * 2;

	DWORD attr = GetFileAttributesW((LPCWSTR)path16.data);
	if (attr == INVALID_FILE_ATTRIBUTES)
		return false;
	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		return false;
	return true;
}

bool is_directory(Read_String path) {
	auto_release_scratch();

	size_t path16n = widen(path, { (u16*)g_scratch_buffer, g_scratch_buffer_size });
	Read_String16 path16 = { (u16*)g_scratch_buffer, path16n };
	g_scratch_buffer += path16n * 2;

	DWORD attr = GetFileAttributesW((LPCWSTR)path16.data);
	if (attr == INVALID_FILE_ATTRIBUTES)
		return false;
	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		return true;
	return false;
}

size_t canonize(Read_String path, Write_String& out) {
	// >TODO
	// This is weirdge and buggy
	auto_release_scratch();

	size_t path16n = widen(path, { (u16*)g_scratch_buffer, g_scratch_buffer_size });
	Read_String16 path16 = { (u16*)g_scratch_buffer, path16n };
	g_scratch_buffer += path16n * 2;

	u16 buffer[MAX_PATH];
	DWORD result = GetFullPathNameW((LPCWSTR)path16.data, MAX_PATH, (wchar_t*)buffer, nullptr);
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

	Write_String16 buffer16 = { buffer, result };
	out.size = narrow({ buffer16.data, buffer16.size }, out);
	return out.size;
}

void list_files_in_directory(Read_String path, DynArray<Owned_String>& out) {
	auto_release_scratch();

	size_t path16n = widen({ path.data, path.size }, { (u16*)g_scratch_buffer, g_scratch_buffer_size });
	Read_String16 path16 = { (u16*)g_scratch_buffer, path16n };
	g_scratch_buffer += path16n * 2 + 2;

	wchar_t* buffer = talloc<wchar_t>(path16.size + 1);
	for (size_t i = 0; i < path16.size; ++i)
		buffer[i] = path16.data[i];
	buffer[path16.size] = 0;

	WIN32_FIND_DATAW data = {};
	HANDLE h = FindFirstFileW((LPCWSTR)path16.data, &data);
	if (h == INVALID_HANDLE_VALUE) {
		size_t err = GetLastError();
		switch (err) {
			case ERROR_FILE_NOT_FOUND:
				return;
			case ERROR_PATH_NOT_FOUND:
				return;
			case ERROR_ACCESS_DENIED:
				return;
		}
		return;
	}
	defer {
		FindClose(h);
	};

	do {
		Read_String16 name16 = fromcstr<Read_String16>((u16*)data.cFileName);
		Write_String name = { g_scratch_buffer, g_scratch_buffer_size };
		name.size = narrow(name16, name);
		g_scratch_buffer += name.size;
		out.push((Owned_String)name);
	} while (FindNextFileW(h, &data));
}

void recursive_list_files_in_directory(Read_String path, DynArray<Owned_String>& out) {
	auto_release_scratch();

	size_t path16n = widen({ path.data, path.size }, { (u16*)g_scratch_buffer, g_scratch_buffer_size });
	Read_String16 path16 = { (u16*)g_scratch_buffer, path16n };
	g_scratch_buffer += path16n * 2 + 2;

	wchar_t* buffer = talloc<wchar_t>(path16.size + 3);
	for (size_t i = 0; i < path16.size; ++i)
		buffer[i] = path16.data[i];
	buffer[path16.size] = '\\';
	buffer[path16.size + 1] = '*';
	buffer[path16.size + 2] = 0;

	WIN32_FIND_DATAW data = {};
	HANDLE h = FindFirstFileW(buffer, &data);
	if (h == INVALID_HANDLE_VALUE) {
		size_t err = GetLastError();
		switch (err) {
			case ERROR_FILE_NOT_FOUND:
				return;
			case ERROR_PATH_NOT_FOUND:
				return;
			case ERROR_ACCESS_DENIED:
				return;
		}
		return;
	}
	defer {
		FindClose(h);
	};

	do {
		Read_String16 name16 = fromcstr<Read_String16>((u16*)data.cFileName);
		Write_String name = { g_scratch_buffer, g_scratch_buffer_size };
		name.size = narrow(name16, name);
		g_scratch_buffer += name.size;

		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (name.size == 1 && name.data[0] == '.')
				continue;
			if (name.size == 2 && name.data[0] == '.' && name.data[1] == '.')
				continue;

			Write_String16 subpath16 = { (u16*)g_scratch_buffer, g_scratch_buffer_size };
			subpath16.size = name16.size + path16.size + 1;
			for (size_t i = 0; i < path16.size; ++i)
				subpath16.data[i] = path16.data[i];
			subpath16.data[path16.size] = '\\';
			for (size_t i = 0; i < name16.size; ++i)
				subpath16.data[i + path16.size + 1] = name16.data[i];
			g_scratch_buffer += subpath16.size * 2;

			Write_String subpath = { g_scratch_buffer, g_scratch_buffer_size };
			subpath.size = narrow({ subpath16.data, subpath16.size }, subpath);
			g_scratch_buffer += subpath.size;

			recursive_list_files_in_directory({ subpath.data, subpath.size }, out);
		} else {
			Write_String fullname = { g_scratch_buffer, g_scratch_buffer_size };
			fullname.size = 0;
			for (size_t i = 0; i < path.size; i += 1)
				fullname.data[fullname.size++] = path.data[i];
			fullname.data[fullname.size++] = '\\';
			for (size_t i = 0; i < name.size; i += 1)
				fullname.data[fullname.size++] = name.data[i];
			g_scratch_buffer += fullname.size;

			out.push((Owned_String)fullname);
		}
	} while (FindNextFileW(h, &data));
}

size_t get_file_size(Read_String path) {
	auto_release_scratch();

	size_t path16n = widen(path, { (u16*)g_scratch_buffer, g_scratch_buffer_size });
	Read_String16 path16 = { (u16*)g_scratch_buffer, path16n };
	g_scratch_buffer += path16n * 2;

	wchar_t* buffer = talloc<wchar_t>(path16.size + 1);
	for (size_t i = 0; i < path16.size; ++i)
		buffer[i] = path16.data[i];
	buffer[path16.size] = 0;

	HANDLE h = CreateFileW(
		buffer, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
	);
	if (h == INVALID_HANDLE_VALUE) {
		size_t err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND)
			return 0;
		if (err == ERROR_PATH_NOT_FOUND)
			return 0;
		if (err == ERROR_ACCESS_DENIED)
			return 0;
		return 0;
	}
	defer {
		CloseHandle(h);
	};

	LARGE_INTEGER size;
	GetFileSizeEx(h, &size);

	return size.QuadPart;
}


[[nodiscard]]
read_entire_file_into_result read_entire_file_into(Read_String path, Write_String& file) {
	auto_release_scratch();

	size_t path16n = widen(path, { (u16*)g_scratch_buffer, g_scratch_buffer_size });
	Read_String16 path16 = { (u16*)g_scratch_buffer, path16n };
	g_scratch_buffer += path16n * 2;

	wchar_t* buffer = talloc<wchar_t>(path16.size + 1);
	for (size_t i = 0; i < path16.size; ++i)
		buffer[i] = path16.data[i];
	buffer[path16.size] = 0;

	HANDLE h = CreateFileW(
		buffer, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
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
	if (in.size == 0) {
		out.size = 0;
		return 0;
	}
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