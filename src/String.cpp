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
size_t ftoa(float num, char *str, size_t max_len, size_t decimals)
{
	if (max_len <= 0)
		return 0;

	char *ptr = str;
	int remaining = max_len;
	char buf[32];
	int i;

	// Zero (positive or negative zero both become "0")
	if (num == 0.0f) {
		if (remaining > 0) {
			*ptr++ = '0';
			remaining--;
		}
		if (decimals > 0) {
			if (remaining > 0) {
				*ptr++ = '.';
				remaining--;
			}
			for (int d = 0; d < decimals && remaining > 0; ++d) {
				*ptr++ = '0';
				remaining--;
			}
		}
		return max_len - remaining;
	}

	// Sign
	if (num < 0.0f) {
		if (remaining > 0) {
			*ptr++ = '-';
			remaining--;
		}
		num = -num;
	}
	if (remaining <= 0) return max_len - remaining;

	// Integer part
	unsigned long long int_part = (unsigned long long)num;

	i = 0;
	if (int_part == 0) {
		buf[i++] = '0';
	} else {
		while (int_part > 0 && i < 32) {
			buf[i++] = '0' + (int_part % 10);
			int_part /= 10;
		}
	}

	// Write integer digits (most-significant first), respecting remaining space
	while (i > 0 && remaining > 0) {
		*ptr++ = buf[--i];
		remaining--;
	}
	if (remaining <= 0) return max_len - remaining;

	// Fractional part (if requested and space remains)
	if (decimals > 0) {
		if (remaining > 0) {
			*ptr++ = '.';
			remaining--;
		}
		if (remaining <= 0) return max_len - remaining;

		float frac_part = num - (float)(unsigned long long)num;

		for (int d = 0; d < decimals && remaining > 0; ++d) {
			frac_part *= 10.0f;
			int digit = (int)frac_part;
			*ptr++ = '0' + digit;
			remaining--;
			frac_part -= (float)digit;
		}
	}

	return max_len - remaining;
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

extern bool operator==(const Read_String& a, const char* cstr) {
	size_t cstr_len = 0;
	while (cstr[cstr_len] != '\0') {
		cstr_len++;
	}

	if (a.size != cstr_len)
		return false;
	for (size_t i = 0; i < a.size; ++i) {
		if (a[i] != cstr[i])
			return false;
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

size_t to_string(f64 x, Write_String& out) {
	out.size = ftoa((float)x, (char*)out.data, out.size, 6);
	return out.size;
}
size_t to_string(f32 x, Write_String& out) {
	return to_string((f64)x, out);
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