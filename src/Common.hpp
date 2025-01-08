#pragma once
using u8 = unsigned char;
using u16 = unsigned short;

// defer implementation
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }

#define DEFER_(LINE) _defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__COUNTER__) = defer_dummy{} *[&]()

constexpr size_t g_scratch_buffer_size = 4096 * 1024;
extern u8 g_scratch_buffer_data[];
extern u8* g_scratch_buffer;

#define auto_release_scratch() auto __start_scratch = g_scratch_buffer;\
defer { g_scratch_buffer = __start_scratch; }

struct Write_String {
	u8* data = nullptr;
	size_t size = 0;
};
struct Read_String {
	const u8* data = nullptr;
	size_t size = 0;

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
