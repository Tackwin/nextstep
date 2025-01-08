#include "Memory.hpp"

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

