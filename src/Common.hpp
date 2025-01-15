#pragma once
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using i64 = signed long long;

constexpr size_t SIZE_MAX = (size_t)-1;

// defer implementation
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }

#define DEFER_(LINE) _defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__COUNTER__) = defer_dummy{} *[&]()
#define offsetof(a,b) ((int)(&(((a*)(0))->b)))

constexpr size_t g_scratch_buffer_size = 512 * 1024 * 1024;
extern u8 g_scratch_buffer_data[];
extern u8* g_scratch_buffer;

#define auto_release_scratch() auto __start_scratch = g_scratch_buffer;\
defer { g_scratch_buffer = __start_scratch; }

extern "C" void* memcpy(void* dst, const void* src, size_t n);
extern "C" void* memset(void* dst, int value, size_t n);

template<size_t N>
constexpr size_t hash(const char (&str)[N]) {
	size_t h = 0;
	for (size_t i = 0; i < N - 1; ++i) {
		h = h * 31 + str[i];
	}
	return h;
}
template<size_t N>
constexpr size_t case_insenstive_hash(const char (&str)[N]) {
	size_t h = 0;
	for (size_t i = 0; i < N - 1; ++i) {
		h = h * 31 + (str[i] | 0x20);
	}
	return h;
}

template<typename T>
using ref = T*;

// placement new implementation for NODEFAULTLIB
extern "C" void* operator new(size_t, void* p);
