#pragma once
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;

constexpr size_t SIZE_MAX = (size_t)-1;

// defer implementation
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }

#define DEFER_(LINE) _defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__COUNTER__) = defer_dummy{} *[&]()

constexpr size_t g_scratch_buffer_size = 128 * 1024 * 1024;
extern u8 g_scratch_buffer_data[];
extern u8* g_scratch_buffer;

#define auto_release_scratch() auto __start_scratch = g_scratch_buffer;\
defer { g_scratch_buffer = __start_scratch; }

extern "C" void* memcpy(void* dst, const void* src, size_t n);