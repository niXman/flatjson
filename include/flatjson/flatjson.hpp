
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2022 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__FLATJSON_HPP
#define __FLATJSON__FLATJSON_HPP

//#include <iostream> // TODO: comment out

#ifndef FJ_NO_TOPLEV_FJSON
#include <ostream>
#endif // FJ_NO_TOPLEV_FJSON

#include <vector>
#include <string>
#include <limits>

#include <cassert>
#include <cstdint>
#include <cinttypes>
#include <cstring>

#ifndef FJ_NO_TOPLEV_IO
#   if defined(__linux__) || defined(__APPLE__)
#       include <sys/types.h>
#       include <sys/stat.h>
#       include <sys/mman.h>
#       include <sys/uio.h>
#       include <fcntl.h>
#       include <unistd.h>
#   elif defined(WIN32)
#       include <windows.h>
#   else
#       error "UNKNOWN PLATFORM!"
#   endif // OS detection
#endif // FJ_NO_TOPLEV_IO

/*************************************************************************************************/
// simd headers and definitions

#ifndef FJ_DONT_USE_SIMD
#   ifdef __AVX2__
#       define FJ_SIMD_TYPE "avx2"
#       define __FJ__HAS_AVX2
#   elif defined(__SSE2__)
#       define FJ_SIMD_TYPE "sse2"
#       define __FJ__HAS_SSE2
#   endif

#   if defined(__ARM_NEON) || defined(_M_ARM64)
#       define FJ_SIMD_TYPE "neon"
#       define __FJ__HAS_NEON
#   endif

#   if defined(__FJ__HAS_SSE2) || defined(__FJ__HAS_AVX2)
#       include <x86intrin.h>
#   endif

#   if defined(__FJ__HAS_NEON)
#       include <arm_neon.h>
#   endif

#ifdef _MSC_VER
#   include <intrin.h>
#endif // _MSC_VER

namespace flatjson {
namespace details {

#   ifdef __FJ__HAS_AVX2
        using simd_type = __m256i;
#   elif defined(__FJ__HAS_SSE2)
        using simd_type = __m128i;
#   elif defined(__FJ__HAS_NEON)
        using simd_type = int8x16_t;
#   else
#       error "wrong arch type!"
#   endif // neon

#   define __FJ__SIMD_SIZEOF \
        (unsigned int)sizeof(::flatjson::details::simd_type)

#   define __FJ__SIMD_ALIGNED \
        alignas(::flatjson::details::simd_type)

#   ifdef _MSC_VER
        template<typename T>
        std::size_t fj_ctz_safe(T v) { DWORD r = 0;  return v ? (_BitScanReverse(&r, v),r) : __FJ__SIMD_SIZEOF; }
        template<typename T>
        std::size_t fj_ffs_safe(T v) { DWORD r = 0; return v ? (_BitScanForward(&r, v),r) : 0; }
#   else
        template<typename T>
        std::size_t fj_ctz_safe(T v) { return v ? __builtin_ctz(v) : __FJ__SIMD_SIZEOF; }
        template<typename T>
        std::size_t fj_ffs_safe(T v) { return v ? __builtin_ffs(v)-1 : 0; }
#   endif // WIN32

    // because I don't want to include huge <algorithm> just because of one small std::min()
    template<typename T>
    T fj_min(T l, T r) { return l < r ? l : r; }

#   ifdef __FJ__HAS_AVX2 // avx2
#       define __FJ__SIMD_INIT1(ch) \
            _mm256_set1_epi8(ch)

#       define __FJ__SIMD_LOAD(p) \
            _mm256_load_si256(reinterpret_cast<const ::flatjson::details::simd_type *>(p))

#       define __FJ__SIMD_LOADU(p) \
            _mm256_loadu_si256(reinterpret_cast<const ::flatjson::details::simd_type *>(p))

#       define __FJ__SIMD_CMP_EQ(l, r) \
            _mm256_cmpeq_epi8(l, r)

#       define __FJ__SIMD_CMP_LT(l, r) \
            _mm256_cmplt_epi8(l, r)

#       define __FJ__SIMD_CMP_GT(l, r) \
            _mm256_cmpgt_epi8(l, r)

#       define __FJ__SIMD_AND(l, r) \
            _mm256_and_si256(l, r)

#       define __FJ__SIMD_OR(l, r) \
            _mm256_or_si256(l, r)

#       define __FJ__SIMD_ADD(l, r) \
            _mm256_add_epi8(l, r)

#       define __FJ__SIMD_MIN(l, r) \
            _mm256_min_epu8(l, r)

#       define __FJ__SIMD_TO_MASK(v) \
            _mm256_movemask_epi8(v)

#       define __FJ__SIMD_SET_ZERO() \
            _mm256_setzero_si256()

#   elif defined(__FJ__HAS_SSE2) // sse2

#       define __FJ__SIMD_INIT1(ch) \
            _mm_set1_epi8(ch)

#       define __FJ__SIMD_LOAD(p) \
            _mm_load_si128(reinterpret_cast<const ::flatjson::details::simd_type *>(p))

#       define __FJ__SIMD_LOADU(p) \
            _mm_loadu_si128(reinterpret_cast<const ::flatjson::details::simd_type *>(p))

#       define __FJ__SIMD_CMP_EQ(l, r) \
            _mm_cmpeq_epi8(l, r)

#       define __FJ__SIMD_CMP_LT(l, r) \
            _mm_cmplt_epi8(l, r)

#       define __FJ__SIMD_CMP_GT(l, r) \
            _mm_cmpgt_epi8(l, r)

#       define __FJ__SIMD_AND(l, r) \
            _mm_and_si128(l, r)

#       define __FJ__SIMD_OR(l, r) \
            _mm_or_si128(l, r)

#       define __FJ__SIMD_ADD(l, r) \
            _mm_add_epi8(l, r)

#       define __FJ__SIMD_MIN(l, r) \
            _mm_min_epu8(l, r)

#       define __FJ__SIMD_TO_MASK(v) \
            _mm_movemask_epi8(v)

#       define __FJ__SIMD_SET_ZERO() \
            _mm_setzero_si128()

#   elif defined(__FJ__HAS_NEON) // neon

#       define __FJ__SIMD_INIT1(ch) \
            vreinterpretq_s32_s8(vdupq_n_s8(ch))

#       define __FJ__SIMD_LOAD(p) \
            vld1q_s32(reinterpret_cast<std::int32_t *>(p))

#       define __FJ__SIMD_LOADU(p) \
            __FJ__SIMD_LOAD(p)

#       define __FJ__SIMD_CMP_EQ(l, r) \
            vreinterpretq_s32_u8(vceqq_s8(vreinterpretq_s8_s32(l), vreinterpretq_s8_s32(r)))

#       define __FJ__SIMD_CMP_LT(l, r) \
            vreinterpretq_s32_u8(vcltq_s8(vreinterpretq_s8_s32(l), vreinterpretq_s8_s32(r)))

#       define __FJ__SIMD_CMP_GT(l, r) \
            vreinterpretq_s32_u8(vcgtq_s8(vreinterpretq_s8_s32(l), vreinterpretq_s8_s32(r)))

#       define __FJ__SIMD_AND(l, r) \
            vandq_s32(l, r)

#       define __FJ__SIMD_OR(l, r) \
            vorrq_s32(l, r)

#       define __FJ__SIMD_ADD(l, r) \
            vreinterpretq_s32_s8(vaddq_s8(vreinterpretq_s8_s32(l), vreinterpretq_s8_s32(r)))

#       define __FJ__SIMD_MIN(l, r) \
            vreinterpretq_s32_u8(vminq_u8(vreinterpretq_u8_s32(l), vreinterpretq_u8_s32(r)))

#       define __FJ__SIMD_TO_MASK(v) \
            [](const ::flatjson::details::simd_type &in) -> uint32_t { \
                uint8x16_t vmask = vshlq_u8( \
                     vandq_u8(in, vdupq_n_u8(0x80)) \
                    ,vld1q_u8((uint8_t[]){-7,-6,-5,-4,-3,-2,-1,0,-7,-6,-5,-4,-3,-2,-1,0}) \
                ); \
                return vaddv_u8(vget_low_u8(vmask)) + (vaddv_u8(vget_high_u8(vmask)) << 8); \
            }(v)

#       define __FJ__SIMD_SET_ZERO() \
            vdupq_n_s32(0)

#   endif // __FJ_HAS_SSE2

#   if defined(__FJ__HAS_AVX2)
        __FJ__SIMD_ALIGNED static const char simd_mask_lut[__FJ__SIMD_SIZEOF * 2] = {
             -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
            ,-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
            , 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
            , 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
        };
#   elif defined(__FJ__HAS_SSE2) || defined(__FJ__HAS_NEON)
        __FJ__SIMD_ALIGNED static const char simd_mask_lut[__FJ__SIMD_SIZEOF * 2] = {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
            ,0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
        };
#   else
#       error "wrong arch type!"
#   endif // __FJ__HAS_SSE2 || __FJ__HAS_NEON

#   define __FJ__SIMD_COUNT_VALID_CHARS(pptr, pend) \
        [](const char *ptr, const char *end) -> std::size_t { \
            const auto *start = ptr; \
            for ( ; static_cast<unsigned>(end - ptr) >= __FJ__SIMD_SIZEOF \
                ; ptr += __FJ__SIMD_SIZEOF ) \
            { \
                auto line = __FJ__SIMD_LOADU(ptr); \
                auto mask = __FJ__SIMD_TO_MASK( \
                    __FJ__SIMD_OR( \
                        __FJ__SIMD_OR( \
                             __FJ__SIMD_CMP_EQ(line, __FJ__SIMD_INIT1('"')) \
                            ,__FJ__SIMD_CMP_EQ(line, __FJ__SIMD_INIT1('\\')) \
                        ) \
                        ,__FJ__SIMD_CMP_EQ( \
                             __FJ__SIMD_MIN(line, __FJ__SIMD_INIT1(0x1f)) \
                            ,line \
                        ) \
                    ) \
                ); \
                if ( mask ) { \
                    auto pos = fj_ctz_safe(mask); \
                    return ptr - start + pos; \
                } \
            } \
            \
            for ( ; ptr != end; ++ptr ) { \
                if ( *ptr == '"' || *ptr == '\\' || *ptr < ' ' ) \
                    break; \
            } \
            \
            return ptr - start; \
        }(pptr, pend)

#   ifdef __FJ__ADDRESS_SANITIZER_ENABLED
#       define __FJ__SIMD_LOAD_LINE_OR_LESS(pptr, pavail) \
            [](const char *ptr, std::size_t avail) -> ::flatjson::details::simd_type { \
                __FJ__SIMD_ALIGNED union { \
                    ::flatjson::details::simd_type simd; \
                    std::uint8_t arr[__FJ__SIMD_SIZEOF]; \
                } u; \
                u.simd = __FJ__SIMD_SET_ZERO(); \
                std::size_t to_read = ::flatjson::details::fj_min<std::size_t> \
                    (__FJ__SIMD_SIZEOF, avail); \
                std::memcpy(u.arr, ptr, to_read); \
                return __FJ__SIMD_LOAD(&(u.simd)); \
            }(pptr, pavail)
#   else
#       define __FJ__SIMD_LOAD_LINE_OR_LESS(pptr, pavail) \
            __FJ__SIMD_LOADU(pptr)
#   endif // __FJ__ADDRESS_SANITIZER_ENABLED

// returns NULL if no non-digits found
#   define __FJ__SIMD_FIND_NONDIGIT(pptr, plen) \
        [](const char *ptr, std::size_t len) -> const char* { \
            const char *end = ptr + len; \
            for ( ; ptr + __FJ__SIMD_SIZEOF <= end; ptr += __FJ__SIMD_SIZEOF ) { \
                auto line = __FJ__SIMD_LOADU(ptr); \
                auto mask = __FJ__SIMD_TO_MASK( \
                    __FJ__SIMD_OR( \
                         __FJ__SIMD_CMP_GT(__FJ__SIMD_INIT1('0'), line) \
                        ,__FJ__SIMD_CMP_GT(line, __FJ__SIMD_INIT1('9')) \
                    ) \
                ); \
                if ( mask ) { \
                    auto pos = fj_ffs_safe(mask); \
                    return ptr + pos; \
                } \
            } \
            std::size_t left = end - ptr; \
            if ( left ) { \
                auto line = __FJ__SIMD_LOAD_LINE_OR_LESS(ptr, left); \
                auto mask = __FJ__SIMD_TO_MASK( \
                    __FJ__SIMD_AND( \
                        __FJ__SIMD_OR( \
                             __FJ__SIMD_CMP_GT(__FJ__SIMD_INIT1('0'), line) \
                            ,__FJ__SIMD_CMP_GT(line, __FJ__SIMD_INIT1('9')) \
                        ) \
                        ,__FJ__SIMD_LOADU((simd_mask_lut + __FJ__SIMD_SIZEOF - left)) \
                    ) \
                ); \
                if ( mask ) { \
                    auto pos = fj_ffs_safe(mask); \
                    return ptr + pos; \
                } \
            } \
            return nullptr; \
        }(pptr, plen)

} // ns details
} // ns flatjson

#else // FJ_DONT_USE_SIMD
#   define __FJ__SIMD_ALIGNED
#   define __FJ__SIMD_COUNT_VALID_CHARS(pptr, pend) \
        [](const char *ptr, const char *end) -> std::size_t { \
            const char *start = ptr; \
            for ( ; ptr != end; ++ptr ) { \
                if ( *ptr == '"' || *ptr == '\\' || *ptr < ' ' ) \
                    break; \
            } \
            return ptr - start; \
        }(pptr, pend)

#   define __FJ__SIMD_FIND_NONDIGIT(pptr, plen) \
        (pptr + std::strspn(pptr, "0123456789"))

#endif // FJ_DONT_USE_SIMD

/*************************************************************************************************/

#define __FJ__CAT_I(x, y) x##y
#define __FJ__CAT(x, y) __FJ__CAT_I(x, y)

#define __FJ__STRINGIZE_I(x) #x
#define __FJ__STRINGIZE(x) __FJ__STRINGIZE_I(x)

// FJ_VERSION_HEX >> 24 - is the major version
// FJ_VERSION_HEX >> 16 - is the minor version
// FJ_VERSION_HEX >> 8  - is the bugfix level

#define FJ_VERSION_MAJOR 0
#define FJ_VERSION_MINOR 0
#define FJ_VERSION_BUGFIX 3

#define FJ_VERSION_HEX \
    ((FJ_VERSION_MAJOR << 24) \
    | (FJ_VERSION_MINOR << 16) \
    | (FJ_VERSION_BUGFIX << 8))

#define FJ_VERSION_STRING \
        __FJ__STRINGIZE(FJ_VERSION_MAJOR) \
    "." __FJ__STRINGIZE(FJ_VERSION_MINOR) \
    "." __FJ__STRINGIZE(FJ_VERSION_BUGFIX)

/*************************************************************************************************/

#ifndef FJ_DONT_CHECK_OVERFLOW
#   define __FJ__CHECK_OVERFLOW(expr, type, err) \
        if ( (expr) >= (std::numeric_limits<type>::max)() ) return err
#else
#   define __FJ__CHECK_OVERFLOW(expr, type, err)
#endif // FJ_DONT_CHECK_OVERFLOW

/*************************************************************************************************/

#if __cplusplus >= 201703L
#   define __FJ__FALLTHROUGH [[fallthrough]]
#   define __FJ__CONSTEXPR_IF(...) if constexpr (__VA_ARGS__)
#   include <string_view>
    namespace flatjson {
        using string_view = std::string_view;
    } // ns flatjson
#else
#   define __FJ__CONSTEXPR_IF(...) if (__VA_ARGS__)
#   if defined(__clang__)
#       define __FJ__FALLTHROUGH [[clang::fallthrough]]
#   elif defined(__GNUC__)
#       define __FJ__FALLTHROUGH __attribute__ ((fallthrough))
#   elif defined(_MSC_VER)
#       define __FJ__FALLTHROUGH
#   else
#       error "Unknown compiler"
#   endif //
#endif // __cplusplus >= 201703L

#ifndef FJ_KLEN_TYPE
#   define FJ_KLEN_TYPE std::uint8_t
#endif // FJ_KLEN_TYPE
#ifndef FJ_VLEN_TYPE
#   define FJ_VLEN_TYPE std::uint16_t
#endif // FJ_VLEN_TYPE
#ifndef FJ_CHILDS_TYPE
#   define FJ_CHILDS_TYPE std::uint16_t
#endif // FJ_CHILDS_TYPE

/*************************************************************************************************/

//#define __FJ__ANALYZE_PARSER
#ifdef __FJ__ANALYZE_PARSER
#include <time.h>

namespace flatjson {
namespace details {

inline std::uint64_t fj_ns_time() {
    struct timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (std::uint64_t)ts.tv_sec * 1000000000ULL + (std::uint64_t)ts.tv_nsec;
}

} // ns details

struct parse_stat {
    std::size_t   f_skip_ws_num;
    std::uint64_t f_skip_ws_ns;
    std::size_t   f_parse_expected_num;
    std::uint64_t f_parse_expected_ns;
    std::size_t   f_parse_string_num;
    std::uint64_t f_parse_string_ns;
    std::size_t   f_parse_integer_num;
    std::uint64_t f_parse_integer_ns;
    std::size_t   f_parse_double_num;
    std::uint64_t f_parse_double_ns;
};

} // ns flatjson

#define __FJ__ANALYZE__BLOCK(p, parsemode, func) \
    struct fj_scope_meter { \
        std::uint64_t time_start; \
        std::uint64_t &time_end; \
        std::size_t &cnt; \
        \
        ~fj_scope_meter() { \
            if ( parsemode ) { \
                auto t = fj_ns_time() - time_start; \
                time_end += t; \
                cnt += 1; \
            } \
        } \
    } __fj__scope{ \
         (parsemode ? fj_ns_time() : 0ull) \
        ,__FJ__CAT(p->pstat.f_, __FJ__CAT(func, _ns)) \
        ,__FJ__CAT(p->pstat.f_, __FJ__CAT(func, _num)) \
    }

#else
#define __FJ__ANALYZE__BLOCK(p, parsemode, func)
#endif // __FJ__ANALYZE_PARSER

/*************************************************************************************************/

namespace flatjson {

template<typename T>
struct enable_if_const_char_ptr;

template<>
struct enable_if_const_char_ptr<const char *> {
    using type = void;
};

/*************************************************************************************************/

#if __cplusplus < 201703L

struct string_view {
    string_view() = default;
    explicit string_view(const char *s)
        :m_ptr{s}
        ,m_len{std::strlen(m_ptr)}
    {}
    template<std::size_t N>
    explicit string_view(const char (&str)[N])
        :m_ptr{str}
        ,m_len{N-1}
    {}
    string_view(const char *ptr, std::size_t len)
        :m_ptr{ptr}
        ,m_len{len}
    {}
    string_view(const char *beg, const char *end)
        :m_ptr{beg}
        ,m_len{static_cast<std::size_t>(end - beg)}
    {}

    std::size_t size() const { return m_len; }
    bool empty() const { return size() == 0; }
    const char* data() const { return m_ptr; }

    string_view substr(std::size_t pos = 0, std::size_t n = std::string::npos) const {
        assert(pos <= size());
        return string_view( data() + pos, (std::min)( n, size() - pos ) );
    }

    int compare(string_view other) const
    { int res = std::strncmp(data(), other.data(), (std::min)(size(), other.size()));
        return res ? res : size() == other.size() ? 0 : size() < other.size() ? -1 : 1; }
    int compare(std::size_t pos1, std::size_t n1, string_view other) const
    { return substr(pos1, n1).compare(other); }
    int compare(std::size_t pos1, std::size_t n1, string_view other, std::size_t pos2, std::size_t n2) const
    { return substr(pos1, n1).compare(other.substr(pos2, n2)); }
    template<std::size_t N>
    int compare(const char (&r)[N]) const { return compare(string_view{r, N-1}); }
    int compare(const char *s) const { return compare(string_view(s)); }
    template<std::size_t N>
    int compare(std::size_t pos1, std::size_t n1, const char (&s)[N]) const
    { return substr(pos1, n1).compare(string_view(s, N-1)); }
    int compare(std::size_t pos1, std::size_t n1, const char *s) const
    { return substr(pos1, n1).compare(string_view(s)); }
    int compare( std::size_t pos1, std::size_t n1, const char *s, std::size_t n2 ) const
    { return substr(pos1, n1).compare(string_view(s, n2)); }

    template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
    friend bool operator==(const string_view &l, T r) { return l.compare(r) == 0; }
    template<std::size_t N>
    friend bool operator==(const string_view &l, const char (&r)[N])
    { return l.compare(0, N-1, r, N-1) == 0; }
    friend bool operator==(const string_view &l, const string_view &r)
    { return l.compare(r) == 0; }

    template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
    friend bool operator!=(const string_view &l, T r) { return !(l == r); }
    template<std::size_t N>
    friend bool operator!=(const string_view &l, const char (&r)[N]) { return !(l == r); }
    friend bool operator!=(const string_view &l, const string_view &r) { return !(l == r); }

    friend std::ostream& operator<< (std::ostream &os, const string_view &s) {
        os.write(s.m_ptr, s.m_len);

        return os;
    }

private:
    const char *m_ptr;
    std::size_t m_len;
};

#endif // __cplusplus >= 201703L

/*************************************************************************************************/

// for testing and examples purposses, at least for now

template<std::size_t N>
const char* begin(const char (&str)[N]) { return str; }

template<std::size_t N>
const char* end(const char (&str)[N]) { return &str[N-1]; }

/*************************************************************************************************/

enum token_type: std::uint8_t {
     FJ_TYPE_INVALID = 0
    ,FJ_TYPE_STRING
    ,FJ_TYPE_NUMBER
    ,FJ_TYPE_BOOL
    ,FJ_TYPE_NULL
    ,FJ_TYPE_OBJECT
    ,FJ_TYPE_OBJECT_END
    ,FJ_TYPE_ARRAY
    ,FJ_TYPE_ARRAY_END
};

inline const char *type_name(token_type t) {
    static const char* strs[] = {
         "INVALID"
        ,"STRING"
        ,"NUMBER"
        ,"BOOL"
        ,"NULL"
        ,"OBJECT"
        ,"OBJECT_END"
        ,"ARRAY"
        ,"ARRAY_END"
    };

    std::size_t idx = static_cast<std::size_t>(t);
    if ( idx < sizeof(strs)/sizeof(strs[0]) ) {
        return strs[idx];
    }

    return "UNKNOWN TYPE";
}

enum error_code: char {
     EC_OK = 0
    ,EC_INVALID = -1
    ,EC_INCOMPLETE = -2
    ,EC_EXTRA_DATA = -3
    ,EC_NO_FREE_TOKENS = -4
    ,EC_KLEN_OVERFLOW = -5
    ,EC_VLEN_OVERFLOW = -6
    ,EC_CHILDS_OVERFLOW = -7
};

inline const char* error_string(error_code e) {
    static const char* strs[] = {
         "OK"
        ,"INVALID"
        ,"INCOMPLETE"
        ,"NO_FREE_TOKENS"
        ,"KLEN_OVERFLOW"
        ,"VLEN_OVERFLOW"
        ,"CHILDS_OVERFLOW"
    };
    auto idx = static_cast<std::int8_t>(e);
    idx = -idx;

    return strs[idx];
}

struct error_info {
    error_code  code;
    char        js_str[16];
    unsigned    js_pos;
    unsigned    in_dump_pos;
    unsigned    line;
    const char *func;
    const char *prev_func;
};

#define __FJ__INIT_ERROR_INFO(p, pcode, pprev_func) { \
        p->error.code = pcode; \
        std::memset(p->error.js_str, 0, sizeof(p->error.js_str)); \
        p->error.js_pos = p->json_cur - p->json_beg; \
        if ( p->json_cur ) { \
            const char *js_cur = p->json_cur; \
            unsigned from_back = (js_cur - p->json_beg) > 10 \
                ? 10 \
                : (js_cur - p->json_beg) \
            ; \
            p->error.in_dump_pos = from_back; \
            unsigned to_copy = static_cast<unsigned>(p->json_end - (js_cur - from_back)) \
                < sizeof(p->error.js_str) \
                    ? (p->json_end - (js_cur - from_back)) \
                    : sizeof(p->error.js_str) \
            ; \
            /* std::printf("from_back=%d, to_copy=%u\n", from_back, to_copy);*/ \
            to_copy ? std::memcpy(p->error.js_str, (js_cur - from_back), to_copy) : nullptr; \
        } \
        p->error.func = __func__; \
        p->error.line = __LINE__; \
        p->error.prev_func = pprev_func; \
    }

inline const char* error_message(const error_info &e) {
    static char buf[512];
    const char *p = e.js_str;
    auto f = [](const char *p) -> char { return isprint(*p) ? *p : '.'; };
    auto c = [](const char *p) -> std::uint8_t { return std::uint8_t(*p); };
    auto len = std::snprintf(buf, sizeof(buf)
        ,"%s, in function \"%s()\"(%u), prev function \"%s()\", at json position %u, content:\n"
         "c|%c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c |\n"
         "x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|\n"
         "%*s^\n"
        ,error_string(e.code)
        ,e.func, e.line ,e.prev_func
        ,e.js_pos
        ,f(p+0), f(p+1), f(p+2), f(p+3), f(p+4), f(p+5), f(p+6), f(p+7)
            ,f(p+8), f(p+9), f(p+10), f(p+11), f(p+12), f(p+13), f(p+14), f(p+15)
        ,c(p+0), c(p+1), c(p+2), c(p+3), c(p+4), c(p+5), c(p+6), c(p+7)
            ,c(p+8), c(p+9), c(p+10), c(p+11), c(p+12), c(p+13), c(p+14), c(p+15)
        ,2+(3 * e.in_dump_pos), " "
    );
    buf[len] = 0;

    return buf;
}

/*************************************************************************************************/

namespace details {

/*************************************************************************************************/

template<typename To>
typename std::enable_if<
    (std::is_integral<To>::value && std::is_unsigned<To>::value) &&
    !std::is_same<To, bool>::value, To
>::type
conv_to(const char *ptr, std::size_t len, To) {
    const auto *str = reinterpret_cast<const std::uint8_t *>(ptr);
    std::uint64_t res = 0;

    constexpr std::uint64_t
         _1  = 1ull,      _10  = _1*10,   _100  = _10*10
        ,_1k = _100*10,   _10k = _1k*10,  _100k = _10k*10
        ,_1m = _100k*10,  _10m = _1m*10,  _100m = _10m*10
        ,_1b = _100m*10,  _10b = _1b*10,  _100b = _10b*10
        ,_1t = _100b*10,  _10t = _1t*10,  _100t = _10t*10
        ,_1kd= _100t*10,  _10kd= _1kd*10, _100kd= _10kd*10
        ,_1kn= _100kd*10, _10kn= _1kn*10
    ;

    switch ( len ) {
        case 20: res += ((str[len - 20] - '0') * _10kn) ; __FJ__FALLTHROUGH;
        case 19: res += ((str[len - 19] - '0') * _1kn)  ; __FJ__FALLTHROUGH;
        case 18: res += ((str[len - 18] - '0') * _100kd); __FJ__FALLTHROUGH;
        case 17: res += ((str[len - 17] - '0') * _10kd) ; __FJ__FALLTHROUGH;
        case 16: res += ((str[len - 16] - '0') * _1kd)  ; __FJ__FALLTHROUGH;
        case 15: res += ((str[len - 15] - '0') * _100t) ; __FJ__FALLTHROUGH;
        case 14: res += ((str[len - 14] - '0') * _10t)  ; __FJ__FALLTHROUGH;
        case 13: res += ((str[len - 13] - '0') * _1t)   ; __FJ__FALLTHROUGH;
        case 12: res += ((str[len - 12] - '0') * _100b) ; __FJ__FALLTHROUGH;
        case 11: res += ((str[len - 11] - '0') * _10b)  ; __FJ__FALLTHROUGH;
        case 10: res += ((str[len - 10] - '0') * _1b)   ; __FJ__FALLTHROUGH;
        case 9 : res += ((str[len - 9 ] - '0') * _100m) ; __FJ__FALLTHROUGH;
        case 8 : res += ((str[len - 8 ] - '0') * _10m)  ; __FJ__FALLTHROUGH;
        case 7 : res += ((str[len - 7 ] - '0') * _1m)   ; __FJ__FALLTHROUGH;
        case 6 : res += ((str[len - 6 ] - '0') * _100k) ; __FJ__FALLTHROUGH;
        case 5 : res += ((str[len - 5 ] - '0') * _10k)  ; __FJ__FALLTHROUGH;
        case 4 : res += ((str[len - 4 ] - '0') * _1k)   ; __FJ__FALLTHROUGH;
        case 3 : res += ((str[len - 3 ] - '0') * _100)  ; __FJ__FALLTHROUGH;
        case 2 : res += ((str[len - 2 ] - '0') * _10)   ; __FJ__FALLTHROUGH;
        case 1 : res += ((str[len - 1 ] - '0') * _1)    ;
            break;
        default:
            assert(!"unreachable!");
    }

    return static_cast<To>(res);
}

template<typename To>
typename std::enable_if<
    (std::is_integral<To>::value && std::is_signed<To>::value) &&
    !std::is_same<To, bool>::value, To
>::type
conv_to(const char *ptr, std::size_t len, To) {
    using UnsignedTo = typename std::make_unsigned<To>::type;
    if ( *ptr == '-' ) {
        ++ptr;
        auto res = conv_to(ptr, len - 1, UnsignedTo{});
        if ( res > static_cast<UnsignedTo>(std::numeric_limits<To>::max()) ) {
            assert(!"overflow detected!");
        }

        return -static_cast<To>(res);
    }

    return conv_to(ptr, len, UnsignedTo{});
}

template<typename To>
typename std::enable_if<std::is_same<To, bool>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) {
    return *ptr == 't' && len == 4;
}

template<typename To>
typename std::enable_if<std::is_same<To, double>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) {
    char buf[std::numeric_limits<To>::max_exponent10 + 20];
    std::memcpy(buf, ptr, len);
    buf[len] = 0;

    return std::strtod(buf, nullptr);
}

template<typename To>
typename std::enable_if<std::is_same<To, float>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) {
    char buf[std::numeric_limits<To>::max_exponent10 + 20];
    std::memcpy(buf, ptr, len);
    buf[len] = 0;

    return std::strtof(buf, nullptr);
}

template<typename To>
typename std::enable_if<std::is_same<To, std::string>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) { return {ptr, len}; }

template<typename To>
typename std::enable_if<std::is_same<To, flatjson::string_view>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) { return {ptr, len}; }

} // ns details

/*************************************************************************************************/
// 40 bytes for now
struct token {
    const char *key;
    const char *val;
    token *parent;
    token *end;
    FJ_CHILDS_TYPE childs;
    FJ_VLEN_TYPE vlen;
    FJ_KLEN_TYPE klen;
    token_type type;
    std::uint8_t flags;
    std::uint8_t unused;
};

/*************************************************************************************************/

using alloc_fnptr = void*(*)(std::size_t);
using free_fnptr = void(*)(void *);

struct parser {
    const char *json_beg;
    const char *json_cur;
    const char *json_end;
#define fj_parser_json_beg_ptr(parser) (parser->json_beg)
#define fj_parser_json_cur_ptr(parser) (parser->json_cur)
#define fj_parser_json_end_ptr(parser) (parser->json_end)
#define fj_parser_json_cur_char(parser) (*fj_parser_json_cur_ptr(parser))
#define fj_parser_json_increment_cur(parser) ++(parser->json_cur)
#define fj_parser_json_advance_cur(parser, num) (parser->json_cur += (num))
#define fj_parser_json_left_chars(parser) \
    static_cast<std::size_t>(fj_parser_json_end_ptr(parser) - fj_parser_json_cur_ptr(parser))
#define fj_parser_json_length(parser) \
    static_cast<std::size_t>(parser->json_end - parser->json_beg)

    token *tokens_beg;
    token *tokens_cur;
    token *tokens_end;
#define fj_parser_tokens_beg_ptr(parser) (parser->tokens_beg)
#define fj_parser_tokens_cur_ptr(parser) (parser->tokens_cur)
#define fj_parser_tokens_end_ptr(parser) (parser->tokens_end)
#define fj_parser_tokens_increment_cur(parser) (parser->tokens_cur)++
#define fj_parser_tokens_decrement_cur(parser) --(parser->tokens_cur)
#define fj_parser_tokens_advance_cur(parser, num) (parser->tokens_cur += (num))
#define fj_parser_tokens_left(parser) \
    static_cast<std::size_t>(fj_parser_tokens_end_ptr(parser) - fj_parser_tokens_cur_ptr(parser))
#define fj_parser_tokens_no_free(parser) (fj_parser_tokens_left(parser) == 0)
#define fj_parser_tokens_used(parser) \
    static_cast<std::size_t>(fj_parser_tokens_cur_ptr(parser) - fj_parser_tokens_beg_ptr(parser))

    alloc_fnptr alloc_func;
    free_fnptr  free_func;
    error_info  error;
    bool dyn_tokens;
    bool dyn_parser;

#ifndef FJ_NO_TOPLEV_FJSON
    std::uint32_t ref_counter;
#define fj_parser_refcnt_get(parser) (parser->ref_counter)
#define fj_parser_refcnt_increment(parser) ++(parser->ref_counter)
#define fj_parser_refcnt_decrement(parser) --(parser->ref_counter)
#endif // FJ_NO_TOPLEV_FJSON

#ifdef __FJ__ANALYZE_PARSER
    parse_stat pstat;
#endif // __FJ__ANALYZE_PARSER
};

#ifdef __FJ__ANALYZE_PARSER

inline const parse_stat& get_parse_stat(const parser *p) {
    return p->pstat;
}

inline void dump_parser_stat(const parser *p, std::FILE *os = stdout) {
    std::fprintf(os,
        "skip_ws: times-%zu, ns-%" PRIu64 "\n"
        "expect : times-%zu, ns-%" PRIu64 "\n"
        "string : times-%zu, ns-%" PRIu64 "\n"
        "integer: times-%zu, ns-%" PRIu64 "\n"
        "double : times-%zu, ns-%" PRIu64 "\n"
        ,p->pstat.f_skip_ws_num, p->pstat.f_skip_ws_ns
        ,p->pstat.f_parse_expected_num, p->pstat.f_parse_expected_ns
        ,p->pstat.f_parse_string_num, p->pstat.f_parse_string_ns
        ,p->pstat.f_parse_integer_num, p->pstat.f_parse_integer_ns
        ,p->pstat.f_parse_double_num, p->pstat.f_parse_double_ns
    );
    std::fflush(os);
}
#endif // __FJ__ANALYZE_PARSER

/*************************************************************************************************/

namespace details {

/*************************************************************************************************/
// for debug purposes

inline void dump_tokens_impl(
     std::FILE *stream
    ,const token *beg
    ,const token *cur
    ,const token *end
    ,std::size_t indent)
{
    static const char* tnames[] = {
         "INV" // invalid type
        ,"STR" // string type
        ,"NUM" // number type
        ,"BOL" // bool type
        ,"NUL" // null type
        ,"+OB" // start object
        ,"-OB" // and object
        ,"+AR" // start array
        ,"-AR" // end array
    };
    static const char spaces[] = "                                                         ";

    int local_indent = 0;
    for ( auto it = beg; it != end; ++it ) {
        if ( it->type == FJ_TYPE_ARRAY_END || it->type == FJ_TYPE_OBJECT_END ) {
            assert(indent > 0);
            local_indent -= static_cast<int>(indent);
        }
        std::fprintf(stream, "%3d:%c type=%.*s%3s, addr=%p, end=%p, parent=%p, childs=%d, key=\"%.*s\", val=\"%.*s\"\n"
            ,(int)(it - beg)
            ,(it == cur ? '>' : ' ')
            ,local_indent, spaces, tnames[it->type]
            ,it
            ,it->end
            ,it->parent
            ,(int)it->childs
            ,(int)(it->klen ? it->klen : 5), (it->klen ? it->key : "(nil)")
            ,(int)(it->vlen ? it->vlen : 5), (it->vlen ? it->val : "(nil)")
        );
        std::fflush(stream);

        if ( it->type == FJ_TYPE_ARRAY || it->type == FJ_TYPE_OBJECT ) {
            local_indent += static_cast<int>(indent);
        }
    }
}

/*************************************************************************************************/

static const std::uint8_t fj_utf8_char_len_map[] = {
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    ,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    ,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    ,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
    ,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
    ,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
    ,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
};

#define fj_utf8_char_len_macro(ch) \
    fj_utf8_char_len_map[static_cast<std::uint8_t>(ch)]

#define fj_is_simple_type_macro(v) \
    ("011110000"[v] == '1')

static const bool fj_is_digit_map[] = {
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static_assert(sizeof(fj_is_digit_map) == 256, "");

#define fj_is_digit_macro(ch) \
    (fj_is_digit_map[static_cast<std::uint8_t>(ch)])

static const bool fj_is_hex_digit_map[] = {
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static_assert(sizeof(fj_is_hex_digit_map) == 256, "");

#define fj_is_hex_digit_macro(ch) \
    (fj_is_digit_macro(ch) || fj_is_hex_digit_map[static_cast<std::uint8_t>(ch)])

#define fj_is_hex_digit4_macro(ch0, ch1, ch2, ch3) \
    (fj_is_hex_digit_macro(ch0) && fj_is_hex_digit_macro(ch1) \
        && fj_is_hex_digit_macro(ch2) && fj_is_hex_digit_macro(ch3))

// 0x09/0x0a/0x0d/0x20
static const bool fj_skip_ws_char_map[] = {
     0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static_assert(sizeof(fj_skip_ws_char_map) == 256, "");

template<bool ParseMode>
inline const char* fj_skip_ws(parser *p) {
    __FJ__ANALYZE__BLOCK(p, ParseMode, skip_ws);

    for ( ;fj_skip_ws_char_map[static_cast<std::uint8_t>(fj_parser_json_cur_char(p))]
          ; fj_parser_json_increment_cur(p) )
    {}

    return fj_parser_json_cur_ptr(p);
}

#define fj_current_char_macro(p) \
    (fj_parser_json_cur_char(p) <= ' ' ? *fj_skip_ws<ParseMode>(p) : fj_parser_json_cur_char(p))

#define fj_check_and_skip_macro(p, exp) \
    ((fj_parser_json_cur_char(p) - exp) \
        ? (EC_INVALID) \
        : (fj_parser_json_increment_cur(p),EC_OK))

/*************************************************************************************************/
// string

enum class expected_type {
     true_
    ,false_
    ,null_
};

template<bool ParseMode, expected_type Ex>
inline error_code parse_expected(
     const char *prev_func
    ,parser *p
    ,const char **value
    ,std::size_t *vlen
    ,token_type */*tokt*/
    ,token */*parent*/)
{
    __FJ__ANALYZE__BLOCK(p, ParseMode, parse_expected);

    // without first letters
    static const char *exp_str = (Ex == expected_type::true_)
        ? "true"
        : (Ex == expected_type::false_)
            ? "false"
            : "null"
    ;
    static const auto exp_len = (Ex == expected_type::true_)
        ? 4u
        : (Ex == expected_type::false_)
            ? 5u
            : 4u
    ;

    if ( std::memcmp(fj_parser_json_cur_ptr(p), exp_str, exp_len) != 0 ) {
        __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);

        return p->error.code;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        *value = fj_parser_json_cur_ptr(p);
        *vlen = exp_len;
    }

    fj_parser_json_advance_cur(p, exp_len);

    return EC_OK;
}

template<bool ParseMode>
inline error_code validate_escaped(const char *prev_func, parser *p) {
    switch ( fj_parser_json_cur_char(p) ) {
        case 'u': {
            if ( fj_parser_json_left_chars(p) < 5 ) {
                __FJ__INIT_ERROR_INFO(p, EC_INCOMPLETE, prev_func);

                return EC_INCOMPLETE;
            }
            if ( !fj_is_hex_digit4_macro(
                     *(fj_parser_json_cur_ptr(p)+1)
                    ,*(fj_parser_json_cur_ptr(p)+2)
                    ,*(fj_parser_json_cur_ptr(p)+3)
                    ,*(fj_parser_json_cur_ptr(p)+4)) )
            {
                __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);

                return EC_INVALID;
            }

            fj_parser_json_advance_cur(p, 5);

            return EC_OK;
        }
        case '"':
        case '\\':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't': {
            if ( fj_parser_json_left_chars(p) < 2 ) {
                return EC_INCOMPLETE;
            }
            fj_parser_json_advance_cur(p, 1);

            return EC_OK;
        }
    }

    return EC_INVALID;
}

template<bool ParseMode>
inline error_code parse_string(
     const char *prev_func
    ,parser *p
    ,const char **value
    ,std::size_t *vlen
    ,token_type */*tokt*/
    ,token */*parent*/)
{
    __FJ__ANALYZE__BLOCK(p, ParseMode, parse_string);

    std::uint8_t ch = 0;
    auto *start = fj_parser_json_cur_ptr(p);

    while ( fj_parser_json_left_chars(p) ) {
        auto pos = __FJ__SIMD_COUNT_VALID_CHARS(
             fj_parser_json_cur_ptr(p)
            ,fj_parser_json_end_ptr(p)
        );
        fj_parser_json_advance_cur(p, pos);

        ch = fj_parser_json_cur_char(p);
        if ( ch < ' ' ) {
            __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);

            return EC_INVALID;
        }
        if ( ch != '"' ) {
            if ( ch & 0x80 ) {
                auto len = fj_utf8_char_len_macro(ch);
                if ( fj_parser_json_left_chars(p) < len ) {
                    __FJ__INIT_ERROR_INFO(p, EC_INCOMPLETE, prev_func);

                    return EC_INCOMPLETE;
                }
                fj_parser_json_advance_cur(p, len);
            } else if ( ch == '\\' ) {
                fj_parser_json_increment_cur(p);
                auto ec = validate_escaped<ParseMode>(__func__, p);
                if ( ec != EC_OK ) {
                    return ec;
                }
            }
        } else {
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *value = start;
                *vlen = fj_parser_json_cur_ptr(p) - start;
            }

            fj_parser_json_increment_cur(p);

            break;
        }
    }

    return ch == '"' ? EC_OK : EC_INCOMPLETE;
}

/*************************************************************************************************/
// number

template<bool ParseMode>
inline error_code parse_integer(
     const char *prev_func
    ,parser *p
    ,const char **value
    ,std::size_t *vlen
    ,token_type */*tokt*/
    ,token */*parent*/)
{
    __FJ__ANALYZE__BLOCK(p, ParseMode, parse_integer);

    const auto *start = fj_parser_json_cur_ptr(p);

    fj_parser_json_advance_cur(p
        ,static_cast<unsigned>(!(fj_parser_json_cur_char(p) - '-'))
    );

    if ( !(fj_parser_json_cur_char(p) == '0'
         && fj_is_digit_macro(*(fj_parser_json_cur_ptr(p) + 1))) )
    {
        const char *non_digit = __FJ__SIMD_FIND_NONDIGIT(
             fj_parser_json_cur_ptr(p)
            ,fj_parser_json_left_chars(p)
        );
        if ( non_digit && non_digit != fj_parser_json_cur_ptr(p) ) {
            if ( *non_digit == '.' ) {
                fj_parser_json_cur_ptr(p) = non_digit + 1;

                non_digit = __FJ__SIMD_FIND_NONDIGIT(
                     fj_parser_json_cur_ptr(p)
                    ,fj_parser_json_left_chars(p)
                );
                if ( non_digit == fj_parser_json_cur_ptr(p) ) {
                    __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);

                    return EC_INVALID;
                }
                if ( !non_digit ) {
                    fj_parser_json_advance_cur(p, fj_parser_json_left_chars(p));

                    __FJ__CONSTEXPR_IF( ParseMode ) {
                        *value = start;
                        *vlen = fj_parser_json_cur_ptr(p) - start;
                    }

                    return EC_OK;
                }
            }
            if ( (*non_digit | 0x20) == 'e' ) {
                fj_parser_json_cur_ptr(p) = non_digit + 1;

                std::size_t pm = fj_parser_json_cur_char(p) == '-'
                    || fj_parser_json_cur_char(p) == '+'
                ;
                fj_parser_json_advance_cur(p, pm);

                non_digit = __FJ__SIMD_FIND_NONDIGIT(
                     fj_parser_json_cur_ptr(p)
                    ,fj_parser_json_left_chars(p)
                );
                if ( non_digit == fj_parser_json_cur_ptr(p) ) {
                    __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);

                    return EC_INVALID;
                }
            }
            fj_parser_json_cur_ptr(p) = non_digit;
        } else {
            fj_parser_json_advance_cur(p, fj_parser_json_left_chars(p));
        }
    } else {
        __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);
        return EC_INVALID;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        *value = start;
        *vlen = fj_parser_json_cur_ptr(p) - start;
    }

    return EC_OK;
}

/*************************************************************************************************/

template<bool ParseMode, bool DespacedJson>
inline error_code parse_value(
     const char *prev_func
    ,parser *p
    ,const char **value
    ,std::size_t *vlen
    ,token_type *toktype
    ,token *parent
);

template<bool ParseMode, bool DespacedJson>
inline error_code parse_array(
     const char *prev_func
    ,parser *p
    ,const char **/*value*/
    ,std::size_t */*vlen*/
    ,token_type */*tokt*/
    ,token *parent)
{
    __FJ__CONSTEXPR_IF ( ParseMode ) {
        if ( fj_parser_tokens_no_free(p) ) {
            __FJ__INIT_ERROR_INFO(p, EC_NO_FREE_TOKENS, prev_func);

            return p->error.code;
        }
    }

    auto *startarr = fj_parser_tokens_increment_cur(p);
    __FJ__CONSTEXPR_IF( ParseMode ) {
        startarr->type = FJ_TYPE_ARRAY;
        startarr->flags = 1;
        startarr->parent = parent;
        if ( startarr->parent ) {
            __FJ__CHECK_OVERFLOW(parent->childs, FJ_CHILDS_TYPE, EC_CHILDS_OVERFLOW);
            ++parent->childs;
        }
    }

    __FJ__CONSTEXPR_IF( !DespacedJson ) {
        fj_parser_json_cur_ptr(p) = fj_skip_ws<ParseMode>(p);
    }
    while ( fj_parser_json_cur_char(p) != ']' ) {
        __FJ__CONSTEXPR_IF ( ParseMode ) {
            if ( fj_parser_tokens_no_free(p) ) {
                __FJ__INIT_ERROR_INFO(p, EC_NO_FREE_TOKENS, prev_func);

                return p->error.code;
            }
        }

        auto *current_token = fj_parser_tokens_cur_ptr(p);
        __FJ__CONSTEXPR_IF( ParseMode ) {
            *current_token = token{};
        }
        char current_ch = fj_parser_json_cur_char(p);
        if ( current_ch != '{' && current_ch != '[' ) {
            fj_parser_tokens_increment_cur(p);

            __FJ__CONSTEXPR_IF( ParseMode ) {
                current_token->parent = startarr;
                __FJ__CHECK_OVERFLOW(startarr->childs, FJ_CHILDS_TYPE, EC_CHILDS_OVERFLOW);
                ++startarr->childs;
            }
        }

        std::size_t vlen = 0;
        auto ec = parse_value<ParseMode, DespacedJson>(
             __func__
            ,p
            ,&(current_token->val)
            ,&vlen
            ,&(current_token->type)
            ,startarr
        );
        if ( ec != EC_OK ) {
            return ec;
        }

        __FJ__CONSTEXPR_IF( ParseMode ) {
            startarr->flags = (startarr->flags == 0)
                ? startarr->flags
                : fj_is_simple_type_macro(current_token->type)
            ;
            __FJ__CHECK_OVERFLOW(vlen, FJ_VLEN_TYPE, EC_VLEN_OVERFLOW);
            current_token->vlen = static_cast<FJ_VLEN_TYPE>(vlen);
        }

        __FJ__CONSTEXPR_IF( !DespacedJson ) {
            fj_parser_json_cur_ptr(p) = fj_skip_ws<ParseMode>(p);
        }
        if ( fj_parser_json_cur_char(p) == ',' ) {
            fj_parser_json_increment_cur(p);
            if ( fj_parser_json_cur_char(p) == ']' ) {
                __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);

                return EC_INVALID;
            }
        } else if ( fj_parser_json_cur_char(p) != ']' ) {
            __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);

            return EC_INVALID;
        }

        __FJ__CONSTEXPR_IF( !DespacedJson ) {
            fj_parser_json_cur_ptr(p) = fj_skip_ws<ParseMode>(p);
        }
    } // while loop

    auto ec = fj_check_and_skip_macro(p, ']');
    if ( ec != EC_OK ) {
        __FJ__INIT_ERROR_INFO(p, ec, prev_func);

        return ec;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        if ( fj_parser_tokens_no_free(p) ) {
            __FJ__INIT_ERROR_INFO(p, EC_NO_FREE_TOKENS, prev_func);

            return EC_NO_FREE_TOKENS;
        }

        auto *endarr = fj_parser_tokens_increment_cur(p);
        *endarr = token{};
        endarr->type = FJ_TYPE_ARRAY_END;
        endarr->parent = startarr;
        __FJ__CHECK_OVERFLOW(endarr->parent->childs, FJ_CHILDS_TYPE, EC_CHILDS_OVERFLOW);
        ++startarr->childs;
        startarr->end = endarr;
    } else {
        fj_parser_tokens_increment_cur(p);
    }

    return EC_OK;
}

template<bool ParseMode, bool DespacedJson>
inline error_code parse_object(
     const char *prev_func
    ,parser *p
    ,const char **/*value*/
    ,std::size_t */*vlen*/
    ,token_type */*tokt*/
    ,token *parent)
{
    __FJ__CONSTEXPR_IF ( ParseMode ) {
        if ( fj_parser_tokens_no_free(p) ) {
            __FJ__INIT_ERROR_INFO(p, EC_NO_FREE_TOKENS, prev_func);

            return p->error.code;
        }
    }

    __FJ__CONSTEXPR_IF( !DespacedJson ) {
        fj_parser_json_cur_ptr(p) = fj_skip_ws<ParseMode>(p);
    }
    if ( fj_parser_json_cur_char(p) != '}' && fj_parser_json_cur_char(p) != '"' ) {
        __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);
        return EC_INVALID;
    }

    auto *startobj = fj_parser_tokens_increment_cur(p);
    __FJ__CONSTEXPR_IF( ParseMode ) {
        startobj->type = FJ_TYPE_OBJECT;
        startobj->flags = 1;
        startobj->parent = parent;
        if ( startobj->parent ) {
            __FJ__CHECK_OVERFLOW(parent->childs, FJ_CHILDS_TYPE, EC_CHILDS_OVERFLOW);
            ++parent->childs;
        }
    }

    while ( fj_parser_json_cur_char(p) != '}' ) {
        __FJ__CONSTEXPR_IF ( ParseMode ) {
            if ( fj_parser_tokens_no_free(p) ) {
                __FJ__INIT_ERROR_INFO(p, EC_NO_FREE_TOKENS, prev_func);
                return EC_NO_FREE_TOKENS;
            }
        }

        auto *current_token = fj_parser_tokens_increment_cur(p);
        __FJ__CONSTEXPR_IF( ParseMode ) {
            *current_token = token{};
        }
        __FJ__CONSTEXPR_IF( !DespacedJson ) {
            fj_parser_json_cur_ptr(p) = fj_skip_ws<ParseMode>(p);
        }

        std::size_t klen = 0;
        auto ec = parse_value<ParseMode, DespacedJson>(
             __func__
            ,p
            ,&(current_token->key)
            ,&klen
            ,&(current_token->type)
            ,startobj
        );
        if ( ec != EC_OK ) {
            return ec;
        }
        __FJ__CONSTEXPR_IF( ParseMode ) {
            __FJ__CHECK_OVERFLOW(klen, FJ_KLEN_TYPE, EC_KLEN_OVERFLOW);
            current_token->klen = static_cast<FJ_KLEN_TYPE>(klen);
        }

        __FJ__CONSTEXPR_IF( !DespacedJson ) {
            fj_parser_json_cur_ptr(p) = fj_skip_ws<ParseMode>(p);
        }
        ec = fj_check_and_skip_macro(p, ':');
        if ( ec != EC_OK ) {
            __FJ__INIT_ERROR_INFO(p, ec, prev_func);

            return ec;
        }

        char ch = DespacedJson ? fj_parser_json_cur_char(p) : fj_current_char_macro(p);
        if ( ch == '[' || ch == '{' ) {
            fj_parser_tokens_decrement_cur(p);
            ec = parse_value<ParseMode, DespacedJson>(
                 __func__
                ,p
                ,nullptr
                ,nullptr
                ,&(current_token->type)
                ,startobj
            );
            __FJ__CONSTEXPR_IF( ParseMode ) {
                startobj->flags = (startobj->flags == 0)
                    ? startobj->flags
                    : fj_is_simple_type_macro(current_token->type)
                ;
            }
        } else {
            __FJ__CONSTEXPR_IF( ParseMode ) {
                current_token->parent = startobj;
                __FJ__CHECK_OVERFLOW(startobj->childs, FJ_CHILDS_TYPE, EC_CHILDS_OVERFLOW);
                ++startobj->childs;
            }

            std::size_t vlen = 0;
            ec = parse_value<ParseMode, DespacedJson>(
                 __func__
                ,p
                ,&(current_token->val)
                ,&vlen
                ,&(current_token->type)
                ,startobj
            );
            __FJ__CONSTEXPR_IF( ParseMode ) {
                __FJ__CHECK_OVERFLOW(vlen, FJ_VLEN_TYPE, EC_VLEN_OVERFLOW);
                current_token->vlen = static_cast<FJ_VLEN_TYPE>(vlen);
            }
        }

        if ( ec != EC_OK ) {
            return ec;
        }

        ch = DespacedJson ? fj_parser_json_cur_char(p) : fj_current_char_macro(p);
        if ( ch == ',' ) {
            fj_parser_json_increment_cur(p);
            if ( fj_parser_json_cur_char(p) == '}' ) {
                __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);

                return p->error.code;
            }
        }
    } // while loop

    auto ec = fj_check_and_skip_macro(p, '}');
    if ( ec != EC_OK ) {
        __FJ__INIT_ERROR_INFO(p, ec, prev_func);

        return ec;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        if ( fj_parser_tokens_no_free(p) ) {
            __FJ__INIT_ERROR_INFO(p, EC_NO_FREE_TOKENS, prev_func);

            return p->error.code;
        }
        auto *endobj = fj_parser_tokens_increment_cur(p);
        *endobj = token{};
        endobj->type = FJ_TYPE_OBJECT_END;
        endobj->parent = startobj;
        __FJ__CHECK_OVERFLOW(endobj->parent->childs, FJ_CHILDS_TYPE, EC_CHILDS_OVERFLOW);
        ++endobj->parent->childs;
        startobj->end = endobj;
    } else {
        fj_parser_tokens_increment_cur(p);
    }

    return EC_OK;
}

template<bool ParseMode, bool DespacedJson>
inline error_code parse_value(
     const char *prev_func
    ,parser *p
    ,const char **value
    ,std::size_t *vlen
    ,token_type *toktype
    ,token *parent)
{
    using parser_function_type = error_code(*)(
         const char *
        ,parser *
        ,const char **
        ,std::size_t *
        ,token_type *
        ,token *
    );

    __FJ__SIMD_ALIGNED static const std::uint8_t valid_chars[] =
        {'{', '[', 't', 'f', 'n', '"', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-'};

    struct handler_item {
        parser_function_type func;
        token_type type;
        int skip_first;
    };
    __FJ__SIMD_ALIGNED static const handler_item handlers_array[] = {
    /* { */  {&parse_object<ParseMode, DespacedJson>, FJ_TYPE_OBJECT, 1}
    /* [ */ ,{&parse_array<ParseMode, DespacedJson>, FJ_TYPE_ARRAY, 1}
    /* t */ ,{&parse_expected<ParseMode, expected_type::true_>, FJ_TYPE_BOOL, 0}
    /* f */ ,{&parse_expected<ParseMode, expected_type::false_>, FJ_TYPE_BOOL, 0}
    /* n */ ,{&parse_expected<ParseMode, expected_type::null_>, FJ_TYPE_NULL, 0}
    /* " */ ,{&parse_string<ParseMode>, FJ_TYPE_STRING, 1}
    /* 1 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* 2 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* 3 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* 4 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* 5 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* 6 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* 7 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* 8 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* 9 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* 0 */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    /* - */ ,{&parse_integer<ParseMode>, FJ_TYPE_NUMBER, 0}
    };
    static_assert(sizeof(handlers_array)/sizeof(handlers_array[0]) == sizeof(valid_chars), "");

    error_code ec{EC_INVALID};
    const char first_char = fj_parser_json_cur_char(p);
    const auto *pi = std::memchr(valid_chars, first_char, sizeof(valid_chars));
    if ( pi ) {
        const auto idx = static_cast<const std::uint8_t *>(pi) - valid_chars;
        //std::cout << "c=" << first_char << ", idx=" << idx << std::endl;

        const auto &handler = handlers_array[idx];
        fj_parser_json_advance_cur(p, handler.skip_first);
        ec = handler.func(__func__, p, value, vlen, toktype, parent);
        __FJ__CONSTEXPR_IF( ParseMode ) { *toktype = handler.type; }
    } else {
        __FJ__INIT_ERROR_INFO(p, EC_INVALID, prev_func);

        return EC_INVALID;
    }

    std::size_t inc = static_cast<unsigned>
        (fj_parser_tokens_cur_ptr(p) == fj_parser_tokens_beg_ptr(p));
    fj_parser_tokens_advance_cur(p, inc);

    return ec;
}

/*************************************************************************************************/

inline void init_parser(
     parser *p
    ,token *toksbeg
    ,token *toksend
    ,const char *strbeg
    ,const char *strend
    ,alloc_fnptr alloc_fn
    ,free_fnptr free_fn
    ,bool dyn_parser
    ,bool dyn_tokens)
{
    // root token
    if ( toksbeg ) {
        toksbeg->type   = FJ_TYPE_INVALID;
        toksbeg->key    = nullptr;
        toksbeg->klen   = 0;
        toksbeg->val    = nullptr;
        toksbeg->vlen   = 0;
        toksbeg->parent = nullptr;
        toksbeg->childs = 0;
        toksbeg->end    = nullptr;
    }

    fj_parser_json_beg_ptr(p) = strbeg;
    fj_parser_json_cur_ptr(p) = strbeg;
    fj_parser_json_end_ptr(p) = strend;
    fj_parser_tokens_beg_ptr(p) = toksbeg;
    fj_parser_tokens_cur_ptr(p) = toksbeg;
    fj_parser_tokens_end_ptr(p) = toksend;
    p->alloc_func  = alloc_fn;
    p->free_func   = free_fn;
    __FJ__INIT_ERROR_INFO(p, EC_INVALID, "");
    p->dyn_parser= dyn_parser;
    p->dyn_tokens= dyn_tokens;
#ifndef FJ_NO_TOPLEV_FJSON
    p->ref_counter   = 0;
#endif // FJ_NO_TOPLEV_FJSON
#ifdef __FJ__ANALYZE_PARSER
    std::memset(&(p->pstat), 0, sizeof(p->pstat));
#endif // __FJ__ANALYZE_PARSER
}

} // ns details

/*************************************************************************************************/

// zero-alloc routines

inline parser make_parser(
     token *toksbeg
    ,token *toksend
    ,const char *strbeg
    ,const char *strend)
{
    parser p;
    details::init_parser(
         &p
        ,toksbeg
        ,toksend
        ,strbeg
        ,strend
        ,nullptr
        ,nullptr
        ,false
        ,false
    );

    return p;
}

inline parser init_parser(
     alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    parser p;
    details::init_parser(
         &p
        ,nullptr
        ,nullptr
        ,nullptr
        ,nullptr
        ,alloc_fn
        ,free_fn
        ,false
        ,false
    );

    return p;
}

/*************************************************************************************************/
// dyn-alloc routines

inline parser* alloc_parser(
     token *toksbeg
    ,token *toksend
    ,const char *strbeg
    ,const char *strend
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    auto *p = static_cast<parser *>(alloc_fn(sizeof(parser)));
    if ( !p ) {
        return nullptr;
    }

    details::init_parser(
         p
        ,toksbeg
        ,toksend
        ,strbeg
        ,strend
        ,alloc_fn
        ,free_fn
        ,true
        ,false
    );

    return p;
}

inline std::size_t count_tokens(
     error_info *ei
    ,const char *strbeg
    ,const char *strend
    ,bool despaced_json = false)
{
    if ( strbeg == strend ) {
        if ( ei ) { ei->code = EC_INVALID; }

        return 0;
    }

    static token fake_token;
    auto p = make_parser(&fake_token, &fake_token, strbeg, strend);
    fj_parser_json_cur_ptr((&p)) = details::fj_skip_ws<false>(&p);

    error_code ec = despaced_json
        ? details::parse_value<false, true>(
             __func__
            ,&p
            ,nullptr
            ,nullptr
            ,nullptr
            ,nullptr)
        : details::parse_value<false, false>(
            __func__
            ,&p
            ,nullptr
            ,nullptr
            ,nullptr
            ,nullptr)
    ;

    if ( ec == EC_OK ) {
        if ( fj_parser_json_left_chars((&p)) ) {
            fj_parser_json_cur_ptr((&p)) = details::fj_skip_ws<false>(&p);
            if ( fj_parser_json_left_chars((&p)) ) {
                __FJ__INIT_ERROR_INFO((&p), EC_EXTRA_DATA, "");
                if ( ei ) { *ei = p.error; }

                return 0;
            }
        }
    } else {
        p.error.code = ec;
        if ( ei ) { *ei = p.error; }

        return 0;
    }

    std::size_t toknum = fj_parser_tokens_cur_ptr((&p)) - fj_parser_tokens_beg_ptr((&p));

    return toknum;
}

inline parser make_parser(
     const char *strbeg
    ,const char *strend
    ,bool despaced_json = false
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    parser p;
    details::init_parser(
         &p
        ,nullptr
        ,nullptr
        ,nullptr
        ,nullptr
        ,alloc_fn
        ,free_fn
        ,false
        ,true
    );

    error_info ei{};
    ei.code = EC_OK;
    auto toknum = count_tokens(&ei, strbeg, strend, despaced_json);
    if ( ei.code ) {
        p.error = ei;
        return p;
    }

    auto in_bytes = sizeof(token) * toknum;
    auto *toksbeg = static_cast<token *>(alloc_fn(in_bytes));
    auto *toksend = toksbeg + toknum;

    details::init_parser(
         &p
        ,toksbeg
        ,toksend
        ,strbeg
        ,strend
        ,alloc_fn
        ,free_fn
        ,false
        ,true
    );

    return p;
}

inline parser* alloc_parser(
     const char *strbeg
    ,const char *strend
    ,bool despaced_json = false
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    auto *p = static_cast<parser *>(alloc_fn(sizeof(parser)));
    if ( !p ) {
        return nullptr;
    }

    details::init_parser(
         p
        ,nullptr
        ,nullptr
        ,nullptr
        ,nullptr
        ,alloc_fn
        ,free_fn
        ,true
        ,true
    );

    if ( strbeg == strend ) {
        return p;
    }

    error_info ei{};
    ei.code = EC_OK;
    auto toknum = count_tokens(&ei, strbeg, strend, despaced_json);
    if ( ei.code ) {
        p->error = ei;
        return p;
    }

    auto in_bytes = sizeof(token) * toknum;
    auto *toksbeg = static_cast<token *>(alloc_fn(in_bytes));
    auto *toksend = toksbeg + toknum;

    details::init_parser(
         p
        ,toksbeg
        ,toksend
        ,strbeg
        ,strend
        ,alloc_fn
        ,free_fn
        ,true
        ,true
    );
    p->error.code = EC_OK;

    return p;
}

inline void free_parser(parser *p) {
    if ( p->dyn_tokens && fj_parser_tokens_beg_ptr(p) ) {
        p->free_func(fj_parser_tokens_beg_ptr(p));
    }

    fj_parser_tokens_beg_ptr(p) = nullptr;
    fj_parser_tokens_cur_ptr(p) = nullptr;
    fj_parser_tokens_end_ptr(p) = nullptr;
    p->error = {EC_INVALID, "", 0, 0, 0, nullptr, nullptr};

    if ( p->dyn_parser ) {
        p->free_func(p);
    }
}

/*************************************************************************************************/
// parsing

inline std::size_t parse(parser *p, bool despaced_json = false) {
    if ( fj_parser_json_left_chars(p) == 0 ) {
        p->error.code = EC_INVALID;

        return 0;
    }

    fj_parser_json_cur_ptr(p) = details::fj_skip_ws<false>(p);

    std::size_t vlen{};
    token_type type{};
    p->error.code = despaced_json
        ? details::parse_value<true, true>(
             __func__
            ,p
            ,&fj_parser_tokens_beg_ptr(p)->val
            ,&vlen
            ,&type
            ,nullptr)
        : details::parse_value<true, false>(
            __func__
            ,p
            ,&fj_parser_tokens_beg_ptr(p)->val
            ,&vlen
            ,&type
            ,nullptr)
    ;

    if ( p->error.code == EC_OK ) {
        fj_parser_json_cur_ptr(p) = details::fj_skip_ws<false>(p);
        if ( fj_parser_json_cur_ptr(p) != fj_parser_json_cur_ptr(p)
             && fj_parser_json_cur_char(p) != '\0' )
        {
            p->error.code = EC_INVALID;
        }
    }

    fj_parser_tokens_beg_ptr(p)->type = type;
    assert(vlen <= std::numeric_limits<FJ_VLEN_TYPE>::max());
    fj_parser_tokens_beg_ptr(p)->vlen = static_cast<FJ_VLEN_TYPE>(vlen);

    return fj_parser_tokens_cur_ptr(p) - fj_parser_tokens_beg_ptr(p);
}

// returns the num of tokens
inline std::size_t parse(
     token *tokbeg
    ,token *tokend
    ,const char *strbeg
    ,const char *strend
    ,bool despaced_json = false)
{
    auto p = make_parser(tokbeg, tokend, strbeg, strend);

    return parse(&p, despaced_json);
}

// returns the dyn-allocated parser
inline parser* parse(
     const char *strbeg
    ,const char *strend
    ,bool despaced_json = false
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    auto *p = alloc_parser(strbeg, strend, despaced_json, alloc_fn, free_fn);
    if ( p->error.code != EC_OK ) {
        return p;
    }

    parse(p);

    return p;
}

/*************************************************************************************************/
// parser state

inline bool is_valid(const parser *p) {
    return p && p->error.code == EC_OK;
}

inline error_code get_error(const parser *p) {
    return p->error.code;
}

inline const char* get_error_message(const parser *p) {
    return error_message(p->error);
}

inline std::size_t num_tokens(const parser *p) {
    return fj_parser_tokens_cur_ptr(p) - fj_parser_tokens_beg_ptr(p);
}

inline std::size_t num_childs(const parser *p) {
    return (!fj_is_simple_type_macro(fj_parser_tokens_beg_ptr(p)->type))
       ? fj_parser_tokens_beg_ptr(p)->childs - 1
       : static_cast<std::size_t>(fj_parser_tokens_beg_ptr(p)->type != FJ_TYPE_INVALID)
    ;
}

inline bool is_empty(const parser *p) {
    return p == nullptr || fj_parser_tokens_beg_ptr(p) == fj_parser_tokens_end_ptr(p);
}

inline bool is_array(const parser *p)
{ return fj_parser_tokens_beg_ptr(p)->type == FJ_TYPE_ARRAY; }

inline bool is_object(const parser *p)
{ return fj_parser_tokens_beg_ptr(p)->type == FJ_TYPE_OBJECT; }

inline bool is_null(const parser *p)
{ return fj_parser_tokens_beg_ptr(p)->type == FJ_TYPE_NULL; }

inline bool is_bool(const parser *p)
{ return fj_parser_tokens_beg_ptr(p)->type == FJ_TYPE_BOOL; }

inline bool is_number(const parser *p)
{ return fj_parser_tokens_beg_ptr(p)->type == FJ_TYPE_NUMBER; }

inline bool is_string(const parser *p)
{ return fj_parser_tokens_beg_ptr(p)->type == FJ_TYPE_STRING; }

inline bool is_simple_type(const parser *p)
{ return fj_is_simple_type_macro(fj_parser_tokens_beg_ptr(p)->type); }

/*************************************************************************************************/
// dump for parser

inline void dump_tokens(
     std::FILE *stream
    ,const char *caption
    ,const parser *parser
    ,std::size_t indent = 3)
{
    std::fprintf(stream, "%s:\n", caption);
    return details::dump_tokens_impl(
         stream
        ,fj_parser_tokens_beg_ptr(parser)
        ,fj_parser_tokens_beg_ptr(parser)
        ,fj_parser_tokens_end_ptr(parser)
        ,indent
    );
}

/*************************************************************************************************/
// iterators

struct iterator {
    token *beg;
    token *cur;
    token *end;

    string_view key() const { return {cur->key, cur->klen}; }
    string_view value() const { return {cur->val, cur->vlen}; }
    std::size_t childs() const { return cur->childs; }
    const token* parent() const { return cur->parent; }

    token_type type() const { return cur->type; }
    const char* type_name() const { return flatjson::type_name(type()); }
    bool is_valid() const { return cur && type() != FJ_TYPE_INVALID; }
    bool is_array() const { return type() == FJ_TYPE_ARRAY; }
    bool is_object() const { return type() == FJ_TYPE_OBJECT; }
    bool is_null() const { return type() == FJ_TYPE_NULL; }
    bool is_bool() const { return type() == FJ_TYPE_BOOL; }
    bool is_number() const { return type() == FJ_TYPE_NUMBER; }
    bool is_string() const { return type() == FJ_TYPE_STRING; }
    bool is_simple_type() const { return fj_is_simple_type_macro(type()); }

    string_view to_string_view() const { return value(); }
    std::string to_string() const { auto s = to_string_view(); return {s.data(), s.size()}; }

    template<typename T>
    T to() const { auto s = to_string_view(); return details::conv_to(s.data(), s.size(), T{}); }
    bool to_bool() const { return to<bool>(); }
    std::uint32_t to_uint() const { return to<std::uint32_t>(); }
    std::int32_t to_int() const { return to<std::int32_t>(); }
    std::uint64_t to_uint64() const { return to<std::uint64_t>(); }
    std::int64_t to_int64() const { return to<std::int64_t>(); }
    double to_double() const { return to<double>(); }
    float to_float() const { return to<float>(); }

    std::size_t members() const {
        return (!is_simple_type())
           ? childs() - 1
           : static_cast<std::size_t>(type() != FJ_TYPE_INVALID)
        ;
    }
};

// dump for iterator
inline void dump_tokens(
     std::FILE *stream
    ,const char *caption
    ,const iterator &it
    ,std::size_t indent = 3)
{
    std::fprintf(stream, "%s:\n", caption);
    return details::dump_tokens_impl(stream, it.beg, it.cur, it.end, indent);
}

inline iterator iter_begin(const parser *p) {
    return {
         fj_parser_tokens_beg_ptr(p)
        ,fj_parser_tokens_beg_ptr(p)
        ,fj_parser_tokens_cur_ptr(p)-1
    };
}

inline iterator iter_end(const parser *p) {
    return {
         fj_parser_tokens_cur_ptr(p)-1
        ,fj_parser_tokens_cur_ptr(p)-1
        ,fj_parser_tokens_cur_ptr(p)-1
    };
}

inline iterator iter_begin(const iterator &it) {
    if ( !it.is_simple_type() ) {
        return {it.cur, it.cur, it.cur->end};
    }

    return {it.cur, it.cur, it.cur->parent->end};
}

inline iterator iter_end(const iterator &it) {
    if ( !it.is_simple_type() ) {
        return {it.cur->end, it.cur->end, it.cur->end};
    }

    return {it.end, it.end, it.end};
}

inline bool iter_equal(const iterator &l, const iterator &r)
{ return l.cur == r.cur; }

inline bool iter_not_equal(const iterator &l, const iterator &r)
{ return !iter_equal(l, r); }

inline std::size_t iter_members(const iterator &it) {
    if ( it.is_simple_type() ) {
        return 0;
    }

    return it.cur->childs - 1;
}

inline iterator iter_next(const iterator &it) {
    assert(it.cur != it.end);

    auto next = it.cur + 1;
    if ( next != it.end && next->parent == it.beg ) {
        return {it.beg, next, it.end};
    }

    for ( ; next != it.end && next->parent != it.beg; ++next )
    {}

    return {it.beg, next, it.end};
}

inline std::size_t iter_distance(const iterator &from, const iterator &to) {
    assert(from.cur->parent == to.cur->parent);

    if ( from.cur->parent->flags == 1 ) {
        return to.cur - from.cur;
    }

    std::size_t cnt{};
    iterator it{from};
    for ( ; iter_not_equal(it, to); it = iter_next(it), ++cnt )
    {}

    return cnt;
}

namespace details {

// find by key name
inline iterator iter_find(const char *key, std::size_t klen, const iterator &beg, const iterator &end) {
    if ( beg.cur->parent->type != FJ_TYPE_OBJECT ) {
        return end;
    }

    iterator it{beg};
    while ( iter_not_equal(it, end) ) {
        if ( it.type() == FJ_TYPE_OBJECT_END ) {
            return end;
        }
        const auto sv = it.key();
        if ( sv.size() == klen && sv == string_view{key, klen} ) {
            break;
        }

        it = it.is_simple_type()
            ? iterator{it.beg, it.cur + 1, it.end}
            : iterator{it.beg, it.cur->end + 1, it.end}
        ;
    }

    const auto type = it.type();
    switch ( type ) {
        case FJ_TYPE_STRING:
        case FJ_TYPE_NUMBER:
        case FJ_TYPE_BOOL:
        case FJ_TYPE_NULL: {
            return {it.beg, it.cur, it.cur + 1};
        }
        case FJ_TYPE_OBJECT:
        case FJ_TYPE_ARRAY: {
            return {it.cur, it.cur, it.cur->end};
        }
        default: {
            if ( iter_equal(it, end) && it.type() == FJ_TYPE_OBJECT_END ) {
                return end;
            }
        }
    }

    assert(!"unreachable!");

    return end;
}

} // ns details

// at by key name, from the parser
inline iterator iter_at(const char *key, std::size_t klen, const parser *p) {
    bool is_simple = fj_is_simple_type_macro(fj_parser_tokens_beg_ptr(p)->type);
    const iterator beg = {
         fj_parser_tokens_beg_ptr(p)
        ,fj_parser_tokens_beg_ptr(p) + static_cast<std::size_t>(!is_simple)
        ,fj_parser_tokens_end_ptr(p)
    };
    const iterator end = iter_end(p);

    return details::iter_find(key, klen, beg, end);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
iterator iter_at(T key, const parser *p) {
    bool is_simple = fj_is_simple_type_macro(fj_parser_tokens_beg_ptr(p)->type);
    const iterator beg = {
         fj_parser_tokens_beg_ptr(p)
        ,fj_parser_tokens_beg_ptr(p) + static_cast<std::size_t>(!is_simple)
        ,fj_parser_tokens_end_ptr(p)
    };
    const iterator end = iter_end(p);

    return details::iter_find(key, std::strlen(key), beg, end);
}

template<std::size_t N>
iterator iter_at(const char (&key)[N], const parser *p) {
    bool is_simple = fj_is_simple_type_macro(fj_parser_tokens_beg_ptr(p)->type);
    const iterator beg = {
         fj_parser_tokens_beg_ptr(p)
        ,fj_parser_tokens_beg_ptr(p) + static_cast<std::size_t>(!is_simple)
        ,fj_parser_tokens_end_ptr(p)
    };
    const iterator end = iter_end(p);

    return details::iter_find(key, N-1, beg, end);
}

// at by key name, from the iterators
inline iterator iter_at(const char *key, std::size_t klen, const iterator &it) {
    const iterator beg = {
         it.beg
        ,it.cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.end
    };
    const iterator end = iter_end(it);

    return details::iter_find(key, klen, beg, end);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
iterator iter_at(T key, const iterator &it) {
    const iterator beg = {
         it.beg
        ,it.cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.end
    };
    const iterator end = iter_end(it);

    return details::iter_find(key, std::strlen(key), beg, end);
}

template<std::size_t N>
iterator iter_at(const char (&key)[N], const iterator &it) {
    const iterator beg = {
         it.beg
        ,it.cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.end
    };
    const iterator end = iter_end(it);

    return details::iter_find(key, N-1, beg, end);
}

namespace details {

// find by index
inline iterator iter_find(std::size_t idx, const iterator &beg, const iterator &end) {
    if ( beg.cur->parent->type != FJ_TYPE_ARRAY ) {
        return end;
    }
    if ( idx >= beg.cur->parent->childs ) {
        return end;
    }
    if ( beg.cur->parent->flags == 1 ) {
        return {beg.beg, beg.cur + idx, beg.end};
    } else {
        iterator it{beg};
        for ( ; iter_not_equal(it, end) && idx; --idx ) {
            if ( it.type() == FJ_TYPE_ARRAY_END ) {
                return end;
            }

            it = it.is_simple_type()
                ? iterator{it.beg, it.cur + 1, it.end}
                : iterator{it.beg, it.cur->end + 1, it.end}
            ;
        }

        const auto type = it.type();
        switch ( type ) {
            case FJ_TYPE_STRING:
            case FJ_TYPE_NUMBER:
            case FJ_TYPE_BOOL:
            case FJ_TYPE_NULL: {
                return {it.beg, it.cur, it.cur + 1};
            }
            case FJ_TYPE_OBJECT:
            case FJ_TYPE_ARRAY: {
                return {it.cur, it.cur, it.cur->end};
            }
            default: {
                if ( iter_equal(it, end) && it.type() == FJ_TYPE_ARRAY_END ) {
                    return end;
                }
            }
        }
    }

    assert(!"unreachable!");

    return end;
}

} // ns details

// at by index, from a parser
inline iterator iter_at(std::size_t idx, const parser *p) {
    bool is_simple = fj_is_simple_type_macro(fj_parser_tokens_beg_ptr(p)->type);
    const iterator beg = {
         fj_parser_tokens_beg_ptr(p)
        ,fj_parser_tokens_beg_ptr(p) + static_cast<std::size_t>(!is_simple)
        ,fj_parser_tokens_end_ptr(p)
    };
    const iterator end = iter_end(p);

    return details::iter_find(idx, beg, end);
}

// at by index, from a iterator
inline iterator iter_at(std::size_t idx, const iterator &it) {
    const iterator beg = {
         it.beg
        ,it.cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.end
    };
    const iterator end = iter_end(it);

    return details::iter_find(idx, beg, end);
}

/*************************************************************************************************/

namespace details {

#define __FJ_IO_CALL_CB_WITH_CHECK_1(ptr0, size0) \
    callback(userdata, ptr0, size0, nullptr, 0, nullptr, 0, nullptr, 0, 1, size0, ec); \
    if ( *ec ) { return length; }

#define __FJ_IO_CALL_CB_WITH_CHECK_2(ptr0, size0, ptr1, size1) \
    callback(userdata, ptr0, size0, ptr1, size1, nullptr, 0, nullptr, 0, 2, size0 + size1, ec); \
    if ( *ec ) { return length; }

#define __FJ_IO_CALL_CB_WITH_CHECK_3(ptr0, size0, ptr1, size1, ptr2, size2) \
    callback(userdata, ptr0, size0, ptr1, size1, ptr2, size2, nullptr, 0, 3, size0 + size1 + size2, ec); \
    if ( *ec ) { return length; }

#define __FJ_IO_CALL_CB_WITH_CHECK_4(ptr0, size0, ptr1, size1, ptr2, size2, ptr3, size3) \
    callback(userdata, ptr0, size0, ptr1, size1, ptr2, size2, ptr3, size3, 4, size0 + size1 + size2 + size3, ec); \
    if ( *ec ) { return length; }

template<
     bool CalcLength
    ,bool WithIndentation
>
std::size_t walk_through_tokens(
     const token *toksbeg
    ,const token *toksend
    ,std::size_t indent
    ,void *userdata
    ,void(*callback)(
         void *userdata
        ,const void *ptr0
        ,std::size_t size0
        ,const void *ptr1
        ,std::size_t size1
        ,const void *ptr2
        ,std::size_t size2
        ,const void *ptr3
        ,std::size_t size3
        ,std::size_t num
        ,std::size_t total_bytes
        ,int *ec
    )
    ,int *ec)
{
    static const char indent_str[] = "                                                                                ";
    std::size_t indent_scope = 0;
    std::size_t length = 0;
    for ( auto *it = toksbeg; it != toksend + 1; ++it ) {
        if ( it != toksbeg
             && ((it-1)->type != FJ_TYPE_OBJECT && (it-1)->type != FJ_TYPE_ARRAY)
             && (it->type != FJ_TYPE_ARRAY_END && it->type != FJ_TYPE_OBJECT_END ) )
        {
            __FJ__CONSTEXPR_IF( !CalcLength ) {
                __FJ__CONSTEXPR_IF ( WithIndentation ) {
                    __FJ_IO_CALL_CB_WITH_CHECK_1(",\n", 2);
                } else {
                    __FJ_IO_CALL_CB_WITH_CHECK_1(",", 1);
                }
            }
            length += 1;
            __FJ__CONSTEXPR_IF ( WithIndentation ) {
                length += 1; // for '\n'
            }
        }

        switch ( it->type ) {
            case FJ_TYPE_OBJECT: {
                if ( it->key ) {
                    __FJ__CONSTEXPR_IF ( !CalcLength ) {
                        __FJ__CONSTEXPR_IF ( WithIndentation ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_1(indent_str, indent_scope);
                        }
                        __FJ_IO_CALL_CB_WITH_CHECK_3("\"", 1, it->key, it->klen, "\":", 2);
                    }
                    __FJ__CONSTEXPR_IF ( WithIndentation ) {
                        length += indent_scope;
                    }
                    length += 1 + it->klen + 2;
                }
                __FJ__CONSTEXPR_IF ( !CalcLength ) {
                    __FJ__CONSTEXPR_IF ( WithIndentation ) {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("{\n", 2);
                    } else {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("{", 1);
                    }
                }
                length += 1;
                __FJ__CONSTEXPR_IF ( WithIndentation ) {
                    length += 1;
                    indent_scope += indent;
                }
                break;
            }
            case FJ_TYPE_OBJECT_END: {
                __FJ__CONSTEXPR_IF ( !CalcLength ) {
                    __FJ__CONSTEXPR_IF ( WithIndentation ) {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("\n", 1);
                        __FJ_IO_CALL_CB_WITH_CHECK_1(indent_str, indent_scope - indent);
                    }
                    __FJ_IO_CALL_CB_WITH_CHECK_1("}", 1);
                }
                length += 1; // for '}'
                __FJ__CONSTEXPR_IF ( WithIndentation ) {
                    length += 1; // for '\n'
                    indent_scope -= indent;
                    length += indent_scope;
                }
                break;
            }
            case FJ_TYPE_ARRAY: {
                if ( it->key ) {
                    __FJ__CONSTEXPR_IF ( !CalcLength ) {
                        __FJ__CONSTEXPR_IF ( WithIndentation ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_1(indent_str, indent_scope);
                        }
                        __FJ_IO_CALL_CB_WITH_CHECK_3("\"", 1, it->key, it->klen, "\":", 2);
                    }
                    length += 1;
                    length += it->klen;
                    length += 2;
                    __FJ__CONSTEXPR_IF ( WithIndentation ) {
                        length += indent_scope;
                    }
                }
                __FJ__CONSTEXPR_IF ( !CalcLength ) {
                    __FJ__CONSTEXPR_IF ( WithIndentation ) {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("[\n", 2);
                    } else {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("[", 1);
                    }
                }
                length += 1;
                __FJ__CONSTEXPR_IF ( WithIndentation ) {
                    length += 1;
                    indent_scope += indent;
                }
                break;
            }
            case FJ_TYPE_ARRAY_END: {
                __FJ__CONSTEXPR_IF ( !CalcLength ) {
                    __FJ__CONSTEXPR_IF ( WithIndentation ) {
                        __FJ_IO_CALL_CB_WITH_CHECK_3("\n", 1, indent_str, indent_scope - indent, "]", 1);
                    } else {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("]", 1);
                    }
                }
                length += 1; // for ']'
                __FJ__CONSTEXPR_IF ( WithIndentation ) {
                    length += 1; // for '\n'
                    indent_scope -= indent;
                    length += indent_scope;
                }
                break;
            }
            case FJ_TYPE_NULL:
            case FJ_TYPE_BOOL:
            case FJ_TYPE_NUMBER:
            case FJ_TYPE_STRING: {
                if ( it->parent->type != FJ_TYPE_ARRAY ) {
                    __FJ__CONSTEXPR_IF ( !CalcLength ) {
                        __FJ__CONSTEXPR_IF ( WithIndentation ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_4(indent_str, indent_scope, "\"", 1, it->key, it->klen, "\":", 2);
                        } else {
                            __FJ_IO_CALL_CB_WITH_CHECK_3("\"", 1, it->key, it->klen, "\":", 2);
                        }
                    }
                    length += 1 + it->klen + 2;
                    __FJ__CONSTEXPR_IF ( WithIndentation ) {
                        length += indent_scope;
                    }
                } else if ( it->parent->type == FJ_TYPE_ARRAY ) {
                    __FJ__CONSTEXPR_IF ( !CalcLength ) {
                        __FJ__CONSTEXPR_IF ( WithIndentation ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_1(indent_str, indent_scope);
                        }
                    }
                    __FJ__CONSTEXPR_IF ( WithIndentation ) {
                        length += indent_scope;
                    }
                }
                switch ( it->type ) {
                    case FJ_TYPE_NULL:
                    case FJ_TYPE_BOOL:
                    case FJ_TYPE_NUMBER: {
                        __FJ__CONSTEXPR_IF ( !CalcLength ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_1(it->val, it->vlen);
                        }
                        length += it->vlen;
                        break;
                    }
                    case FJ_TYPE_STRING: {
                        __FJ__CONSTEXPR_IF ( !CalcLength ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_3("\"", 1, it->val, it->vlen, "\"", 1);
                        }
                        length += 1;
                        length += it->vlen;
                        length += 1;
                        break;
                    }
                    default: break;
                }

                break;
            }
            default: break;
        }
    }

    return length;
}

#undef __FJ_IO_CALL_CB_WITH_CHECK_1
#undef __FJ_IO_CALL_CB_WITH_CHECK_2
#undef __FJ_IO_CALL_CB_WITH_CHECK_3
#undef __FJ_IO_CALL_CB_WITH_CHECK_4

} // ns details

/*************************************************************************************************/

namespace details {

inline std::size_t walk_through_keys(
     iterator it
    ,const iterator &end
    ,void *userdata
    ,void(*cb)(void *userdata, const char *ptr, std::size_t len))
{
    std::size_t cnt{};

    if ( !it.is_object() ) {
        return 0;
    }

    it.cur += 1;
    for ( ; iter_not_equal(it, end); it = iter_next(it) ) {
        if ( cb ) {
            auto key = it.key();
            cb(userdata, key.data(), key.size());
        }

        ++cnt;
    }

    return cnt;
}

} // ns details

inline std::vector<string_view> get_keys(const iterator &it, const iterator &end) {
    auto num = details::walk_through_keys(it, end, nullptr, nullptr);
    std::vector<string_view> res;
    res.reserve(num);

    static const auto get_fj_keys_cb = [](void *userdata, const char *ptr, std::size_t len) {
        auto *vec = static_cast<std::vector<string_view> *>(userdata);
        vec->push_back(string_view{ptr, len});
    };

    details::walk_through_keys(it, end, &res, get_fj_keys_cb);

    return res;
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

enum class compare_mode {
     markup_only // just compare JSON structure and keys only.
    ,length_only // 'markup_only' + compare a length of keys and values.
    ,full        // 'markup_only' + 'length_only' + compare each value for equality.
};

enum class compare_result {
     equal
    ,type    // differs in token type.
    ,key     // differs in key. (for objects only)
    ,no_key  // differs in key, a key doesnt exists (for objects only)
    ,length  // differs in length.
    ,value   // differs in values.
    ,longer  // the right JSON are longer.
    ,shorter // the right JSON are shorter.
};

inline const char* compare_result_string(compare_result r) {
    switch ( r ) {
        case compare_result::equal: return "equal";
        case compare_result::type: return "tokens types do not match";
        case compare_result::key: return "values of keys do not match";
        case compare_result::no_key: return "no required key";
        case compare_result::length: return "length of values do not match";
        case compare_result::value: return "value do not match";
        case compare_result::longer: return "the right-side JSON is longer";
        case compare_result::shorter: return "the right-side JSON is shorter";
    }

    assert(!"unreachable!");

    return nullptr;
}

inline compare_result compare_impl(
     iterator *left_diff_ptr
    ,iterator *right_diff_ptr
    ,const iterator &left_beg
    ,const iterator &left_end
    ,const iterator &right_beg
    ,const iterator &right_end
    ,compare_mode cmpmode = compare_mode::markup_only)
{
    const bool in_array = left_beg.parent()->type == FJ_TYPE_ARRAY;
    const bool only_simple = left_beg.parent()->flags;
    if ( in_array && only_simple ) {
        using comparator_fnptr = compare_result(*)(const token *l, const token *r);
        static const comparator_fnptr cmparr[3] = {
             [](const token *l, const token *r)
             { return l->type == r->type ? compare_result::equal : compare_result::type; }
            ,[](const token *l, const token *r)
             { return l->vlen == r->vlen ? compare_result::equal : compare_result::length; }
            ,[](const token *l, const token *r)
             { return string_view{l->val, l->vlen} == string_view{r->val, r->vlen}
                ? compare_result::equal
                : compare_result::value;
             }
        };
        for ( const auto *lit = left_beg.cur, *rit = right_beg.cur; lit != left_beg.end; ++lit, ++rit ) {
            auto res = cmparr[static_cast<unsigned>(cmpmode)](lit, rit);
            if ( res != compare_result::equal ) {
                return res;
            }
        }
    } else {
        for ( auto it = left_beg; iter_not_equal(it, left_end); it = iter_next(it) ) {
            const iterator found = in_array
                ? details::iter_find(iter_distance(left_beg, it), right_beg, right_end)
                : details::iter_find(it.key().data(), it.key().size(), right_beg, right_end)
            ;
            if ( iter_equal(found, right_end) ) {
                *left_diff_ptr = it;

                return compare_result::no_key;
            }

            if ( it.type() != found.type() ) {
                *left_diff_ptr = it;
                *right_diff_ptr= found;

                return compare_result::type;
            }

            if ( !it.is_simple_type() ) {
                if ( it.members() != found.members() ) {
                    *left_diff_ptr = it;
                    *right_diff_ptr= found;

                    return (it.members() < found.members())
                        ? compare_result::longer
                        : compare_result::shorter
                    ;
                }

                auto left_next_beg  = iter_begin(it);
                auto left_next_end  = iter_end(it);
                auto right_next_beg = iter_begin(found);
                auto right_next_end = iter_end(found);
                auto res = it.is_array()
                    ? compare_impl(
                         left_diff_ptr, right_diff_ptr
                        ,iter_next(left_next_beg), left_next_end
                        ,iter_next(right_next_beg), right_next_end, cmpmode)
                    : compare_impl(
                         left_diff_ptr, right_diff_ptr
                        ,iter_next(left_next_beg), left_next_end
                        ,iter_next(right_next_beg), right_next_end, cmpmode)
                ;
                if ( res != compare_result::equal ) {
                    return res;
                }
            } else {
                auto res = cmpmode == compare_mode::full
                    ? it.value() == found.value()
                        ? compare_result::equal
                        : compare_result::value
                    : cmpmode == compare_mode::length_only
                        ? it.value().size() == found.value().size()
                            ? compare_result::equal
                            : compare_result::length
                        : it.type() == found.type()
                            ? compare_result::equal
                            : compare_result::type
                ;
                if ( res != compare_result::equal ) {
                    *left_diff_ptr = it;
                    *right_diff_ptr = found;

                    return res;
                }
            }
        }
    }

    return compare_result::equal;
}

inline compare_result compare(
     iterator *left_diff_ptr
    ,iterator *right_diff_ptr
    ,const parser *left_parser
    ,const parser *right_parser
    ,compare_mode cmpr = compare_mode::markup_only)
{
    auto ltokens = fj_parser_tokens_cur_ptr(left_parser) - fj_parser_tokens_beg_ptr(left_parser);
    auto rtokens = fj_parser_tokens_cur_ptr(right_parser) - fj_parser_tokens_beg_ptr(right_parser);
    if ( ltokens != rtokens ) {
        return (ltokens < rtokens)
            ? compare_result::longer
            : compare_result::shorter
        ;
    }

    if ( fj_parser_tokens_beg_ptr(left_parser)->type != fj_parser_tokens_beg_ptr(right_parser)->type ) {
        return compare_result::type;
    }

    auto left_beg  = iter_begin(left_parser);
    auto left_end  = iter_end  (left_parser);
    auto right_beg = iter_begin(right_parser);
    auto right_end = iter_end  (right_parser);
    if ( !left_beg.is_simple_type() ) {
        auto lchilds = fj_parser_tokens_beg_ptr(left_parser)->childs;
        auto rchilds = fj_parser_tokens_beg_ptr(right_parser)->childs;
        if ( lchilds != rchilds ) {
            return (lchilds < rchilds)
                ? compare_result::longer
                : compare_result::shorter
            ;
        }
    } else {
        return cmpr == compare_mode::full
            ? left_beg.value() == right_beg.value()
                ? compare_result::equal
                : compare_result::value
            : cmpr == compare_mode::length_only
                ? left_beg.value().size() == right_beg.value().size()
                    ? compare_result::equal
                    : compare_result::length
                : left_beg.type() == right_beg.type()
                    ? compare_result::equal
                    : compare_result::type
        ;
    }

    return left_beg.is_array()
        ? compare_impl(left_diff_ptr, right_diff_ptr
            ,iter_next(left_beg), left_end
            ,iter_next(right_beg), right_end, cmpr)
        : compare_impl(left_diff_ptr, right_diff_ptr
            ,iter_next(left_beg), left_end
            ,iter_next(right_beg), right_end, cmpr)
    ;
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

#ifndef FJ_NO_TOPLEV_FJSON

struct fjson {
    struct const_iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = const iterator;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using reference = value_type &;
        using const_reference = const value_type &;

        explicit const_iterator(iterator it)
            :m_it{std::move(it)}
        {}

        const_pointer operator->() const { return &m_it; }
        const_iterator& operator++() { m_it = iter_next(m_it); return *this; }
        const_iterator operator++(int) { const_iterator tmp{*this}; ++(*this); return tmp; }
        reference operator* () { return m_it; }
        const_reference operator* () const { return m_it; }
        friend bool operator== (const const_iterator &l, const const_iterator &r)
        { return iter_equal(l.m_it, r.m_it); }
        friend bool operator!= (const const_iterator &l, const const_iterator &r)
        { return !operator==(l, r); }

        iterator m_it;
    };

private:
    struct intrusive_ptr {
        using deleter_fn_ptr = void(*)(parser *);

        intrusive_ptr(bool manage, parser *parser, deleter_fn_ptr free_fn)
            :m_manage{manage}
            ,m_parser{parser}
            ,m_free_fn{free_fn}
        {
            if ( m_manage && m_parser ) {
                fj_parser_refcnt_increment(m_parser);
            }
        }
        virtual ~intrusive_ptr() {
            if ( m_manage && m_parser ) {
                auto refcnt = fj_parser_refcnt_decrement(m_parser);
                if ( !refcnt ) {
                    m_free_fn(m_parser);
                }
            }
        }
        intrusive_ptr(const intrusive_ptr &other)
            :m_manage{other.m_manage}
            ,m_parser{other.m_parser}
            ,m_free_fn{other.m_free_fn}
        {
            if ( m_manage ) {
                fj_parser_refcnt_increment(m_parser);
            }
        }
        intrusive_ptr(intrusive_ptr &&other)
            :m_manage{other.m_manage}
            ,m_parser{other.m_parser}
            ,m_free_fn{other.m_free_fn}
        {
            other.m_manage  = false;
            other.m_parser  = nullptr;
            other.m_free_fn = nullptr;
        }
        intrusive_ptr& operator= (const intrusive_ptr &r) {
            m_manage  = r.m_manage;
            m_parser  = r.m_parser;
            m_free_fn = r.m_free_fn;

            if ( m_manage ) {
                fj_parser_refcnt_increment(m_parser);
            }

            return *this;
        }
        intrusive_ptr& operator= (intrusive_ptr &&r) {
            m_manage    = r.m_manage;
            m_parser    = r.m_parser;
            m_free_fn   = r.m_free_fn;
            r.m_manage  = false;
            r.m_parser  = nullptr;
            r.m_free_fn = nullptr;

            return *this;
        }

        explicit operator bool()       const { return m_parser != nullptr; }
        const parser* get()         const { return m_parser; }
        parser*       get()               { return m_parser; }
        const parser* operator-> () const { return m_parser; }
        parser*       operator-> ()       { return m_parser; }
        const parser& operator*  () const { return *m_parser; }
        parser&       operator*  ()       { return *m_parser; }

    private:
        bool m_manage;
        parser *m_parser;
        deleter_fn_ptr m_free_fn;
    };

public:
    const_iterator begin()  const { return const_iterator{m_beg}; }
    const_iterator end()    const { return const_iterator{m_end}; }
    const_iterator cbegin() const { return const_iterator{m_beg}; }
    const_iterator cend()   const { return const_iterator{m_end}; }

    fjson(const fjson &) = default;
    fjson& operator= (const fjson &) = default;
    fjson(fjson &&) = default;
    fjson& operator= (fjson &&) = default;

    fjson()
        :m_parser{false, nullptr, nullptr}
        ,m_beg{}
        ,m_end{}
    {}

    // construct using user-provided already initialized parser
    fjson(parser *p)
        :m_parser{false, p, [](parser *){}}
        ,m_beg{iter_begin(p)}
        ,m_end{iter_end(p)}
    {
        assert(flatjson::is_valid(p));
    }

    // construct and parse using user-provided array of tokens and parser
    fjson(
         parser *p
        ,token *toksbeg
        ,token *toksend
        ,const char *strbeg
        ,const char *strend
    )
        :m_parser{
             false
            ,(*p = make_parser(toksbeg, toksend, strbeg, strend), p)
            ,[](parser *){}
        }
        ,m_beg{}
        ,m_end{}
    {
        parse(m_parser.get());
        if ( flatjson::is_valid(m_parser.get()) ) {
            m_beg = iter_begin(m_parser.get());
            m_end = iter_end(m_parser.get());
        }
    }

    // construct and parse using user-provided array of tokens and dyn-allocated parser
    fjson(
         token *toksbeg
        ,token *toksend
        ,const char *strbeg
        ,const char *strend
    )
        :m_parser{true, alloc_parser(toksbeg, toksend, strbeg, strend), free_parser}
        ,m_beg{}
        ,m_end{}
    {
        parse(m_parser.get());
        if ( flatjson::is_valid(m_parser.get()) ) {
            m_beg = iter_begin(m_parser.get());
            m_end = iter_end(m_parser.get());
        }
    }

    // construct and parse using user-provided parser and dyn-allocated tokens
    fjson(
         parser *p
        ,const char *strbeg
        ,const char *strend
        ,bool despaced_json = false
        ,alloc_fnptr alloc_fn = &malloc
        ,free_fnptr free_fn = &free
    )
        :m_parser{
             true
            ,(*p = make_parser(strbeg, strend, despaced_json, alloc_fn, free_fn), p)
            ,free_parser
        }
        ,m_beg{}
        ,m_end{}
    {
        parse(m_parser.get());
        if ( flatjson::is_valid(m_parser.get()) ) {
            m_beg = iter_begin(m_parser.get());
            m_end = iter_end(m_parser.get());
        }
    }

    // construct and parse using dyn-allocated tokens and dyn-allocated parser
    fjson(
         const char *strbeg
        ,const char *strend
        ,bool despaced_json = false
        ,alloc_fnptr alloc_fn = &malloc
        ,free_fnptr free_fn = &free
    )
        :m_parser{true
            ,alloc_parser(strbeg, strend, despaced_json, alloc_fn, free_fn)
            ,free_parser}
        ,m_beg{}
        ,m_end{}
    {
        parse(m_parser.get());
        if ( flatjson::is_valid(m_parser.get()) ) {
            m_beg = iter_begin(m_parser.get());
            m_end = iter_end(m_parser.get());
        }
    }

    virtual ~fjson() = default;

private:
    fjson(intrusive_ptr p, iterator beg, iterator end)
        :m_parser{std::move(p)}
        ,m_beg{std::move(beg)}
        ,m_end{std::move(end)}
    {}

public:
    bool is_valid() const { return m_parser && flatjson::is_valid(m_parser.get()); }
    int error() const { return m_parser->error.code; }
    const char* error_string() const { return flatjson::error_message(m_parser->error); }

    // number of direct childs for OBJECT/ARRAY, or 1 for a valid SIMPLE type
    std::size_t size() const { return m_beg.members(); }
    std::size_t members() const { return m_beg.members(); }
    bool is_empty() const { return size() == 0; }

    // total number of tokens
    std::size_t tokens() const
    { return fj_parser_tokens_cur_ptr(m_parser) - fj_parser_tokens_beg_ptr(m_parser); }

    token_type type() const { return m_beg.type(); }
    const char* type_name() const { return m_beg.type_name(); }

    bool is_array() const { return m_beg.is_array(); }
    bool is_object() const { return m_beg.is_object(); }
    bool is_null() const { return m_beg.is_null(); }
    bool is_bool() const { return m_beg.is_bool(); }
    bool is_number() const { return m_beg.is_number(); }
    bool is_string() const { return m_beg.is_string(); }
    bool is_simple_type() const { return m_beg.is_simple_type(); }

    string_view to_string_view() const { return m_beg.to_string_view(); }
    std::string to_string() const { return m_beg.to_string(); }
    template<typename T>
    T to() const { return m_beg.template to<T>(); }
    bool to_bool() const { return m_beg.to_bool(); }
    std::uint32_t to_uint() const { return m_beg.to_uint(); }
    std::int32_t to_int() const { return m_beg.to_int(); }
    std::uint64_t to_uint64() const { return m_beg.to_uint64(); }
    std::int64_t to_int64() const { return m_beg.to_int64(); }
    double to_double() const { return m_beg.to_double(); }
    float to_float() const { return m_beg.to_float(); }

    template<std::size_t N>
    bool contains(const char (&key)[N]) const { return contains(key, N-1); }
    template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
    bool contains(T key) const { return contains(key, std::strlen(key)); }
    bool contains(const char *key, std::size_t len) const
    { auto it = iter_at(key, len, m_beg); return iter_not_equal(it, m_end); }

    // for objects
    template<std::size_t N>
    fjson at(const char (&key)[N]) const { return at(key, N-1); }
    template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
    fjson at(T key) const { return at(key, std::strlen(key)); }
    fjson at(const char *key, std::size_t len) const {
        auto it = iter_at(key, len, m_beg);
        if ( iter_equal(it, m_end) ) {
            return {m_parser, m_end, m_end};
        }

        return {m_parser, it, iter_end(it)};
    }
    // for arrays
    fjson at(std::size_t idx) const {
        auto it = iter_at(idx, m_beg);
        if ( iter_equal(it, m_end) ) {
            return {m_parser, m_end, m_end};
        }

        return {m_parser, it, iter_end(it)};
    }

    // get a fjson object at iterator position
    fjson at(const const_iterator &it) const
    { return {m_parser, iter_begin(it.m_it), iter_end(it.m_it)}; }

    // for arrays
    fjson operator[](std::size_t idx) const { return at(idx); }

    // for objects
    template<std::size_t N>
    fjson operator[](const char (&key)[N]) const { return at(key, N-1); }
    template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
    fjson operator[](T key) const { return at(key, std::strlen(key)); }

    // for top level object/array only
    std::size_t keys_num() const
    { return details::walk_through_keys(m_beg, m_end, nullptr, nullptr); }
    std::vector<string_view>
    keys() const
    { return get_keys(m_beg, m_end); }

private:
    intrusive_ptr m_parser;
    iterator   m_beg;
    iterator   m_end;
};

inline std::size_t distance(const fjson::const_iterator &from, const fjson::const_iterator &to) {
    return iter_distance(from.m_it, to.m_it);
}

/*************************************************************************************************/

// dyn tokens and dyn parser
inline fjson pparse(
     const char *beg
    ,const char *end
    ,bool despaced_json = false
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    return fjson{beg, end, despaced_json, alloc_fn, free_fn};
}

inline fjson pparse(
     const char *beg
    ,bool despaced_json = false
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    const auto *end = beg + std::strlen(beg);

    return pparse(beg, end, despaced_json, alloc_fn, free_fn);
}

// user-provided parser and dyn tokens
inline fjson pparse(
     parser *p
    ,const char *beg
    ,const char *end
    ,bool despaced_json = false
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    return fjson{p, beg, end, despaced_json, alloc_fn, free_fn};
}

inline fjson pparse(
     parser *p
    ,const char *beg
    ,bool despaced_json = false
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    const auto *end = beg + std::strlen(beg);

    return pparse(p, beg, end, despaced_json, alloc_fn, free_fn);
}

// user-provided parser and tokens
inline fjson pparse(
     parser *p
    ,token *toksbeg
    ,token *toksend
    ,const char *strbeg
    ,const char *strend)
{
    return fjson{p, toksbeg, toksend, strbeg, strend};
}

inline fjson pparse(
     parser *p
    ,token *toksbeg
    ,token *toksend
    ,const char *beg)
{
    const auto *end = beg + std::strlen(beg);

    return pparse(p, toksbeg, toksend, beg, end);
}

#endif // FJ_NO_TOPLEV_FJSON

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

#ifndef FJ_NO_TOPLEV_IO
/*************************************************************************************************/
// declarations

#if defined(__linux__) || defined(__APPLE__)
using file_handle = int;
using char_type = char;
using io_vector = iovec;
#   define __FJ_INIT_IO_VECTOR(vec, ptr, size) \
        vec.iov_base = ptr; vec.iov_len = size;
#   define __FJ_IO_VECTOR_PTR(vec) vec->iov_base
#   define __FJ_IO_VECTOR_SIZE(vec) vec->iov_len
#elif defined(WIN32)
using file_handle = HANDLE;
using char_type = TCHAR;
struct io_vector {
    const void *iov_base;
    std::size_t iov_len;
};
#   define __FJ_INIT_IO_VECTOR(vec, ptr, size) \
        vec.iov_base = ptr; vec.iov_len = size;
#   define __FJ_IO_VECTOR_PTR(vec) vec->iov_base
#   define __FJ_IO_VECTOR_SIZE(vec) vec->iov_len
#else
#   error "UNKNOWN PLATFORM!"
#endif // OS detection

inline bool file_exists(const char_type *fname);
inline std::size_t file_size(const char_type *fname, int *ec = nullptr);
inline std::size_t file_size(file_handle fd, int *ec = nullptr);
inline std::size_t file_chsize(file_handle fd, std::size_t fsize, int *ec = nullptr);
inline bool file_handle_valid(file_handle fh);
inline file_handle file_open(const char_type *fname, int *ec = nullptr);
inline file_handle file_create(const char_type *fname, int *ec = nullptr);
inline std::size_t file_read(file_handle fd, void *ptr, std::size_t size, int *ec = nullptr);
inline std::size_t file_write(file_handle fd, const void *ptr, std::size_t size, int *ec = nullptr);
inline std::size_t file_write(
     file_handle fd
    ,const io_vector *iovector
    ,std::size_t num
    ,std::size_t total_bytes
    ,int *ec = nullptr
);
inline bool file_close(file_handle fd, int *ec = nullptr);
inline const void* mmap_for_read(file_handle fd, std::size_t size, int *ec = nullptr);
inline const void* mmap_for_read(file_handle *fd, std::size_t *fsize, const char_type *fname, int *ec = nullptr);
inline void* mmap_for_write(file_handle fd, std::size_t size, int *ec = nullptr);
inline void* mmap_for_write(file_handle *fd, const char_type *fname, std::size_t size, int *ec = nullptr);
inline bool munmap_file(const void *addr, std::size_t size, int *ec = nullptr);
inline bool munmap_file_fd(const void *addr, file_handle fd, int *ec = nullptr);

/*************************************************************************************************/
// implementations

#if defined(__linux__) || defined(__APPLE__)
bool file_exists(const char_type *fname) {
    return ::access(fname, 0) == 0;
}

std::size_t file_size(const char_type *fname, int *ec) {
    struct ::stat st;
    if ( ::stat(fname, &st) != 0 ) {
        *ec = errno;
        return 0;
    }

    return st.st_size;
}

std::size_t file_size(file_handle fd, int *ec) {
    struct ::stat st;
    if ( ::fstat(fd, &st) != 0 ) {
        if ( ec ) { *ec = errno; }
        return 0;
    }

    return st.st_size;
}

std::size_t file_chsize(file_handle fd, std::size_t fsize, int *ec) {
    if ( ::ftruncate(fd, fsize) != 0 ) {
        if ( ec ) { *ec = errno; }
        return 0;
    }

    int lec{};
    fsize = file_size(fd, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return 0;
    }

    return fsize;
}

bool file_handle_valid(file_handle fh) {
    return fh != -1;
}

file_handle file_open(const char_type *fname, int *ec) {
    int fd = ::open(fname, O_RDONLY);
    if ( fd == -1 ) {
        if ( ec ) { *ec = errno; }
        return -1;
    }

    return fd;
}

file_handle file_create(const char_type *fname, int *ec) {
    int fd = ::open(fname, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    if ( fd == -1 ) {
        if ( ec ) { *ec = errno; }
        return fd;
    }

    return fd;
}

std::size_t file_read(file_handle fd, void *ptr, std::size_t size, int *ec) {
    auto rd = static_cast<std::size_t>(::read(fd, ptr, size));
    if ( rd != size ) {
        if ( ec ) { *ec = errno; }
    }

    return rd;
}

std::size_t file_write(file_handle fd, const void *ptr, std::size_t size, int *ec) {
    auto wr = static_cast<std::size_t>(::write(fd, ptr, size));
    if ( wr != size ) {
        if ( ec ) { *ec = errno; }
    }

    return wr;
}

std::size_t file_write(
     file_handle fd
    ,const io_vector *iovector
    ,std::size_t num
    ,std::size_t total_bytes
    ,int *ec)
{
    auto wr = ::writev(fd, iovector, num);
    if ( wr == -1 ) {
        if ( ec ) { *ec = errno; }

        return 0;
    }
    if ( static_cast<std::size_t>(wr) != total_bytes ) {
        if ( ec ) { *ec = errno; }
    }

    return wr;
}

bool file_close(file_handle fd, int *ec) {
    bool ok = ::close(fd) == 0;
    if ( !ok ) {
        if ( ec ) { *ec = errno; }

        return false;
    }

    return true;
}

const void* mmap_for_read(file_handle fd, std::size_t size, int *ec) {
    void *addr = ::mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    if ( addr == MAP_FAILED ) {
        if ( ec ) { *ec = errno; }

        return nullptr;
    }

    bool ok = ::posix_madvise(addr, size, POSIX_MADV_SEQUENTIAL) == 0;
    if ( !ok ) {
        if ( ec ) { *ec = errno; }

        return nullptr;
    }

    return addr;
}

const void* mmap_for_read(file_handle *fd, std::size_t *fsize, const char_type *fname, int *ec) {
    int lec{};
    int lfd = file_open(fname, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return nullptr;
    }

    *fsize = file_size(lfd);
    auto *addr = mmap_for_read(lfd, *fsize, &lec);
    if ( !addr || lec ) {
        if ( ec ) { *ec = lec; }
        file_close(lfd, &lec);
        *fd = -1;
        return nullptr;
    }
    *fd = lfd;

    return addr;
}

void* mmap_for_write(file_handle fd, std::size_t size, int *ec) {
    void *addr = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if ( addr == MAP_FAILED ) {
        if ( ec ) { *ec = errno; }

        return nullptr;
    }

    bool ok = ::posix_madvise(addr, size, POSIX_MADV_SEQUENTIAL) == 0;
    if ( !ok ) {
        if ( ec ) { *ec = errno; }

        return nullptr;
    }

    return addr;
}

void* mmap_for_write(file_handle *fd, const char_type *fname, std::size_t size, int *ec) {
    int lec{};
    int lfd = file_create(fname, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return nullptr;
    }

    auto fsize = file_chsize(lfd, size, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec;  }
        file_close(lfd, &lec);
        return nullptr;
    }

    auto *addr = mmap_for_write(lfd, fsize, &lec);
    if ( !addr ) {
        if ( ec ) { *ec = lec; }
        file_close(lfd, &lec);
        return nullptr;
    }
    *fd = lfd;

    return addr;
}

bool munmap_file(const void *addr, std::size_t size, int *ec) {
    bool ok = ::munmap(const_cast<void *>(addr), size) == 0;
    if ( !ok ) {
        if ( ec ) { *ec = errno; }

        return false;
    }

    return true;
}

bool munmap_file_fd(const void *addr, file_handle fd, int *ec) {
    int lec{};
    auto fsize = file_size(fd, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return false;
    }
    if ( !munmap_file(addr, fsize, &lec) ) {
        if ( ec ) { *ec = lec; }
        return false;
    }
    if ( !file_close(fd, &lec) ) {
        if ( ec ) { *ec = lec; }
        return false;
    }

    return true;
}

#elif defined(WIN32)

bool file_exists(const char_type *fname) {
    auto attr = ::GetFileAttributes(fname);

    return (attr != INVALID_FILE_ATTRIBUTES &&
        !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

std::size_t file_size(const char_type *fname, int *ec) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if ( !::GetFileAttributesEx(fname, ::GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fad) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    std::size_t fsize = fad.nFileSizeLow;
    return fsize;
}

std::size_t file_size(file_handle fd, int *ec) {
    LARGE_INTEGER fsize;
    if ( !::GetFileSizeEx(fd, &fsize) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    return static_cast<std::size_t>(fsize.QuadPart);
}

std::size_t file_chsize(file_handle fd, std::size_t fsize, int *ec) {
    auto ret = ::SetFilePointer(fd, static_cast<LONG>(fsize), nullptr, FILE_BEGIN);
    if ( ret == INVALID_SET_FILE_POINTER ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }
    bool ok = ::SetEndOfFile(fd);
    if ( !ok ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    return fsize;
}

bool file_handle_valid(file_handle fh) {
    return fh != INVALID_HANDLE_VALUE;
}

file_handle file_open(const char_type *fname, int *ec) {
    file_handle fd = ::CreateFile(
         fname
        ,GENERIC_READ
        ,FILE_SHARE_READ
        ,nullptr
        ,OPEN_EXISTING
        ,FILE_ATTRIBUTE_NORMAL
        ,nullptr
    );
    if ( fd == INVALID_HANDLE_VALUE ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return fd;
    }

    return fd;
}

file_handle file_create(const char_type *fname, int *ec) {
    file_handle fd = ::CreateFile(
         fname
        ,GENERIC_READ|GENERIC_WRITE
        ,FILE_SHARE_WRITE
        ,nullptr
        ,CREATE_ALWAYS
        ,FILE_ATTRIBUTE_NORMAL
        ,nullptr
    );
    if ( fd == INVALID_HANDLE_VALUE ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return fd;
    }

    return fd;
}

std::size_t file_read(file_handle fd, void *ptr, std::size_t size, int *ec) {
    if ( !::ReadFile(fd, ptr, static_cast<DWORD>(size), nullptr, nullptr) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    return size;
}

std::size_t file_write(file_handle fd, const void *ptr, std::size_t size, int *ec) {
    if ( !::WriteFile(fd, ptr, static_cast<DWORD>(size), nullptr, nullptr) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return 0;
    }

    return size;
}

std::size_t file_write(
     file_handle fd
    ,const io_vector *iovector
    ,std::size_t num
    ,std::size_t total_bytes
    ,int *ec)
{
    for ( const auto *it = iovector, *end = iovector + num; it != end; ++it ) {
        int lec{};
        file_write(fd, __FJ_IO_VECTOR_PTR(it), __FJ_IO_VECTOR_SIZE(it), &lec);
        if ( lec ) {
            if ( ec ) { *ec = lec; }

            return 0;
        }
    }

    return total_bytes;
}

bool file_close(file_handle fd, int *ec) {
    if ( !::CloseHandle(fd) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return false;
    }

    return true;
}

const void* mmap_for_read(file_handle fd, std::size_t /*size*/, int *ec) {
    auto map = ::CreateFileMapping(fd, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if ( !map ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return nullptr;
    }

    void *addr = ::MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    if ( !addr ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        ::UnmapViewOfFile(map);

        return nullptr;
    }

    return addr;
}

const void* mmap_for_read(file_handle *fd, std::size_t *fsize, const char_type *fname, int *ec) {
    auto fdd = file_open(fname, ec);
    if ( fdd == INVALID_HANDLE_VALUE ) {
        return nullptr;
    }
    *fd = fdd;

    int lec{};
    auto lfsize = file_size(fdd, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }

        return nullptr;
    }

    if ( fsize ) {
        *fsize = lfsize;
    }

    auto *addr = mmap_for_read(fdd, lfsize, &lec);
    if ( !addr ) {
        if ( ec ) { *ec = lec; }

        return nullptr;
    }

    return addr;
}

void* mmap_for_write(file_handle fd, std::size_t /*size*/, int *ec) {
    auto map = ::CreateFileMapping(fd, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    if ( !map ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return nullptr;
    }

    void *addr = ::MapViewOfFile(map, FILE_MAP_WRITE, 0, 0, 0);
    if ( !addr ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        ::UnmapViewOfFile(map);

        return nullptr;
    }

    return addr;
}

void* mmap_for_write(file_handle *fd, const char_type *fname, std::size_t size, int *ec) {
    int lec{};
    auto lfd = file_create(fname, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec; }
        return nullptr;
    }

    auto fsize = file_chsize(lfd, size, &lec);
    if ( lec ) {
        if ( ec ) { *ec = lec;  }
        file_close(lfd, &lec);
        return nullptr;
    }

    auto *addr = mmap_for_write(lfd, fsize, &lec);
    if ( !addr ) {
        if ( ec ) { *ec = lec; }
        file_close(lfd, &lec);
        return nullptr;
    }
    *fd = lfd;

    return addr;
}

bool munmap_file(const void *addr, std::size_t /*size*/, int *ec) {
    if ( !::UnmapViewOfFile(addr) ) {
        int lec = ::GetLastError();
        if ( ec ) { *ec = lec; }

        return false;
    }

    return true;
}

bool munmap_file_fd(const void *addr, file_handle fd, int *ec) {
    int lec{};
    if ( !munmap_file(addr, std::size_t{}, &lec) ) {
        if ( ec ) { *ec = lec; }
        return false;
    }
    if ( !file_close(fd, &lec) ) {
        if ( ec ) { *ec = lec; }
        return false;
    }

    return true;
}

#else
#   error "UNKNOWN PLATFORM!"
#endif // OS detection

/*************************************************************************************************/

inline std::size_t serialize(
     file_handle fd
    ,const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    static const auto cb = []
    (    void *userdata
        ,const void *ptr0
        ,std::size_t size0
        ,const void *ptr1
        ,std::size_t size1
        ,const void *ptr2
        ,std::size_t size2
        ,const void *ptr3
        ,std::size_t size3
        ,std::size_t num
        ,std::size_t total_bytes
        ,int *ec)
    {
        auto *fd = static_cast<file_handle *>(userdata);
        const io_vector iovector[4] = {
             {const_cast<void *>(ptr0), size0}
            ,{const_cast<void *>(ptr1), size1}
            ,{const_cast<void *>(ptr2), size2}
            ,{const_cast<void *>(ptr3), size3}
        };
        file_write(*fd, iovector, num, total_bytes, ec);
    };

    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<false, true>(
             beg.cur
            ,end.end
            ,indent
            ,&fd
            ,cb
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<false, false>(
             beg.cur
            ,end.end
            ,indent
            ,&fd
            ,cb
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::size_t serialize(
     std::FILE *stream
    ,const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    static const auto cb = []
    (    void *userdata
        ,const void *ptr0
        ,std::size_t size0
        ,const void *ptr1
        ,std::size_t size1
        ,const void *ptr2
        ,std::size_t size2
        ,const void *ptr3
        ,std::size_t size3
        ,std::size_t num
        ,std::size_t /*total_bytes*/
        ,int *ec)
    {
        auto *stream = static_cast<std::FILE*>(userdata);
        const io_vector iovector[4] = {
             {const_cast<void *>(ptr0), size0}
            ,{const_cast<void *>(ptr1), size1}
            ,{const_cast<void *>(ptr2), size2}
            ,{const_cast<void *>(ptr3), size3}
        };
        const auto *vec = static_cast<const io_vector *>(iovector);
        for ( auto *iovend = vec + num; vec != iovend; ++vec ) {
            auto wr = std::fwrite(__FJ_IO_VECTOR_PTR(vec), 1, __FJ_IO_VECTOR_SIZE(vec), stream);
            if ( wr != __FJ_IO_VECTOR_SIZE(vec) ) {
                *ec = errno;

                break;
            }
        }
    };

    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<false, true>(
             beg.cur
            ,end.end
            ,indent
            ,stream
            ,cb
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<false, false>(
             beg.cur
            ,end.end
            ,indent
            ,stream
            ,cb
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::size_t serialize(
     std::ostream &stream
    ,const token *beg
    ,const token *end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    static const auto cb = []
    (    void *userdata
        ,const void *ptr0
        ,std::size_t size0
        ,const void *ptr1
        ,std::size_t size1
        ,const void *ptr2
        ,std::size_t size2
        ,const void *ptr3
        ,std::size_t size3
        ,std::size_t num
        ,std::size_t /*total_bytes*/
        ,int *ec)
    {
        const io_vector iovector[4] = {
             {const_cast<void *>(ptr0), size0}
            ,{const_cast<void *>(ptr1), size1}
            ,{const_cast<void *>(ptr2), size2}
            ,{const_cast<void *>(ptr3), size3}
        };
        auto *stream = static_cast<std::ostream *>(userdata);
        const auto *vec = static_cast<const io_vector *>(iovector);
        for ( auto *iovend = vec + num; vec != iovend; ++vec ) {
            auto ppos = stream->tellp();
            if ( ppos == -1 ) {
                *ec = errno;

                return;
            }

            auto wr = stream->write(
                 static_cast<const char *>(__FJ_IO_VECTOR_PTR(vec))
                ,__FJ_IO_VECTOR_SIZE(vec)
            ).tellp();
            if ( wr == -1 ) {
                *ec = errno;

                return;
            }
            if ( static_cast<std::size_t>(wr - ppos) != __FJ_IO_VECTOR_SIZE(vec) ) {
                *ec = errno;

                return;
            }
        }
    };

    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<false, true>(
             beg
            ,end
            ,indent
            ,&stream
            ,cb
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<false, false>(
             beg
            ,end
            ,indent
            ,&stream
            ,cb
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::size_t serialize(
     std::ostream &stream
    ,const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    return serialize(stream, beg.cur, end.end, indent, ec);
}

inline std::size_t serialize(
     const iterator &beg
    ,const iterator &end
    ,char *buf
    ,std::size_t bufsize
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    struct tokens_to_buf_userdata {
        char *ptr;
        const char *end;
    };
    tokens_to_buf_userdata userdata{buf, buf + bufsize};

    static const auto cb = []
    (    void *userdata
        ,const void *ptr0
        ,std::size_t size0
        ,const void *ptr1
        ,std::size_t size1
        ,const void *ptr2
        ,std::size_t size2
        ,const void *ptr3
        ,std::size_t size3
        ,std::size_t num
        ,std::size_t /*total_bytes*/
        ,int */*ec*/)
    {
        const io_vector iovector[4] = {
             {const_cast<void *>(ptr0), size0}
            ,{const_cast<void *>(ptr1), size1}
            ,{const_cast<void *>(ptr2), size2}
            ,{const_cast<void *>(ptr3), size3}
        };

        auto *puserdata = static_cast<tokens_to_buf_userdata *>(userdata);
        const auto *vec = static_cast<const io_vector *>(iovector);
        for ( auto *iovend = vec + num; vec != iovend; ++vec ) {
            std::memcpy(puserdata->ptr, __FJ_IO_VECTOR_PTR(vec), __FJ_IO_VECTOR_SIZE(vec));
            puserdata->ptr += __FJ_IO_VECTOR_SIZE(vec);
        }
    };

    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<false, true>(
             beg.cur
            ,end.end
            ,indent
            ,&userdata
            ,cb
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<false, false>(
             beg.cur
            ,end.end
            ,indent
            ,&userdata
            ,cb
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::size_t length_for_string(
     const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    int lec{};
    std::size_t wr{};
    if ( indent ) {
        wr = details::walk_through_tokens<true, true>(
             beg.cur
            ,end.end
            ,indent
            ,nullptr
            ,nullptr
            ,&lec
        );
    } else {
        wr = details::walk_through_tokens<true, false>(
             beg.cur
            ,end.end
            ,indent
            ,nullptr
            ,nullptr
            ,&lec
        );
    }
    if ( lec && ec ) {
        *ec = lec;

        return 0;
    }

    return wr;
}

inline std::string to_string(
     const iterator &beg
    ,const iterator &end
    ,std::size_t indent = 0
    ,int *ec = nullptr)
{
    std::string res;
    auto length = length_for_string(beg, end, indent);
    res.resize(length);

    serialize(beg, end, &(res[0]), res.size(), indent, ec);
    if ( ec && *ec ) {
        return std::string{};
    }

    return res;
}

/*************************************************************************************************/

namespace details {

#define fj_bytes_required_macro(v) \
    static_cast<std::uint8_t>( \
        (v < (1u<<16)) \
            ? (v < (1u<<8)) \
                ? (v < (1u<<7)) ? 1u : 2u \
                : 3u \
            : (v < (1u<<24)) ? 4u : 5u \
    )

/*************************************************************************************************/
// iterate through all tokens

// TODO: experimental!

inline bool pack_state_iterate(
     const parser *parser
    ,void *userdata
    ,bool(*callback)(
         void *userdata
        ,std::uint8_t type
        ,std::uint32_t key_off
        ,std::uint32_t key_len
        ,std::uint32_t val_off
        ,std::uint32_t val_len
        ,std::uint32_t parent_off
        ,std::uint32_t childs
        ,std::uint32_t end_off
    ))
{
    const auto *prev = fj_parser_tokens_beg_ptr(parser);
    const char *prev_key = nullptr;
    const char *prev_val = nullptr;
    for ( const auto *it = fj_parser_tokens_beg_ptr(parser);
          it != fj_parser_tokens_cur_ptr(parser);
          prev = it++ )
    {
        prev_key = ((prev && prev->key) ? prev->key : prev_key);
        prev_val = ((prev && prev->val) ? prev->val : prev_val);
        auto offset_key = it->key
            ? static_cast<std::uint32_t>(
                it->key - ((prev_key && it->key) ? prev_key : fj_parser_json_beg_ptr(parser)))
            : 0u
        ;
        auto offset_val = it->val
            ? static_cast<std::uint32_t>(
                it->val - ((prev_val && it->val) ? prev_val : fj_parser_json_beg_ptr(parser)))
            : 0u
        ;
        auto offset_parent = it->parent ? static_cast<std::uint32_t>(it - it->parent) : 0u;
        auto offset_end = it->end ? static_cast<std::uint32_t>(it->end - it) : 0u;

        bool ok = callback(
             userdata
            ,static_cast<std::uint8_t>(it->type)
            ,offset_key
            ,it->klen
            ,offset_val
            ,it->vlen
            ,offset_parent
            ,it->childs
            ,offset_end
        );
        if ( !ok ) {
            return false;
        }
    }

    return true;
}

inline std::size_t packed_state_header_size(const parser *parser) {
    std::size_t header_size =
          sizeof(std::uint32_t) // json string length field
        + fj_parser_json_length(parser) // json string
        + sizeof(std::uint32_t) // number of tokens
    ;

    return header_size;
}

struct pack_state_userdata {
    char *ptr;
    const std::size_t expected;
    std::size_t size;
};

} // ns details

/*************************************************************************************************/
// pack/unpack the internal representation for pass to another node/process

inline std::size_t packed_state_size(const parser *parser) {
    std::size_t header_size = details::packed_state_header_size(parser);

    static const auto cb = [](
         void *userdata
        ,std::uint8_t type
        ,std::uint32_t key_off
        ,std::uint32_t key_len
        ,std::uint32_t val_off
        ,std::uint32_t val_len
        ,std::uint32_t parent_off
        ,std::uint32_t childs
        ,std::uint32_t end_off)
    {
        auto &udcnt = *static_cast<std::uint32_t *>(userdata);
        auto bytes_type          = fj_bytes_required_macro(type);
        auto bytes_key_offset    = fj_bytes_required_macro(key_off);
        auto bytes_key_len       = fj_bytes_required_macro(key_len);
        auto bytes_val_offset    = fj_bytes_required_macro(val_off);
        auto bytes_val_len       = fj_bytes_required_macro(val_len);
        auto bytes_parent_offset = fj_bytes_required_macro(parent_off);
        auto bytes_childs_num    = fj_bytes_required_macro(childs);
        auto bytes_end_offset    = fj_bytes_required_macro(end_off);
        std::uint32_t per_token =
              bytes_type
            + bytes_key_offset
            + bytes_key_len
            + bytes_val_offset
            + bytes_val_len
            + bytes_parent_offset
            + bytes_childs_num
            + bytes_end_offset
        ;

        udcnt += per_token;

        return true;
    };

    std::uint32_t markup_size = 0;
    bool ok = details::pack_state_iterate(parser, static_cast<void *>(&markup_size), cb);

    return ok ? markup_size + header_size : 0;
}

/*************************************************************************************************/

inline std::size_t pack_state(char *dst, std::size_t size, const parser *parser) {
    static const auto cb = [](
         void *userdata
        ,std::uint8_t type
        ,std::uint32_t key_off
        ,std::uint32_t key_len
        ,std::uint32_t val_off
        ,std::uint32_t val_len
        ,std::uint32_t parent_off
        ,std::uint32_t childs
        ,std::uint32_t end_off)
    {
        static const auto writer = [](void *dst, const void *ptr, std::size_t bytes) {
            auto *dstu = static_cast<std::uint8_t *>(dst);
            if ( bytes == 1 ) {
                *dstu = *static_cast<const std::uint8_t *>(ptr);
                *dstu |= (1u<<7);
            } else {
                *dstu = static_cast<std::uint8_t>(bytes);
                std::memcpy(dstu + 1, ptr, bytes - 1);
            }

            return static_cast<char *>(dst) + bytes;
        };

        auto *ud = static_cast<details::pack_state_userdata *>(userdata);
        auto bytes_type          = fj_bytes_required_macro(type);
        auto bytes_key_offset    = fj_bytes_required_macro(key_off);
        auto bytes_key_len       = fj_bytes_required_macro(key_len);
        auto bytes_val_offset    = fj_bytes_required_macro(val_off);
        auto bytes_val_len       = fj_bytes_required_macro(val_len);
        auto bytes_parent_offset = fj_bytes_required_macro(parent_off);
        auto bytes_childs_num    = fj_bytes_required_macro(childs);
        auto bytes_end_offset    = fj_bytes_required_macro(end_off);
        std::size_t per_token =
              bytes_type
            + bytes_key_offset
            + bytes_key_len
            + bytes_val_offset
            + bytes_val_len
            + bytes_parent_offset
            + bytes_childs_num
            + bytes_end_offset
        ;

        if ( ud->size + per_token > ud->expected ) {
            return false;
        }

        ud->ptr = writer(ud->ptr, &type, bytes_type);
        ud->ptr = writer(ud->ptr, &key_off, bytes_key_offset);
        ud->ptr = writer(ud->ptr, &key_len, bytes_key_len);
        ud->ptr = writer(ud->ptr, &val_off, bytes_val_offset);
        ud->ptr = writer(ud->ptr, &val_len, bytes_val_len);
        ud->ptr = writer(ud->ptr, &parent_off, bytes_parent_offset);
        ud->ptr = writer(ud->ptr, &childs, bytes_childs_num);
        ud->ptr = writer(ud->ptr, &end_off, bytes_end_offset);

        ud->size += per_token;

        return true;
    };

    char *ptr = dst;
    std::uint32_t json_str_len = static_cast<std::uint32_t>(fj_parser_json_length(parser));
    std::memcpy(ptr, &json_str_len, sizeof(json_str_len));
    ptr += sizeof(json_str_len);
    std::memcpy(ptr, fj_parser_json_beg_ptr(parser), json_str_len);
    ptr += json_str_len;
    std::uint32_t toks_num = static_cast<std::uint32_t>(fj_parser_tokens_used(parser));
    std::memcpy(ptr, &toks_num, sizeof(toks_num));
    ptr += sizeof(toks_num);

    std::size_t left_bytes = size - details::packed_state_header_size(parser);
    details::pack_state_userdata ud{ptr, left_bytes, 0};
    bool ok = details::pack_state_iterate(parser, static_cast<void *>(&ud), cb);

    return ok ? details::packed_state_header_size(parser) + ud.size : 0;
}

/*************************************************************************************************/

inline bool unpack_state(parser *parser, char *ptr, std::size_t size) {
    const char *end = ptr + size;
    std::uint32_t json_len{};
    std::memcpy(&json_len, ptr, sizeof(json_len));
    ptr += sizeof(json_len);

    fj_parser_json_beg_ptr(parser) = ptr;
    fj_parser_json_cur_ptr(parser) = fj_parser_json_beg_ptr(parser) + json_len;
    fj_parser_json_end_ptr(parser) = fj_parser_json_beg_ptr(parser) + json_len;
    ptr += json_len;

    std::uint32_t num_toks{};
    std::memcpy(&num_toks, ptr, sizeof(num_toks));
    ptr += sizeof(num_toks);

    parser->dyn_parser = false;
    parser->dyn_tokens = true;
    fj_parser_tokens_beg_ptr(parser) = static_cast<token *>(parser->alloc_func(num_toks * sizeof(token)));
    fj_parser_tokens_cur_ptr(parser) = fj_parser_tokens_end_ptr(parser) = fj_parser_tokens_beg_ptr(parser) + num_toks;
    token *prev = nullptr;
    const char *prev_key = nullptr;
    const char *prev_val = nullptr;
    for ( auto *it = fj_parser_tokens_beg_ptr(parser);
          it != fj_parser_tokens_end_ptr(parser);
          prev = it++ )
    {
        static const auto reader = [](std::uint32_t *dst, char *ptr, const char *end) -> char * {
            std::uint8_t v = *reinterpret_cast<std::uint8_t *>(ptr);
            const bool onebyte = static_cast<bool>((v >> 7) & 1u);
            v &= ~(1u << 7);
            if ( onebyte ) {
                *dst = v;

                return ptr + 1;
            }

            if ( ptr + v > end ) {
                return nullptr;
            }

            std::uint32_t res{};
            std::memcpy(&res, ptr, v);

            *dst = res;

            return ptr + v;
        };

        std::uint32_t type{};
        ptr = reader(&type, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t key_off{};
        ptr = reader(&key_off, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t key_len{};
        ptr = reader(&key_len, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t val_off{};
        ptr = reader(&val_off, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t val_len{};
        ptr = reader(&val_len, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t parent_off{};
        ptr = reader(&parent_off, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t childs{};
        ptr = reader(&childs, ptr, end);
        if ( !ptr ) { return false; }
        std::uint32_t end_off{};
        ptr = reader(&end_off, ptr, end);
        if ( !ptr ) { return false; }

        prev_key = ((prev && prev->key) ? prev->key : prev_key);
        prev_val = ((prev && prev->val) ? prev->val : prev_val);
        it->type   = static_cast<token_type>(type);
        it->key    = (key_off ? (prev_key ? prev_key + key_off
                                          : fj_parser_json_beg_ptr(parser) + key_off)
                              : nullptr);
        it->klen   = static_cast<decltype(it->klen)>(key_len);
        it->val    = (val_off ? (prev_val ? prev_val + val_off
                                          : fj_parser_json_beg_ptr(parser) + val_off)
                              : nullptr);
        it->vlen   = static_cast<decltype(it->vlen)>(val_len);
        it->parent = (parent_off ? it - parent_off : nullptr);
        it->childs = static_cast<decltype(it->childs)>(childs);
        it->end    = (end_off ? it + end_off : nullptr);
    }
    parser->error.code = EC_OK;

    return true;
}

/*************************************************************************************************/

#endif // FJ_NO_TOPLEV_IO

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

} // ns flatjson

/*************************************************************************************************/

// undef internally used macro-vars
#undef __FJ__FALLTHROUGH
#undef __FJ__CHECK_OVERFLOW
#undef FJ_KLEN_TYPE
#undef FJ_VLEN_TYPE
#undef FJ_CHILDS_TYPE

/*************************************************************************************************/

#endif // __FLATJSON__FLATJSON_HPP
