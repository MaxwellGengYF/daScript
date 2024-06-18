#pragma once

#ifndef DAS_SKA_HASH
#define DAS_SKA_HASH 1
#endif

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <string>
#include <memory>
#include <vector>
#include <type_traits>
#include <initializer_list>
#include <functional>
#include <algorithm>
#include <functional>
#include <climits>

#include <limits.h>
#include <setjmp.h>

#include <mutex>

#include <EASTL/allocator.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/shared_ptr.h>
#include <xxhash.h>
#include <EASTL/deque.h>
#include "pdqsort.h"
namespace das {
template<typename T>
struct eastl_allocator {
	using value_type = T;
	constexpr eastl_allocator() noexcept = default;
	explicit constexpr eastl_allocator(const char*) noexcept {}
	template<typename U>
	constexpr eastl_allocator(eastl_allocator<U>) noexcept {}
	[[nodiscard]] auto allocate(std::size_t n) const noexcept {
		return static_cast<T*>(eastl::GetDefaultAllocator()->allocate(sizeof(T) * n, alignof(T)));
	}
	[[nodiscard]] auto allocate(std::size_t n, size_t alignment, size_t) const noexcept {
		assert(alignment >= alignof(T));
		return static_cast<T*>(eastl::GetDefaultAllocator()->allocate(sizeof(T) * n, alignment));
	}
	void deallocate(T* p, size_t) const noexcept {
		eastl::GetDefaultAllocator()->deallocate(p, alignof(T));
	}
	void deallocate(void* p, size_t) const noexcept {
		eastl::GetDefaultAllocator()->deallocate(p, alignof(T));
	}
	template<typename R>
	[[nodiscard]] constexpr auto operator==(eastl_allocator<R>) const noexcept -> bool {
		return std::is_same_v<T, R>;
	}
};
using eastl::enable_shared_from_this;
using eastl::function;
using eastl::make_shared;
using eastl::make_unique;
using eastl::shared_ptr;
using eastl::unique_ptr;
using eastl::vector;
using std::addressof;
using std::allocator;
using std::allocator_traits;
using std::atomic;
using std::ceil;
using std::condition_variable;
using std::copy;
using std::declval;
using std::equal_to;
using std::false_type;
using std::find;
using std::find_if;
using std::forward;
using std::forward_iterator_tag;
using std::index_sequence;
using std::is_abstract;
using std::is_arithmetic;
using std::is_base_of;
using std::is_base_of_v;
using std::is_const;
using std::is_const_v;
using std::is_default_constructible;
using std::is_destructible;
using std::is_enum;
using std::is_pointer;
using std::is_pointer_v;
using std::is_reference;
using std::is_same;
using std::is_same_v;
using std::is_standard_layout;
using std::is_trivial;
using std::is_trivially_constructible;
using std::is_trivially_copy_constructible;
using std::is_trivially_copyable;
using std::is_trivially_destructible;
using std::is_void;
using std::less;
using std::lock_guard;
using std::make_index_sequence;
using std::make_pair;
using std::make_tuple;
using std::max;
using std::min;
using std::move;
using std::mutex;
using std::next;
using std::pair;
using std::ptrdiff_t;
using std::recursive_mutex;
using std::remove_const;
using std::remove_cv;
using std::remove_cv_t;
using std::remove_reference_t;
using std::reverse;
using std::stable_sort;
using std::hash;
using std::string;
using std::stringstream;
using std::thread;
using std::to_string;
using std::true_type;
using std::tuple;
using std::underlying_type;
using std::unique_lock;
using std::wstring;

template<pdqsort_detail::LinearIterable Iter, pdqsort_detail::CompareFunc<Iter> Compare>
inline void sort(Iter begin, Iter end, Compare comp) {
	pdqsort(begin, end, comp);
}
template<pdqsort_detail::LinearIterable Iter>
inline void sort(Iter begin, Iter end) {
	pdqsort(begin, end);
}

namespace chrono {
using std::chrono::milliseconds;
}// namespace chrono
namespace this_thread {
inline void yield() {
	std::this_thread::yield();
}
}// namespace this_thread

}// namespace das

#include <fmt/core.h>

#if DAS_SKA_HASH
#ifdef _MSC_VER
#pragma warning(disable : 4503)// decorated name length exceeded, name was truncated
#endif
#include <ska/flat_hash_map.hpp>
namespace das {

template<typename T>
struct das_hash {};
static constexpr uint64_t hash64_default_seed = (1ull << 61ull) - 1ull;// (2^61 - 1) is a prime
inline uint64_t hash64(const void* ptr, size_t size, uint64_t seed) noexcept {
	return XXH3_64bits_withSeed(ptr, size, seed);
}

template<typename T>
	requires std::disjunction_v<std::is_arithmetic<T>,
								std::is_pointer<T>,
								std::is_enum<T>>
struct das_hash<T> {
	using is_avalaunching = void;
	[[nodiscard]] constexpr uint64_t operator()(T value, uint64_t seed = hash64_default_seed) const noexcept {
		return hash64(&value, sizeof(T), seed);
	}
};

template<typename T>
	requires requires(const T v) { v.hash(); }
struct das_hash<T> {
	using is_avalaunching = void;
	[[nodiscard]] constexpr uint64_t operator()(const T& value) const noexcept {
		return value.hash();
	}
};
template<>
struct das_hash<uint64_t> {
	[[nodiscard]] constexpr uint64_t operator()(uint64_t value) const noexcept {
		return value;
	}
};
template<typename Char, typename CharTraits>
struct basic_string_hash {
	using is_transparent = void;// to enable heterogeneous lookup
	using is_avalaunching = void;
	[[nodiscard]] uint64_t operator()(std::basic_string_view<Char, CharTraits> s,
									  uint64_t seed = hash64_default_seed) const noexcept {
		return hash64(s.data(), s.size() * sizeof(Char), seed);
	}
	template<typename Allocator>
	[[nodiscard]] uint64_t operator()(const std::basic_string<Char, CharTraits, Allocator>& s,
									  uint64_t seed = hash64_default_seed) const noexcept {
		return hash64(s.data(), s.size() * sizeof(Char), seed);
	}
	[[nodiscard]] uint64_t operator()(const Char* s,
									  uint64_t seed = hash64_default_seed) const noexcept {
		return hash64(s, CharTraits::length(s) * sizeof(Char), seed);
	}
};
template<typename Char>
struct is_char : std::disjunction<
					 std::is_same<std::remove_cvref_t<Char>, char>,
					 std::is_same<std::remove_cvref_t<Char>, char8_t>,
					 std::is_same<std::remove_cvref_t<Char>, char16_t>,
					 std::is_same<std::remove_cvref_t<Char>, char32_t>,
					 std::is_same<std::remove_cvref_t<Char>, wchar_t>> {};

template<typename Char>
constexpr bool is_char_v = is_char<Char>::value;
using string_hash = basic_string_hash<char, std::char_traits<char>>;
using u16string_hash = basic_string_hash<char16_t, std::char_traits<char16_t>>;
using u32string_hash = basic_string_hash<char32_t, std::char_traits<char32_t>>;
using u8string_hash = basic_string_hash<char8_t, std::char_traits<char8_t>>;
using wstring_hash = basic_string_hash<wchar_t, std::char_traits<wchar_t>>;
template<typename Char>
	requires is_char_v<Char>
struct das_hash<Char*> : string_hash {};

template<typename Char, size_t N>
	requires is_char_v<Char>
struct das_hash<Char[N]> : string_hash {};

template<typename C, typename CT, typename Alloc>
struct das_hash<std::basic_string<C, CT, Alloc>> : basic_string_hash<C, CT> {};

template<typename C, typename CT>
struct das_hash<std::basic_string_view<C, CT>> : basic_string_hash<C, CT> {};
// using string = std::basic_string<char, std::char_traits<char>, eastl_allocator<char>>;
// using u16string = std::basic_string<char16_t, std::char_traits<char16_t>, eastl_allocator<char16_t>>;
// using u32string = std::basic_string<char32_t, std::char_traits<char32_t>, eastl_allocator<char32_t>>;
// using u8string = std::basic_string<char8_t, std::char_traits<char8_t>, eastl_allocator<char8_t>>;
// using wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, eastl_allocator<wchar_t>>;

// using std::string_view;
// using std::u16string_view;
// using std::u32string_view;
// using std::u8string_view;
// using std::wstring_view;

template<typename K, typename V, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using das_map = das_ska::flat_hash_map<K, V, H, E, eastl_allocator<das::pair<K, V>>>;
template<typename K, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using das_set = das_ska::flat_hash_set<K, H, E, eastl_allocator<K>>;
template<typename K, typename V, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using das_hash_map = das_ska::flat_hash_map<K, V, H, E, eastl_allocator<das::pair<K, V>>>;
template<typename K, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using das_hash_set = das_ska::flat_hash_set<K, H, E, eastl_allocator<K>>;
template<typename K, typename V>
using das_safe_map = std::map<K, V, das::less<K>, eastl_allocator<das::pair<const K, V>>>;
template<typename K, typename C = das::less<K>>
using das_safe_set = std::set<K, C, eastl_allocator<K>>;
template<typename K, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using unordered_set = std::unordered_set<K, H, E, eastl_allocator<K>>;
template<typename K, typename V, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using unordered_map = std::unordered_map<K, V, H, E, eastl_allocator<das::pair<const K, V>>>;
template<typename K, class Pr = std::less<K>>
using set = std::set<K, Pr, eastl_allocator<K>>;
template<typename K, typename V, class Pr = std::less<K>>
using map = std::map<K, V, Pr, eastl_allocator<std::pair<const K, V>>>;
template<typename K>
using deque = eastl::deque<K>;

}// namespace das
#else
namespace das {
template<typename K, typename V, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using das_map = std::unordered_map<K, V, H, E>;
template<typename K, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using das_set = std::unordered_set<K, H, E>;
template<typename K, typename V, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using das_hash_map = std::unordered_map<K, V, H, E>;
template<typename K, typename H = das::das_hash<K>, typename E = das::equal_to<K>>
using das_hash_set = std::unordered_set<K, H, E>;
template<typename K, typename V>
using das_safe_map = std::map<K, V>;
template<typename K, typename C = das::less<K>>
using das_safe_set = std::set<K, C>;
}// namespace das
#endif

#define DAS_STD_HAS_BIND 1

// if enabled, the generated interop will be marginally slower
// the upside is that it well generate significantly less templated code, thus reducing compile time (and binary size)
#ifndef DAS_SLOW_CALL_INTEROP
#define DAS_SLOW_CALL_INTEROP 0
#endif

#ifndef DAS_MAX_FUNCTION_ARGUMENTS
#define DAS_MAX_FUNCTION_ARGUMENTS 32
#endif

#ifndef DAS_FUSION
#define DAS_FUSION 0
#endif

#if DAS_SLOW_CALL_INTEROP
#undef DAS_FUSION
#define DAS_FUSION 0
#endif

#ifndef DAS_DEBUGGER
#define DAS_DEBUGGER 1
#endif

#ifndef DAS_BIND_EXTERNAL
#if defined(_WIN32) && defined(_WIN64)
#define DAS_BIND_EXTERNAL 1
#elif defined(__APPLE__)
#define DAS_BIND_EXTERNAL 1
#elif defined(__linux__)
#define DAS_BIND_EXTERNAL 1
#elif defined __HAIKU__
#define DAS_BIND_EXTERNAL 1
#else
#define DAS_BIND_EXTERNAL 0
#endif
#endif

#ifndef DAS_PRINT_VEC_SEPARATROR
#define DAS_PRINT_VEC_SEPARATROR ","
#endif

#ifndef das_to_stdout
#define das_to_stdout(...)            \
	{                                 \
		fprintf(stdout, __VA_ARGS__); \
		fflush(stdout);               \
	}
#endif

#ifndef das_to_stderr
#define das_to_stderr(...)            \
	{                                 \
		fprintf(stderr, __VA_ARGS__); \
		fflush(stderr);               \
	}
#endif
