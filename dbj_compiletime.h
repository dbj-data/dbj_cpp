
#ifndef DBJ_COMPILE_TIME_H_
#define DBJ_COMPILE_TIME_H_

/*
TODO: wherever there is a cast from void *  to object *
function can not be compile time
this makes all mem*  functions here redundant
*/

/*
It is easy to get lost and start believeing anything can be compile time in C++

It can not

Also we rely on the optimizations capabilities of modern compilers to make the simple functions
bellow almost as fast or as fast as inbuilt equivalents.

But we the added benefit: we can actually use them at C++ compile time. Example

static_assert( false ==  dbj::compiletime::strcompare("A","B") ) ;

The other added benfit is you can follow through here in debugger
The other added benfit is you can place in here your conditional checks and asserts

NOTE: you can not have function named as any CRT function even if it is inside your namespace
	  CRT function names are reserved

NOTE: arguments to these functions have to be compile time themselves, that means: literals

NOTE: this is to be used from C++ code, not C

*/

#include "dbj_common.h"

#ifndef DBJ_HAS_CXX17
#error C++17 or above is required
#endif // DBJ_CPP17

#include <array>
#include <string_view>

namespace dbj::compiletime
{
	extern "C"
	{
		// do not try this at home. ever.
		inline void secure_reset(void *s, size_t n) noexcept
		{
			assert(s);
			volatile char *p = (char *)s;
			while (n--)
				*p++ = 0;
		}

		constexpr char *str_ncpy(char *, const char *, size_t) noexcept;

		constexpr inline char *str_ncpy(char *destination, const char *source, size_t num) noexcept
		{
			assert(!is_empty(destination));
			assert(!is_empty(source));

			// return if no memory is allocated to the destination
			if (destination == nullptr)
				return nullptr;

			// take a pointer pointing to the beginning of destination string
			char *ptr = destination;

			// copy first num characters of C-string pointed by source
			// into the array pointed by destination
			while (*source && num--)
			{
				*destination = *source;
				destination++;
				source++;
			}

			// null terminate destination string
			*destination = '\0';

			// destination is returned by standard strncpy()
			return ptr;
		}

		// https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/aa366877(v=vs.85)?redirectedfrom=MSDN
		// https://opensource.apple.com/source/sudo/sudo-83/sudo/lib/util/memset_s.c.auto.html

#ifndef RSIZE_MAX
#if defined(SIZE_MAX)
#define RSIZE_MAX (SIZE_MAX >> 1)
#elif defined(__LP64__)
#define RSIZE_MAX 0x7fffffffffffffffUL
#else
#define RSIZE_MAX 0x7fffffffU
#endif
#endif

		inline errno_t
		mem_set_s(volatile void *v, rsize_t smax, int c, rsize_t n)
		{
			assert(v);

			errno_t ret = 0;
			/* Updating through a volatile pointer should not be optimized away. */
			using vuc_ptr = volatile unsigned char *;
			volatile unsigned char *s = static_cast<vuc_ptr>(v);

			/* Fatal runtime-constraint violations. */
			if (s == NULL || smax > RSIZE_MAX)
			{
				return ret = errno = EINVAL;
			}
			/* Non-fatal runtime-constraint violation, n must not exceed smax. */
			if (n > smax)
			{
				n = smax;
				return ret = errno = EINVAL;
			}

			while (n--)
				*s++ = (unsigned char)c;

			return ret;
		}

#undef RSIZE_MAX
		// memset, memcpy, memcmp, memchr, memmove

		constexpr inline void *mem_set(void *s, int c, size_t n)
		{
			assert(s);
			unsigned char *p = (unsigned char *)s;
			while (n--)
				*p++ = (unsigned char)c;
			return s;
		}

		constexpr inline int mem_cmp(const void *s1, const void *s2, size_t n)
		{
			assert(s1);
			assert(s2);

			const unsigned char *p1 = (const unsigned char *)s1, *p2 = (const unsigned char *)s2;
			while (n--)
				if (*p1 != *p2)
					return *p1 - *p2;
				else
					p1++, p2++;
			return 0;
		}

		constexpr inline void *mem_cpy(void *dest, const void *src, size_t n)
		{
			assert(dest);
			assert(src);

			char *dp = (char *)dest;
			const char *sp = (const char *)src;
			while (n--)
				*dp++ = *sp++;
			return dest;
		}

		constexpr inline void *mem_chr(const void *s, int c, size_t n)
		{
			assert(s);

			unsigned char *p = (unsigned char *)s;
			while (n--)
				if (*p != (unsigned char)c)
					p++;
				else
					return p;
			return 0;
		}

		// -----------------------------------------------------------------------------------------
		// compile time native string handling -- C++11 or better
		//
		// for testing and convincing yourself this works see https://godbolt.org/z/gGL95T
		//
		// NOTE: C++20 char8_t is a path peppered with a shards of glass, just don't
		// go there
		//
		// NOTE: char16_t and char32_t are ok. if need them add them bellow
		//
		// NOTE: WIN32 is UTF-16 aka wchar_t universe, WIN32 char API's are all
		// calling wchar_t variants
		//
		constexpr inline bool str_equal(char const *lhs, char const *rhs) noexcept
		{
			assert(!is_empty(lhs));
			assert(!is_empty(rhs));

			while (*lhs || *rhs)
				if (*lhs++ != *rhs++)
					return false;
			return true;
		}

		constexpr inline bool wstr_equal(wchar_t const *lhs, wchar_t const *rhs) noexcept
		{
			assert(!wis_empty(lhs));
			assert(!wis_empty(rhs));

			while (*lhs || *rhs)
				if (*lhs++ != *rhs++)
					return false;
			return true;
		}

		constexpr inline size_t str_len(const char *ch) noexcept
		{
			assert(!is_empty(ch));

			size_t len = 0;
			while (ch[len])
				++len;
			return len;
		}

		constexpr inline size_t wstr_len(const wchar_t *ch) noexcept
		{
			assert(!wis_empty(ch));

			size_t len = 0;
			while (ch[len])
				++len;
			return len;
		}

		constexpr size_t str_nlen(const char *str, size_t max)
		{
			assert(!is_empty(str));
			const char *end = (const char *)mem_chr(str, 0, max);
			return end ? (size_t)(end - str) : max;
		}

        // the FreeBSD implementation
		constexpr int
			str_cmp(register const char *s1, register const char *s2) 
		{
			while (*s1 == *s2++)
				if (*s1++ == 0)
					return (0);
// casting stops compile time 					 
// return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
			return (*s1 - *(s2 - 1));
		}

	} //	extern "C"

	// portable memmove is tricky in C
	// in C++ we have the luxury of template value arguments which are compile time
	// instead of
	// memmove( dest, src, 32 );
	// you will do
	// mem_move<32>( dest, src) ;

	template <size_t n>
	extern inline void *mem_move(void *dest, const void *src)
	{
		assert(dest);
		assert(src);
		// without n as template argument this will
		// require C99 VLA feature
		unsigned char tmp[n]{0};
		dbj::compiletime::mem_cpy(tmp, src, n);
		dbj::compiletime::mem_cpy(dest, tmp, n);
		return dest;
	}

	// non macro count of is easy in  C++
	template <typename T, size_t N>
	inline constexpr size_t countof(T const (&)[N]) { return N; };

#ifndef _MSVC_LANG
	template <typename T>
	inline constexpr size_t countof(T *) = delete;
#endif

	/*

	Return string literal copy with white spaces removed.

		1. This is compile time function.
		   It uses stack space, thus be carefull of going happy-go-lucky and overdosing on the "happy"

		What is returned is std::array, thus:

		2. result size equals the input size
		3. result strlen is less than it's size
		4. on all white spaces use-case strlen is 0

	Synopsis:

		using namespace dbj::compiletime ;
		constexpr auto clean_ = remove_ws("A \t\v\rB\n C");
		printf("\n%s\n", clean_.data());

	*/
	constexpr auto remove_ws_max_input_size{1024U};

	template <typename C, size_t N>
	constexpr auto remove_ws(const C (&literal_)[N])
	{

		static_assert(
			N < remove_ws_max_input_size,
			"remove_ws(), argument char array must be shorter than dbj::compiletime::remove_ws_max_input_size");

		std::array<C, N> rezult_{{}}; // zero it
		size_t rezult_idx_{};

		for (size_t j{}; j < N; ++j)
		{
			switch (literal_[j])
			{
			case ' ':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
			case '\v':
				continue;
				break;
			default:
				rezult_[rezult_idx_++] = literal_[j];
			};
		}
		return rezult_;
	}

	/*
	This is peculiarly useful.

	Basically copy string to static std::array
	which instance can be moved, as we know
	But. This is compile tame.
	why have I not thought of this before?

	constexpr auto arr_literal = to_array ("Whatever");

	https://godbolt.org/z/sTEceK
	*/
	template <size_t N>
	constexpr inline std::array<char, N> to_array(const char (&sl_)[N]) noexcept
	{
		std::array<char, N> arr_{{0}};
		for (auto k = 0U; k < N; ++k)
		{
			arr_[k] = sl_[k];
		}
		return arr_;
	}

	// this can not be compile time as it will require strlen?
	template <size_t N>
	constexpr inline std::array<char, N> to_array(const char *) noexcept = delete;

	/*
	----------------------------------------------------------------------------------------------------------------
	*/
	// are all 3 things equal?
	template <typename... T>
	constexpr inline bool equal3(const T &... args_)
	{
		if ((sizeof...(args_)) < 2)
			return true;
		// non recursive version
		const auto il_ = {args_...};
		// compare them all to the first
		auto first_ = *(il_.begin());
		// assumption
		bool rezult_{true};
		for (auto &&elem_ : il_)
		{
			// yes I know, first cycle compares first_ to itself ...
			rezult_ = rezult_ && (first_ == elem_);
			// short circuit-ing
			if (!rezult_)
				break;
		}
		return rezult_;
	};

	// simpler ?
	inline auto eq3 = [](auto v1, auto v2, auto v3) constexpr -> bool
	{
		return (v1 == v2) && (v2 == v3);
	};

} // namespace dbj::compiletime

#endif // !DBJ_COMPILE_TIME_H_
