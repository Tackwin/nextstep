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

	// is_same
	template<typename T, typename U>
	struct is_same {
		static constexpr bool value = false;
	};
	template<typename T>
	struct is_same<T, T> {
		static constexpr bool value = true;
	};
	template<typename T, typename U>
	constexpr bool is_same_v = is_same<T, U>::value;

	// conditional
	template<bool b, typename T, typename U>
	struct conditional {
		using type = T;
	};
	template<typename T, typename U>
	struct conditional<false, T, U> {
		using type = U;
	};
	template<bool b, typename T, typename U>
	using conditional_t = typename conditional<b, T, U>::type;

	template<typename ... Ts>
	struct Template_Pack {};

	template<typename Add, typename Pack>
	struct Add_To_Pack;

	template<typename Add, typename ... Pack>
	struct Add_To_Pack<Add, Template_Pack<Pack...>> {
		using type = Template_Pack<Add, Pack...>;
	};

	// concat two packs
	template<typename A, typename B>
	struct Concat_Packs;
	template<typename ... A, typename ... B>
	struct Concat_Packs<Template_Pack<A ...>, Template_Pack<B ...>> {
		using type = Template_Pack<A ..., B ...>;
	};
	template<typename ... A>
	struct Concat_Packs<Template_Pack<A ...>, Template_Pack<>> {
		using type = Template_Pack<A ...>;
	};
	template<typename ... B>
	struct Concat_Packs<Template_Pack<>, Template_Pack<B ...>> {
		using type = Template_Pack<B ...>;
	};

	// in pack
	template<typename T, typename Pack>
	struct In_Pack_T {
		static constexpr bool value = false;
	};
	template<typename T>
	struct In_Pack_T<T, Template_Pack<>> {
		static constexpr bool value = false;
	};
	template<typename T, typename ... Pack>
	struct In_Pack_T<T, Template_Pack<T, Pack...>> {
		static constexpr bool value = true;
	};
	template<typename T, typename Head, typename ... Pack>
	struct In_Pack_T<T, Template_Pack<Head, Pack...>> {
		static constexpr bool value = In_Pack_T<T, Template_Pack<Pack...>>::value;
	};
	template<typename T, typename Pack>
	constexpr bool in_pack_v = In_Pack_T<T, Pack>::value;

	template<typename ... Pack>
	struct Unique_Pack;

	template<typename Head, typename ... Tail>
	struct Unique_Pack<Template_Pack<Head, Tail...>> {
		using type = conditional_t<
			in_pack_v<Head, Template_Pack<Tail ...>>,
			typename Unique_Pack<Template_Pack<Tail ...>>::type,
			typename Add_To_Pack<
				Head,
				typename Unique_Pack<Template_Pack<Tail ...>>::type
			>::type
		>;
	};

	template<typename T>
	struct Unique_Pack<Template_Pack<T>> {
		using type = Template_Pack<T>;
	};

	template<typename T, typename U>
	struct Flatten_Helper;

	template<typename ... Ts, typename ... Heads, typename ... Tail>
	struct Flatten_Helper<Template_Pack<Ts ...>, Template_Pack<Template_Pack<Heads ...>, Tail ...>> {
		using type = typename Flatten_Helper<Template_Pack<Ts ...>, Template_Pack<Heads ..., Tail ...>>::type;
	};
	template<typename ... Ts, typename Head, typename ... Tail>
	struct Flatten_Helper<Template_Pack<Ts ...>, Template_Pack<Head, Tail ...>> {
		using type = typename Flatten_Helper<Template_Pack<Ts ..., Head>, Template_Pack<Tail ...>>::type;
	};

	template<typename ... Ts>
	struct Flatten_Helper<Template_Pack<Ts ...>, Template_Pack<>> {
		using type = Template_Pack<Ts ...>;
	};


	template<typename T>
	struct Flatten_Pack;

	template<typename ... Pack>
	struct Flatten_Pack<Template_Pack<Pack ...>> {
		using type = typename Flatten_Helper<Template_Pack<>, Template_Pack<Pack ...>>::type;
	};


	struct A { int a; };
	struct B { int b; };
	struct C { int c; };
	struct D { int d; };
	struct E { int e; };
	struct F { int f; };

	using Pack1 = Template_Pack<A, B>;
	using Pack2 = Add_To_Pack<C, Pack1>::type;
	using Pack3 = Add_To_Pack<A, Pack2>::type;
	using Pack4 = Unique_Pack<Pack3>::type;
	using Pack5 = Add_To_Pack<B, Pack3>::type;
	using Pack6 = Add_To_Pack<A, Pack5>::type;
	using Pack7 = Unique_Pack<Pack6>::type;
	using Pack8 = Template_Pack<D, E, F>;
	using Pack9 = Concat_Packs<Pack1, Pack8>::type;
	using Pack10 = Template_Pack<Pack1, Pack2>;
	using Pack11 = Flatten_Pack<Pack10>::type;
	using Pack12 = Unique_Pack<Flatten_Pack<Pack10>::type>::type;
	using Pack13 = Template_Pack<A, B, C, D>;
	using Pack14 = Flatten_Pack<Pack13>::type;

	template<typename Pack>
	struct inherit {
		
	};

	template<typename ... Pack>
	struct inherit_helper : Pack ... {

	};

	template<typename ... Pack>
	struct inherit<Template_Pack<Pack ...>> : Pack ... {
	};

	template<typename T, typename F, typename U>
	struct infix_operator {
		F* op;
	};
	template<typename T, typename F, typename U>
	struct infix_operator_build {
		const T& lhs;
		F* op;
	};
}
template<typename T, typename F, typename U>
extern xstd::infix_operator_build<T, F, U> operator<(const T& lhs, xstd::infix_operator<T, F, U> op)
{
	return { lhs, op.op };
}
template<typename T, typename F, typename U>
extern U operator>(xstd::infix_operator_build<T, F, U> op, const T& rhs)
{
	return op.op(op.lhs, rhs);
}