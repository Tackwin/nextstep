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

