#pragma once

#include "Common.hpp"

#include "Platform.hpp"

template<typename T>
struct DynArray {
	T* data = nullptr;
	size_t size = 0;
	size_t capacity = 0;

	T& operator[](size_t i) {
		return data[i];
	}
	const T& operator[](size_t i) const {
		return data[i];
	}

	DynArray() = default;
	DynArray(size_t n) {
		reserve(n);
	}
	~DynArray() {
		free(data);
	}

	DynArray(const DynArray& other) {
		*this = other;
	}

	DynArray& operator=(const DynArray& other) {
		if (this == &other) return *this;

		reserve(other.size);
		for (size_t i = 0; i < other.size; ++i) {
			data[i] = other.data[i];
		}
		size = other.size;

		return *this;
	}

	T* push(T t) {
		if (size >= capacity) {
			reserve(capacity * 2 + 1);
		}

		data[size] = t;
		size += 1;

		return data + size - 1;
	}

	void reserve(size_t n) {
		if (n <= capacity) return;

		auto new_data = alloc<T>(n);
		if (data) {
			for (size_t i = 0; i < size; ++i) {
				new_data[i] = data[i];
			}
			free(data);
		}
		data = new_data;
		capacity = n;
	}
};

extern bool strcomp(Read_String a, Read_String b);
extern bool begins_with(Read_String a, Read_String b);
extern bool begins_with_at(Read_String a, Read_String b, size_t at);