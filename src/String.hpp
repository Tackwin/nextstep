#pragma once

#include "Common.hpp"

template<typename T>
T fromcstr(const char* str) {
	return fromcstr<T>((const u8*)str);
}

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

struct Write_String {
	u8* data = nullptr;
	size_t size = 0;
};
struct Read_String {
	const u8* data = nullptr;
	size_t size = 0;

	Read_String() = default;
	Read_String(const u8* data, size_t size) : data(data), size(size) {}
	// Read_String(const char* cstr) {
	// 	*this = fromcstr<Read_String>(cstr);
	// }
	template<size_t N>
	Read_String(const char (&cstr) [N]) {
		data = (const u8*)cstr;
		size = N - 1;
	}

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

struct Owned_String {
	u8* data = nullptr;
	size_t size = 0;

	Owned_String() = default;
	Owned_String(u8* data, size_t size);
	Owned_String(Owned_String&& other);
	Owned_String& operator=(Owned_String&& other);
	Owned_String(const Owned_String& other);
	Owned_String& operator=(const Owned_String& other);
	~Owned_String();

	explicit Owned_String(Read_String str);
	explicit Owned_String(Write_String str);

	explicit operator Read_String() const;
	explicit operator Write_String() const;
};

extern bool strcomp(Read_String a, Read_String b);
extern bool begins_with(Read_String a, Read_String b);
extern bool begins_with_at(Read_String a, Read_String b, size_t at);
template<size_t N>
bool begins_with(Read_String a, const char (&b) [N]) {
	for (size_t i = 0; i < N - 1; ++i) {
		if (a.size <= i || a[i] != b[i])
			return false;
	}
	return true;
}
template<size_t N>
bool begins_with_at(Read_String a, const char (&b) [N], size_t at) {
	for (size_t i = 0; i < N - 1; ++i) {
		if (a.size <= at + i || a[at + i] != b[i])
			return false;
	}
	return true;
}
extern bool operator==(const Read_String& a, const Read_String& b);
template<size_t N>
extern bool operator==(const Read_String& a, const char (&cstr)[N]) {
	if (a.size != N - 1)
		return false;
	for (size_t i = 0; i < N - 1; ++i) {
		if (a[i] != cstr[i])
			return false;
	}
	return true;
}

extern size_t to_string(size_t n, Write_String& out);
extern size_t to_string(i64 n, Write_String& out);
extern size_t parse_size_t(Read_String str);