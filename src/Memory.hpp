#pragma once

#include "Common.hpp"

#include "Templates.hpp"
#include "Platform.hpp"
#include "String.hpp"

template<typename T>
T* alloc(size_t n);
template<typename T>
T* alloc();
template<typename T>
T* talloc(size_t n);
extern void* malloc(size_t n);
extern void free(void* p);

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
		data = nullptr;
		size = 0;
		capacity = 0;
	}

	DynArray(const DynArray& other) {
		*this = other;
	}

	DynArray& operator=(const DynArray& other) {
		if (this == &other) return *this;

		reserve(other.size);
		for (size_t i = 0; i < other.size; ++i) {
			data[i] = xstd::move(other.data[i]);
		}
		size = other.size;

		return *this;
	}

	T* push(T t) {
		if (size >= capacity) {
			size_t new_capacity = capacity * 2 + 1;
			if (new_capacity * sizeof(T) < 1024*4) {
				new_capacity = (1024*4 + sizeof(T) - 1) / sizeof(T);
			}
			reserve(capacity * 2 + 1);
		}

		data[size] = xstd::move(t);
		size += 1;

		return data + size - 1;
	}

	void reserve(size_t n) {
		if (n <= capacity) return;

		auto new_data = alloc<T>(n);
		if (data) {
			for (size_t i = 0; i < size; ++i) {
				new_data[i] = xstd::move(data[i]);
			}
			free(data);
		}
		data = new_data;
		capacity = n;
	}

	void clear() {
		for (size_t i = 0; i < size; ++i) {
			data[i].~T();
		}
		size = 0;
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
		data = nullptr;
		size = 0;
		capacity = 0;
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
		*this = move(other);
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
		if (size < N) {
			capacity = size + 1;
		}
		else if (size >= capacity) {
			reserve(capacity * 2 + 1);
		}

		if (size < N) {
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
			} else {
				for (size_t i = 0; i < size; ++i) {
					new_data[i] = sso[i];
				}
			}
			data = new_data;
		}
		capacity = n;
	}
};

template<typename T, size_t N>
struct Append_Only_Stable_Array {
	struct Bucket {
		T data[N];
		size_t size = 0;
		Bucket* next = nullptr;
	};

	Bucket* first = nullptr;

	Append_Only_Stable_Array() = default;
	Append_Only_Stable_Array(const Append_Only_Stable_Array& other) {
		*this = other;
	}
	Append_Only_Stable_Array& operator=(const Append_Only_Stable_Array& other) {
		if (this == &other) return *this;

		this->~Append_Only_Stable_Array();

		auto other_bucket = other.first;
		Bucket** bucket = &first;
		while (other_bucket) {
			*bucket = alloc<Bucket>();
			(*bucket)->size = other_bucket->size;
			for (size_t i = 0; i < other_bucket->size; ++i) {
				(*bucket)->data[i] = other_bucket->data[i];
			}
			bucket = &(*bucket)->next;
			other_bucket = other_bucket->next;
		}

		return *this;
	}
	Append_Only_Stable_Array(Append_Only_Stable_Array&& other) {
		*this = move(other);
	}
	Append_Only_Stable_Array& operator=(Append_Only_Stable_Array&& other) {
		if (this == &other) return *this;

		this->~Append_Only_Stable_Array();

		first = other.first;
		other.first = nullptr;

		return *this;
	}
	~Append_Only_Stable_Array() {
		while (first) {
			auto next = first->next;
			free(first);
			first = next;
		}
		first = nullptr;
	}

	T* push(T t) {
		if (!first) {
			first = alloc<Bucket>();
		}

		Bucket* last = first;
		while (last->next) {
			last = last->next;
		}

		if (last->size >= N) {
			last->next = alloc<Bucket>();
			last = last->next;
		}

		last->data[last->size] = t;
		last->size += 1;
		return last->data + last->size - 1;
	}

	T& operator[](size_t i) {
		Bucket* bucket = first;
		while (bucket) {
			if (i < bucket->size) {
				return bucket->data[i];
			}
			i -= bucket->size;
			bucket = bucket->next;
		}
		assert(false);
		return first->data[0];
	}

	const T& operator[](size_t i) const {
		Bucket* bucket = first;
		while (bucket) {
			if (i < bucket->size) {
				return bucket->data[i];
			}
			i -= bucket->size;
			bucket = bucket->next;
		}
		assert(false);
		return first->data[0];
	}
};

struct Stable_Arena {
	struct Bucket {
		u8* data = nullptr;
		size_t size = 0;
		size_t capacity = 0;
		Bucket* next = nullptr;
	};

	Bucket* first = nullptr;
	size_t bucket_size = 1024 * 1024;

	Stable_Arena() = default;
	Stable_Arena(size_t bucket_size) : bucket_size(bucket_size) {
	}

	Stable_Arena(const Stable_Arena& other) {
		*this = other;
	}
	Stable_Arena& operator=(const Stable_Arena& other) {
		if (this == &other) return *this;

		this->~Stable_Arena();

		auto other_bucket = other.first;
		Bucket** bucket = &first;
		while (other_bucket) {
			*bucket = alloc<Bucket>(1);
			(*bucket)->size = other_bucket->size;
			(*bucket)->capacity = other_bucket->capacity;
			(*bucket)->data = ::alloc<u8>(other_bucket->capacity);
			memcpy((*bucket)->data, other_bucket->data, other_bucket->size);
			bucket = &(*bucket)->next;
			other_bucket = other_bucket->next;
		}

		return *this;
	}

	Stable_Arena(Stable_Arena&& other) {
		*this = xstd::move(other);
	}
	Stable_Arena& operator=(Stable_Arena&& other) {
		if (this == &other) return *this;

		this->~Stable_Arena();

		first = other.first;
		other.first = nullptr;

		return *this;
	}
	~Stable_Arena() {
		while (first) {
			auto next = first->next;
			free(first->data);
			free(first);
			first = next;
		}
		first = nullptr;
	}

	void* alloc(size_t n, size_t align) {
		if (!first) {
			first = ::alloc<Bucket>();
			first->data = ::alloc<u8>(bucket_size + align - 1);
			first->capacity = bucket_size;
		}

		Bucket* last = first;
		while (last->next) {
			last = last->next;
		}

		if (last->size + (n + align - 1) > last->capacity) {
			last->next = ::alloc<Bucket>();
			last = last->next;
			last->data = ::alloc<u8>(bucket_size + align - 1);
			last->capacity = bucket_size;
		}

		void* ptr = last->data + last->size;
		ptr = (void*)((size_t(ptr) + align - 1) & ~(align - 1));
		last->size = (size_t(ptr) - size_t(last->data)) + n;
		return ptr;
	}

	template<typename T>
	T* alloc(size_t n = 1) {
		T* ptr = (T*)alloc(n * sizeof(T), alignof(T));
		for (size_t i = 0; i < n; ++i) {
			new (ptr + i) T();
		}
		return ptr;
	}

	template<typename T>
	T* copy(T t) {
		T* ptr = (T*)alloc(sizeof(T), alignof(T));
		new (ptr) T(xstd::move(t));
		return ptr;
	}

	template<typename T>
	T* take(T&& t) {
		T* ptr = (T*)alloc(sizeof(T), alignof(T));
		new (ptr) T(xstd::move(t));
		return ptr;
	}
};

template<typename T>
struct Fixed_Array {
	T* data = nullptr;
	size_t size = 0;

	Fixed_Array() = default;
	Fixed_Array(size_t n) {
		data = alloc<T>(n);
		size = n;
	}
	~Fixed_Array() {
		free(data);
		data = nullptr;
		size = 0;
	}

	Fixed_Array(const Fixed_Array& other) {
		*this = other;
	}
	Fixed_Array& operator=(const Fixed_Array& other) {
		if (this == &other) return *this;

		if (data) free(data);
		data = alloc<T>(other.size);
		size = other.size;
		for (size_t i = 0; i < size; ++i) {
			data[i] = other.data[i];
		}

		return *this;
	}

	Fixed_Array(Fixed_Array&& other) {
		*this = xstd::move(other);
	}
	Fixed_Array& operator=(Fixed_Array&& other) {
		if (this == &other) return *this;

		if (data) free(data);
		data = other.data;
		size = other.size;
		other.data = nullptr;
		other.size = 0;

		return *this;
	}

	T& operator[](size_t i) {
		return data[i];
	}
	const T& operator[](size_t i) const {
		return data[i];
	}
};