#include "Common.hpp"

u8 g_scratch_buffer_data[g_scratch_buffer_size];
u8* g_scratch_buffer = g_scratch_buffer_data;

extern "C" void memcpy(void* dst, const void* src, size_t n) {
	for (size_t i = 0; i < n; ++i) {
		((u8*)dst)[i] = ((const u8*)src)[i];
	}
}

extern "C" void memset(void* dst, u8 value, size_t n) {
	for (size_t i = 0; i < n; ++i) {
		((u8*)dst)[i] = value;
	}
}
