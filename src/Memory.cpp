#include "Memory.hpp"

void append(DynArray<u8>& out, Read_String str) {
	for (size_t i = 0; i < str.size; i += 1) {
		out.push(str.data[i]);
	}
}
