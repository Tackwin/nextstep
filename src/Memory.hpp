#pragma once

#include "Common.hpp"

#include "Platform.hpp"
#include "String.hpp"

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

template<typename T, size_t N>
struct SSO_Array {
	T* data = nullptr;
	size_t size = 0;
	size_t capacity = 0;
	T sso[N];

	T& operator[](size_t i) {
		return data ? data[i] : sso[i];
	}
	const T& operator[](size_t i) const {
		return data ? data[i] : sso[i];
	}

	SSO_Array() = default;
	SSO_Array(size_t n) {
		reserve(n);
	}
	~SSO_Array() {
		if (data) free(data);
	}

	SSO_Array(const SSO_Array& other) {
		*this = other;
	}
	SSO_Array& operator=(const SSO_Array& other) {
		if (this == &other) return *this;

		reserve(other.size);

		if (other.size <= N) {
			for (size_t i = 0; i < other.size; ++i) {
				sso[i] = other.sso[i];
			}
		} else {
			for (size_t i = 0; i < other.size; ++i) {
				data[i] = other.data[i];
			}
		}
		size = other.size;

		return *this;
	}

	SSO_Array(SSO_Array&& other) {
		*this = (SSO_Array&&)other;
	}
	SSO_Array& operator=(SSO_Array&& other) {
		if (this == &other) return *this;

		if (data) free(data);

		data = other.data;
		size = other.size;
		capacity = other.capacity;

		if (size <= N) {
			for (size_t i = 0; i < size; ++i) {
				sso[i] = other.sso[i];
			}
		} else {
			other.data = nullptr;
		}

		other.size = 0;
		other.capacity = 0;

		return *this;
	}

	T* push(T t) {
		if (size >= capacity) {
			reserve(capacity * 2 + 1);
		}

		if (size <= N) {
			sso[size] = t;
		} else {
			data[size] = t;
		}
		size += 1;

		return data + size - 1;
	}

	void reserve(size_t n) {
		if (n <= capacity) return;

		if (n >= N) {
			auto new_data = alloc<T>(n);
			if (data) {
				for (size_t i = 0; i < size; ++i) {
					new_data[i] = data[i];
				}
				free(data);
			}
			data = new_data;
		}
		capacity = n;
	}
};