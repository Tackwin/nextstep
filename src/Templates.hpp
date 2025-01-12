#pragma once


namespace xstd {
	// remove reference
	template<typename T>
	struct remove_reference { using type = T; };

	template<typename T>
	struct remove_reference<T&> { using type = T; };

	template<typename T>
	struct remove_reference<T&&> { using type = T; };

	template<typename T>
	using remove_reference_t = remove_reference<T>::type;

	template<typename T>
	remove_reference_t<T>&& move(T&& t) {
		return (remove_reference_t<T>&&)t;
	}

	// optional
	struct nullopt_t {
	};
	constexpr nullopt_t nullopt;

	template <typename T>
	struct optional {
		T value;
		bool has_value = false;

		optional() = default;
		~optional() {
			if (has_value)
				value.~T();
		}
		optional(nullopt_t) {
		}
		optional(const T& value) : has_value(true), value(value) {
		}
		optional(T&& value) : has_value(true), value(xstd::move(value)) {
		}
		optional(const optional& other) {
			*this = other;
		}
		optional(optional&& other) {
			*this = xstd::move(other);
		}
		optional& operator=(const optional& other) {
			if (this == &other)
				return *this;
			has_value = other.has_value;
			value = other.value;
			return *this;
		}
		optional& operator=(optional&& other) {
			if (this == &other)
				return *this;
			has_value = other.has_value;
			value = std::move(other.value);
			return *this;
		}
		optional& operator=(const T& value) {
			has_value = true;
			this->value = value;
			return *this;
		}
		optional& operator=(T&& value) {
			has_value = true;
			this->value = xstd::move(value);
			return *this;
		}

		operator bool() const {
			return has_value;
		}

		T& operator*() {
			return value;
		}
		const T& operator*() const {
			return value;
		}

		T* operator->() {
			return &value;
		}
		const T* operator->() const {
			return &value;
		}

		void reset() {
			has_value = false;
		}

		operator T&() {
			return value;
		}
		operator const T&() const {
			return value;
		}
	};

}