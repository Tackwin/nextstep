#include "String.hpp"
#include "Platform.hpp"

Owned_String::Owned_String(u8* data, size_t size) : data(data), size(size) {}
Owned_String::Owned_String(Owned_String&& other) {
	*this = static_cast<Owned_String&&>(other);
}
Owned_String& Owned_String::operator=(Owned_String&& other) {
	if (this != &other) {
		data = other.data;
		size = other.size;
		other.data = nullptr;
		other.size = 0;
	}
	return *this;
}
Owned_String::Owned_String(const Owned_String& other) {
	*this = other;
}
Owned_String& Owned_String::operator=(const Owned_String& other) {
	if (this != &other) {
		if (data) {
			free(data);
		}
		data = alloc<u8>(other.size);
		size = other.size;
		for (size_t i = 0; i < size; ++i) {
			data[i] = other.data[i];
		}
	}
	return *this;
}
Owned_String::~Owned_String() {
	if (data) {
		free(data);
	}
	data = nullptr;
}

Owned_String::Owned_String(Read_String str) {
	data = alloc<u8>(str.size);
	size = str.size;
	for (size_t i = 0; i < size; ++i) {
		data[i] = str[i];
	}
}
Owned_String::Owned_String(Write_String str) {
	data = alloc<u8>(str.size);
	size = str.size;
	for (size_t i = 0; i < size; ++i) {
		data[i] = str.data[i];
	}
}

Owned_String::operator Read_String() const {
	return { data, size };
}
Owned_String::operator Write_String() const {
	return { data, size };
}


bool strcomp(Read_String a, Read_String b) {
	if (a.size != b.size) return false;
	for (size_t i = 0; i < a.size; ++i) {
		if (a[i] != b[i]) return false;
	}
	return true;
}

bool begins_with(Read_String a, Read_String b) {
	if (a.size < b.size) return false;
	for (size_t i = 0; i < b.size; ++i) {
		if (a[i] != b[i]) return false;
	}
	return true;
}

bool begins_with_at(Read_String a, Read_String b, size_t at) {
	if (a.size - at < b.size) return false;
	for (size_t i = 0; i < b.size; ++i) {
		if (a[at + i] != b[i]) return false;
	}
	return true;
}

bool operator==(const Read_String& a, const Read_String& b) {
	if (a.size != b.size)
		return false;
	for (size_t i = 0; i < a.size; ++i) {
		if (a[i] != b[i])
			return false;
	}
	return true;
}

size_t to_string(size_t n, Write_String& out) {
	if (n == 0 && out.size > 0) {
		out.data[0] = '0';
		out.size = 1;
		return 1;
	}
	// in reverse
	size_t cursor = 0;
	while (n > 0 && cursor < out.size) {
		out.data[cursor++] = '0' + (n % 10);
		n /= 10;
	}
	// reverse
	for (size_t i = 0; i < cursor / 2; ++i) {
		u8 tmp = out.data[i];
		out.data[i] = out.data[cursor - i - 1];
		out.data[cursor - i - 1] = tmp;
	}
	out.size = cursor;
	return cursor;
}

size_t to_string(i64 n, Write_String& out) {
	if (n < 0) {
		out.data[0] = '-';
		Write_String tmp = { out.data + 1, out.size - 1 };
		size_t wrote = to_string((size_t)(-n), tmp);
		out.size = wrote + 1;
		return wrote + 1;
	}
	return to_string((size_t)n, out);
}

size_t parse_size_t(Read_String str) {
	size_t n = 0;
	for (size_t i = 0; i < str.size; ++i) {
		if (str[i] < '0' || str[i] > '9') {
			return n;
		}
		n = n * 10 + (str[i] - '0');
	}
	return n;
}

bool case_insensitive_compare(Read_String a, Read_String b) {
	if (a.size != b.size)
		return false;
	for (size_t i = 0; i < a.size; ++i) {
		if (a[i] != b[i] && (a[i] | 0x20) != (b[i] | 0x20))
			return false;
	}
	return true;
}
size_t case_insenstive_hash(Read_String str) {
	size_t h = 0;
	for (size_t i = 0; i < str.size; ++i) {
		h = h * 31 + (str[i] | 0x20);
	}
	return h;
}