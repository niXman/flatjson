
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2021 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__FLATJSON_HPP
#define __FLATJSON__FLATJSON_HPP

// TODO
//#include <iostream>

#include <limits>
#include <ostream>
#include <iterator>
#include <vector>
#include <string>
#include <memory>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

/*************************************************************************************************/

#if defined(__clang__)
#   define __FLATJSON__FALLTHROUGH [[clang::fallthrough]]
#elif defined(__GNUC__)
#   define __FLATJSON__FALLTHROUGH __attribute__ ((fallthrough))
#elif defined(_MSC_VER)
#   define __FLATJSON__FALLTHROUGH
#else
#   error "Unknown compiler"
#endif //

/*************************************************************************************************/

#define __FLATJSON__STRINGIZE_I(x) #x
#define __FLATJSON__STRINGIZE(x) __FLATJSON__STRINGIZE_I(x)

#define __FLATJSON__MAKE_ERROR_MESSAGE(msg) \
    __FILE__ "(" __FLATJSON__STRINGIZE(__LINE__) "): " msg

#ifdef __FLATJSON__DONT_CHECK_OVERFLOW
#   define __FLATJSON__CHECK_OVERFLOW(expr, type, err)
#else
#   define __FLATJSON__CHECK_OVERFLOW(expr, type, err) \
        if ( (expr) >= (std::numeric_limits<type>::max)() ) return err
#endif //__FLATJSON__DONT_CHECK_OVERFLOW

#ifndef __FLATJSON__KLEN_TYPE
#   define __FLATJSON__KLEN_TYPE std::uint8_t
#endif // __FLATJSON__KLEN_TYPE
#ifndef __FLATJSON__VLEN_TYPE
#   define __FLATJSON__VLEN_TYPE std::uint16_t
#endif // __FLATJSON__VLEN_TYPE
#ifndef __FLATJSON__CHILDS_TYPE
#   define __FLATJSON__CHILDS_TYPE std::uint8_t
#endif // __FLATJSON__CHILDS_TYPE

/*************************************************************************************************/

#if __cplusplus >= 201703L
#   define __FLATJSON__CONSTEXPR_IF(...) if constexpr (__VA_ARGS__)
#   include <string_view>
namespace flatjson {
using static_string = std::string_view;
} // ns flatjson
#else
#   define __FLATJSON__CONSTEXPR_IF(...) if (__VA_ARGS__)
namespace flatjson {
namespace details {

/*************************************************************************************************/

struct static_string {
    static_string() = default;
    template<std::size_t N>
    explicit static_string(const char (&str)[N])
        :m_ptr{str}
        ,m_len{N-1}
    {}
    static_string(const char *ptr, std::size_t len)
        :m_ptr{ptr}
        ,m_len{len}
    {}
    static_string(const char *beg, const char *end)
        :m_ptr{beg}
        ,m_len{static_cast<std::size_t>(end-beg)}
    {}

    std::size_t size() const { return m_len; }
    bool empty() const { return size() == 0; }
    const char* data() const { return m_ptr; }

    int compare(const char *r) const { return compare(r, std::strlen(r)); }
    int compare(const static_string &r) const { return compare(r.data(), r.size()); }
    int compare(const char *r, std::size_t len) const { return std::strncmp(m_ptr, r, len); }

    friend bool operator< (const static_string &l, const char *r) { return l.compare(r) < 0; }
    friend bool operator< (const static_string &l, const static_string &r) { return l.compare(r) < 0; }
    friend bool operator> (const static_string &l, const char *r) { return l.compare(r) > 0; }
    friend bool operator> (const static_string &l, const static_string &r) { return l.compare(r) > 0; }
    friend bool operator==(const static_string &l, const char *r) { return l.compare(r) == 0; }
    friend bool operator==(const static_string &l, const static_string &r) { return l.compare(r) == 0; }
    friend bool operator!=(const static_string &l, const char *r) { return !(l == r); }
    friend bool operator!=(const static_string &l, const static_string &r) { return !(l == r); }

    friend std::ostream& operator<< (std::ostream &os, const static_string &s) {
        os.write(s.m_ptr, s.m_len);

        return os;
    }

private:
    const char* m_ptr;
    std::size_t m_len;
};

/*************************************************************************************************/

} // ns details

using static_string = details::static_string;

} // ns flatjson
#endif // __cplusplus >= 201703L

namespace flatjson {

/*************************************************************************************************/

enum e_fj_token_type: std::uint8_t {
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

inline const char *fj_token_type_name(e_fj_token_type t) {
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

    auto idx = static_cast<std::size_t>(t);
    if ( idx < sizeof(strs)/sizeof(strs[0]) ) {
        return strs[idx];
    }

    return "UNKNOWN TYPE";
}

enum e_fj_error_code {
     FJ_EC_OK = 0
    ,FJ_EC_INVALID = -1
    ,FJ_EC_INCOMPLETE = -2
    ,FJ_EC_NO_FREE_TOKENS = -3
    ,FJ_EC_KLEN_OVERFLOW = -4
    ,FJ_EC_VLEN_OVERFLOW = -5
    ,FJ_EC_CHILDS_OVERFLOW = -6
};

inline const char* fj_error_string(e_fj_error_code e) {
    static const char* strs[] = {
         "OK"
        ,"INVALID"
        ,"INCOMPLETE"
        ,"NO_FREE_TOKENS"
        ,"KLEN_OVERFLOW"
        ,"VLEN_OVERFLOW"
        ,"CHILDS_OVERFLOW"
    };
    int idx = static_cast<int>(e);
    idx = -idx;
    if ( static_cast<unsigned>(idx) < sizeof(strs)/sizeof(strs[0]) ) {
        return strs[idx];
    }

    return "UNKNOWN ERROR";
}

/*************************************************************************************************/

namespace details {

#define _FJ_CASE_1(x) \
    case x:

#define _FJ_CASE_3(S) \
    _FJ_CASE_1(S) _FJ_CASE_1(S+1) _FJ_CASE_1(S+2)

#define _FJ_CASE_5(S) \
    _FJ_CASE_3(S) _FJ_CASE_1(S+3) _FJ_CASE_1(S+4)

#define _FJ_CASE_10(S) \
    _FJ_CASE_5(S) _FJ_CASE_5(S+5)

#define _FJ_CASE_20(S) \
    _FJ_CASE_10(S) _FJ_CASE_10(S+10)

#define _FJ_CASE_40(S) \
    _FJ_CASE_20(S) _FJ_CASE_20(S+20)

template<typename CharT>
bool fj_is_simple_type(CharT v) {
    switch ( static_cast<std::uint8_t>(v) ) {
        _FJ_CASE_1(0) \
            return false;
        _FJ_CASE_3(1) \
        _FJ_CASE_1(4) \
            return true;
        default:
            return false;
    }
}

template<typename CharT>
bool fj_is_digit(CharT ch) {
    switch ( static_cast<std::uint8_t>(ch) ) {
        _FJ_CASE_20(0) \
        _FJ_CASE_20(20) \
        _FJ_CASE_5(40) \
        _FJ_CASE_3(45) \
            return false;
        _FJ_CASE_10(48) \
            return true;
        default:
            return false;
    }
}

template<typename CharT>
bool fj_is_hex_digit(CharT ch) {
    if ( fj_is_digit(ch) ) {
        return true;
    }

    switch ( static_cast<std::uint8_t>(ch) ) {
        _FJ_CASE_20(0) \
        _FJ_CASE_20(20) \
        _FJ_CASE_20(40) \
        _FJ_CASE_5(60) \
            return false;
        _FJ_CASE_3(65) \
        _FJ_CASE_3(68) \
            return true;
        _FJ_CASE_20(71) \
        _FJ_CASE_3(91) \
        _FJ_CASE_3(94) \
            return false;
         _FJ_CASE_3(97) \
         _FJ_CASE_3(100) \
            return true;
        default:
            return false;
    }
}

template<typename CharT>
bool fj_is_hex_digit4(CharT ch0, CharT ch1, CharT ch2, CharT ch3) {
    return fj_is_hex_digit(ch0) && fj_is_hex_digit(ch1) && fj_is_hex_digit(ch2) && fj_is_hex_digit(ch3);
}

template<typename CharT>
std::size_t fj_utf8_char_len(CharT ch) {
    switch ( static_cast<std::uint8_t>(ch) ) {
        _FJ_CASE_40(0) \
        _FJ_CASE_40(40) \
        _FJ_CASE_40(80) \
        _FJ_CASE_5 (120) \
        _FJ_CASE_3 (125) \
            return 1;
        _FJ_CASE_40(128) \
        _FJ_CASE_40(168) \
        _FJ_CASE_10(208) \
        _FJ_CASE_5 (218) \
        _FJ_CASE_1 (223) \
            return 2;
        _FJ_CASE_10(224) \
        _FJ_CASE_5 (234) \
        _FJ_CASE_1 (239) \
            return 3;
        _FJ_CASE_10(240) \
        _FJ_CASE_5 (250) \
            return 4;
        default:
            assert("unrechable!" == nullptr);
    }
}

template<typename CharT>
bool fj_is_whitespace_char(CharT ch) {
    switch ( static_cast<std::uint8_t>(ch) ) {
        _FJ_CASE_5(0) \
        _FJ_CASE_3(5) \
        _FJ_CASE_1(8) \
            return false;
        _FJ_CASE_1(9) \
        _FJ_CASE_1(10) \
            return true;
        _FJ_CASE_1(11) \
        _FJ_CASE_1(12) \
            return false;
        _FJ_CASE_1(13) \
            return true;
        _FJ_CASE_10(14) \
        _FJ_CASE_5 (24) \
        _FJ_CASE_3 (29) \
            return false;
        _FJ_CASE_1(32) \
            return true;
        default:
            return false;
    }
}

#undef _FJ_CASE_1
#undef _FJ_CASE_3
#undef _FJ_CASE_5
#undef _FJ_CASE_10
#undef _FJ_CASE_20
#undef _FJ_CASE_40

/*************************************************************************************************/

template<typename To>
static typename std::enable_if<
    (std::is_integral<To>::value && std::is_unsigned<To>::value) &&
    !std::is_same<To, bool>::value, To
>::type
conv_to(const char *ptr, std::size_t len) {
    const auto *str = reinterpret_cast<const std::uint8_t *>(ptr);
    std::uint64_t v = 0;
    switch ( len ) {
        case 20: v = v + (str[len - 20] - '0') * 10000000000000000000ull; __FLATJSON__FALLTHROUGH;
        case 19: v = v + (str[len - 19] - '0') * 1000000000000000000ull; __FLATJSON__FALLTHROUGH;
        case 18: v = v + (str[len - 18] - '0') * 100000000000000000ull; __FLATJSON__FALLTHROUGH;
        case 17: v = v + (str[len - 17] - '0') * 10000000000000000ull; __FLATJSON__FALLTHROUGH;
        case 16: v = v + (str[len - 16] - '0') * 1000000000000000ull; __FLATJSON__FALLTHROUGH;
        case 15: v = v + (str[len - 15] - '0') * 100000000000000ull; __FLATJSON__FALLTHROUGH;
        case 14: v = v + (str[len - 14] - '0') * 10000000000000ull; __FLATJSON__FALLTHROUGH;
        case 13: v = v + (str[len - 13] - '0') * 1000000000000ull; __FLATJSON__FALLTHROUGH;
        case 12: v = v + (str[len - 12] - '0') * 100000000000ull; __FLATJSON__FALLTHROUGH;
        case 11: v = v + (str[len - 11] - '0') * 10000000000ull; __FLATJSON__FALLTHROUGH;
        case 10: v = v + (str[len - 10] - '0') * 1000000000ull; __FLATJSON__FALLTHROUGH;
        case 9 : v = v + (str[len - 9 ] - '0') * 100000000ull; __FLATJSON__FALLTHROUGH;
        case 8 : v = v + (str[len - 8 ] - '0') * 10000000ull; __FLATJSON__FALLTHROUGH;
        case 7 : v = v + (str[len - 7 ] - '0') * 1000000ull; __FLATJSON__FALLTHROUGH;
        case 6 : v = v + (str[len - 6 ] - '0') * 100000ull; __FLATJSON__FALLTHROUGH;
        case 5 : v = v + (str[len - 5 ] - '0') * 10000ull; __FLATJSON__FALLTHROUGH;
        case 4 : v = v + (str[len - 4 ] - '0') * 1000ull; __FLATJSON__FALLTHROUGH;
        case 3 : v = v + (str[len - 3 ] - '0') * 100ull; __FLATJSON__FALLTHROUGH;
        case 2 : v = v + (str[len - 2 ] - '0') * 10ull; __FLATJSON__FALLTHROUGH;
        case 1 : v = v + (str[len - 1 ] - '0') * 1ull; __FLATJSON__FALLTHROUGH;
        default: break;
    }

    return static_cast<To>(v);
}

template<typename To>
static typename std::enable_if<
    (std::is_integral<To>::value && std::is_signed<To>::value) &&
    !std::is_same<To, bool>::value, To
>::type
conv_to(const char *ptr, std::size_t len) {
    if ( *ptr == '-' ) {
        ++ptr;
        return -conv_to<typename std::make_unsigned<To>::type>(ptr, len - 1);
    }

    return conv_to<typename std::make_unsigned<To>::type>(ptr, len);
}

template<typename To>
static typename std::enable_if<std::is_same<To, bool>::value, To>::type
conv_to(const char *ptr, std::size_t len) {
    return *ptr == 't' && len == 4;
}

template<typename To>
static typename std::enable_if<std::is_same<To, double>::value, To>::type
conv_to(const char *ptr, std::size_t len) {
    char buf[std::numeric_limits<To>::max_exponent10 + 20];
    std::memcpy(buf, ptr, len);
    buf[len] = 0;

    return std::strtod(buf, nullptr);
}

template<typename To>
static typename std::enable_if<std::is_same<To, float>::value, To>::type
conv_to(const char *ptr, std::size_t len) {
    char buf[std::numeric_limits<To>::max_exponent10 + 20];
    std::memcpy(buf, ptr, len);
    buf[len] = 0;

    return std::strtof(buf, nullptr);
}

template<typename To>
static typename std::enable_if<std::is_same<To, std::string>::value, To>::type
conv_to(const char *ptr, std::size_t len) { return {ptr, len}; }

template<typename To>
static typename std::enable_if<std::is_same<To, static_string>::value, To>::type
conv_to(const char *ptr, std::size_t len) { return {ptr, len}; }

} // ns details

/*************************************************************************************************/

#ifndef __FLATJSON__DONT_PACK_TOKENS
#pragma pack(push, 1)
#endif // __FLATJSON__DONT_PACK_TOKENS

template<typename Iterator>
struct fj_token {
    fj_token()
        :m_type{}
        ,m_klen{}
        ,m_vlen{}
        ,m_childs{}
        ,m_key{}
        ,m_val{}
        ,m_parent{}
        ,m_end{}
    {}

    e_fj_token_type type() const { return m_type; }
    void type(e_fj_token_type t) { m_type = t; }
    const char* type_name() const { return fj_token_type_name(type()); }

    bool valid() const { return type() != FJ_TYPE_INVALID; }
    bool is_array() const { return type() == FJ_TYPE_ARRAY; }
    bool is_object() const { return type() == FJ_TYPE_OBJECT; }
    bool is_null() const { return type() == FJ_TYPE_NULL; }
    bool is_bool() const { return type() == FJ_TYPE_BOOL; }
    bool is_number() const { return type() == FJ_TYPE_NUMBER; }
    bool is_string() const { return type() == FJ_TYPE_STRING; }
    bool is_simple_type() const { return details::fj_is_simple_type(type()); }

    static_string to_sstring() const {
        if ( is_simple_type() ) {
            if ( !is_null() ) {
                return value();
            }

            return {};
        }

        throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("not STRING/NUMBER/BOOL/NULL type"));
    }
    std::string to_string() const { auto s = to_sstring(); return {s.data(), s.size()}; }
    template<typename T>
    T to() const { auto s = to_sstring(); return details::conv_to<T>(s.data(), s.size()); }
    bool to_bool() const { return to<bool>(); }
    std::uint32_t to_uint() const { return to<std::uint32_t>(); }
    std::int32_t to_int() const { return to<std::int32_t>(); }
    std::uint64_t to_uint64() const { return to<std::uint64_t>(); }
    std::int64_t to_int64() const { return to<std::int64_t>(); }
    double to_double() const { return to<double>(); }
    float to_float() const { return to<float>(); }

    static_string key() const { return {m_key, m_klen}; }
    Iterator const* key_iterator() const { return &m_key; }
    Iterator* key_iterator() { return &m_key; }
    void key(Iterator v) { m_key = v; }

    std::size_t klen() const { return m_klen; }
    void klen(std::size_t v) { m_klen = static_cast<__FLATJSON__KLEN_TYPE>(v); }

    static_string value() const { return {m_val, m_vlen}; }
    Iterator const* value_iterator() const { return &m_val; }
    Iterator* value_iterator() { return &m_val; }
    void value(Iterator v) { m_val = v; }

    std::size_t vlen() const { return m_vlen; }
    void vlen(std::size_t len) { m_vlen = static_cast<__FLATJSON__VLEN_TYPE>(len); }

    std::size_t childs() const { return m_childs; }
    void childs(std::size_t v) { m_childs = static_cast<__FLATJSON__CHILDS_TYPE>(v); }

    std::size_t tokens() const { return end() - this; }
    bool empty() const { return tokens() == 1; }

    const fj_token* parent() const { return m_parent; }
    fj_token* parent() { return m_parent; }

    void parent(fj_token *v) { m_parent = v; }

    fj_token* end() { return m_end; }
    const fj_token* end() const { return m_end; }
    void end(fj_token *v) { m_end = v; }

private:
    e_fj_token_type m_type;
    __FLATJSON__KLEN_TYPE m_klen;
    __FLATJSON__VLEN_TYPE m_vlen;
    __FLATJSON__CHILDS_TYPE m_childs;
    Iterator m_key;
    Iterator m_val;
    fj_token *m_parent;
    fj_token *m_end; // pointing to the last token for arrays and objects
};

#ifndef __FLATJSON__DONT_PACK_TOKENS
#pragma pack(pop)
#endif // __FLATJSON__DONT_PACK_TOKENS

/*************************************************************************************************/

namespace details {

template<typename Iterator, typename Compare>
fj_token<Iterator>* lower_bound(fj_token<Iterator>* left, fj_token<Iterator>* right, fj_token<Iterator> *val, Compare cmp) {
    fj_token<Iterator>* it{};
    std::size_t count = right - left, step;

    while ( count ) {
        it = left;
        step = count / 2;
        it += step;

        bool ok = cmp(it, val);
        if ( ok ) {
            left = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }

    return left;
}

// left - simple, right - simple
template<typename Iterator>
void swap_simple_with_simple(fj_token<Iterator>* dst, fj_token<Iterator>* src) {
    auto tmp = *src;
    *src = *dst;
    *dst = tmp;
}

// input: left - complex, right - simple
template<typename Iterator>
void swap_simple_with_complex(fj_token<Iterator>* complex, fj_token<Iterator>* complex_end, fj_token<Iterator>* simple) {
    auto *complex_parent = complex->parent();
    auto tmp = *simple;
    for ( auto *beg = complex - 1; complex_end != beg; --complex_end ) {
        auto *new_end = complex_end + 1;
        *new_end = *complex_end;
        if ( new_end->end() ) { new_end->end(new_end->end() + 1); }
        if ( new_end->parent() && new_end->parent() != complex_parent ) { new_end->parent(new_end->parent() + 1); }
    }
    *complex = tmp;
}

// input: left - simple, right - complex
template<typename Iterator>
void swap_complex_with_simple(fj_token<Iterator>* simple, fj_token<Iterator>* complex, fj_token<Iterator>* complex_end) {
    auto *complex_parent = complex->parent();
    auto tmp = *simple;
    for ( auto *end = complex_end + 1; complex != end; ++complex  ) {
        auto *new_beg = complex - 1;
        *new_beg = *complex;
        if ( new_beg->end() ) { new_beg->end(new_beg->end() - 1); }
        if ( new_beg->parent() && new_beg->parent() != complex_parent ) { new_beg->parent(new_beg->parent() - 1); }
    }
    *complex_end = tmp;
}

template<typename Iterator>
void swap_complex_with_complex(
     fj_token<Iterator> *complex_dst
    ,fj_token<Iterator> *complex_dst_end
    ,fj_token<Iterator> *complex_src
    ,fj_token<Iterator> *complex_src_end)
{
    auto src_size = complex_src_end - complex_src;
    auto dst_size = complex_dst_end - complex_dst;

    if ( src_size == dst_size ) {
        auto diff = complex_src - complex_dst;
        for ( auto *end = complex_src_end + 1; complex_src != end; ++complex_src, ++complex_dst ) {
            auto tmp = *complex_src;
            *complex_src = *complex_dst;
            if ( complex_src->end() ) { complex_src->end(); }
            *complex_dst = tmp;
        }
    } else if ( src_size < dst_size ) {

    } else {

    }
}

/*************************************************************************************************/

template<typename Iterator>
struct fj_parser {
    Iterator js_cur;
    Iterator js_end;

    fj_token<Iterator> *jstok_beg;
    fj_token<Iterator> *jstok_cur;
    fj_token<Iterator> *jstok_end;
};

/*************************************************************************************************/

template<typename Iterator>
void fj_skip_ws(fj_parser<Iterator> *p) {
    for ( ; p->js_cur < p->js_end && fj_is_whitespace_char(*p->js_cur); ++p->js_cur )
    {}
}

template<typename Iterator>
char fj_current_char(fj_parser<Iterator> *p) {
    fj_skip_ws(p);
    return (p->js_cur >= p->js_end ? ((char)-1) : *(p->js_cur));
}

template<typename Iterator>
int fj_check_and_skip(fj_parser<Iterator> *p, char expected) {
    char ch = fj_current_char(p);
    if ( ch == expected ) {
        p->js_cur++;

        return FJ_EC_OK;
    }

    if ( ch == ((char)-1) ) {
        return FJ_EC_INCOMPLETE;
    }

    return FJ_EC_INVALID;
}

template<typename Iterator>
int fj_escape_len(Iterator s, std::ptrdiff_t len) {
    switch ( *s ) {
        case 'u':
            return len < 6
                ? FJ_EC_INCOMPLETE
                : fj_is_hex_digit4(*(s+1), *(s+2), *(s+3), *(s+4))
                    ? 5
                    : FJ_EC_INVALID
            ;
        case '"':
        case '\\':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
            return len < 2 ? FJ_EC_INCOMPLETE : 1;
        default:
            return FJ_EC_INVALID;
    }
}

/*************************************************************************************************/

enum parser_mode: std::size_t {
     parse          = 1u << 0u
    ,count_tokens   = 1u << 1u
    ,sorted_objects = 1u << 2u
};

template<std::size_t M, typename Iterator, std::size_t ExLen>
int fj_expect(fj_parser<Iterator> *p, const char (&s)[ExLen], Iterator *ptr, std::size_t *size) {
    if ( p->js_cur + (ExLen-1) > p->js_end )
        return FJ_EC_INCOMPLETE;

    if ( std::strncmp(p->js_cur, s, ExLen-1) != 0 ) {
        return FJ_EC_INVALID;
    }

    __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
        *ptr = p->js_cur;
        *size = ExLen-1;
    }

    p->js_cur += ExLen-1;

    return FJ_EC_OK;
}

template<std::size_t M, typename Iterator>
int fj_parse_string(fj_parser<Iterator> *p, Iterator *ptr, std::size_t *size) {
    int ec = fj_check_and_skip(p, '"');
    if ( ec ) {
        return ec;
    }

    int ch = 0;
    Iterator start = p->js_cur;
    for ( std::size_t len = 0; p->js_cur < p->js_end; p->js_cur += len ) {
        ch = static_cast<unsigned char>(*(p->js_cur));
        len = fj_utf8_char_len((unsigned char)ch);
        if ( !(ch >= 32 && len > 0) ) {
            return FJ_EC_INVALID;
        }
        if ( static_cast<std::ptrdiff_t>(len) > (p->js_end - p->js_cur) ) {
            return FJ_EC_INCOMPLETE;
        }

        if ( ch == '\\' ) {
            int n = fj_escape_len(p->js_cur + 1, p->js_end - p->js_cur);
            if ( n <= 0 ) {
                return n;
            }
            len += n;
        } else if ( ch == '"' ) {
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                *ptr = start;
                *size = p->js_cur - start;
            }

            ++p->js_cur;

            break;
        }
    }

    return ch == '"' ? FJ_EC_OK : FJ_EC_INCOMPLETE;
}

template<std::size_t M, typename Iterator>
int fj_parse_number(fj_parser<Iterator> *p, Iterator *ptr, std::size_t *size) {
    Iterator start = p->js_cur;
    if ( fj_current_char(p) == '-' ) {
        p->js_cur++;
    }

    if ( p->js_cur >= p->js_end ) {
        return FJ_EC_INCOMPLETE;
    }
    if ( p->js_cur + 1 < p->js_end && *(p->js_cur) == '0' && *(p->js_cur+1) == 'x' ) {
        p->js_cur += 2;

        if ( p->js_cur >= p->js_end ) {
            return FJ_EC_INCOMPLETE;
        }
        if ( !details::fj_is_hex_digit(*(p->js_cur)) ) {
            return FJ_EC_INVALID;
        }

        for ( ; p->js_cur < p->js_end && details::fj_is_hex_digit(*(p->js_cur)); ++p->js_cur )
            ;
    } else {
        if ( !details::fj_is_digit(*(p->js_cur)) ) {
            return FJ_EC_INVALID;
        }
        for ( ; p->js_cur < p->js_end && details::fj_is_digit(*(p->js_cur)); ++p->js_cur )
            ;

        if ( p->js_cur < p->js_end && *(p->js_cur) == '.' ) {
            p->js_cur++;

            if ( p->js_cur >= p->js_end ) {
                return FJ_EC_INCOMPLETE;
            }
            if ( !details::fj_is_digit(*(p->js_cur)) ) {
                return FJ_EC_INVALID;
            }

            for ( ; p->js_cur < p->js_end && details::fj_is_digit(*(p->js_cur)); ++p->js_cur )
                ;
        }
        if ( p->js_cur < p->js_end && (*(p->js_cur) == 'e' || *(p->js_cur) == 'E') ) {
            p->js_cur++;

            if ( p->js_cur >= p->js_end ) {
                return FJ_EC_INCOMPLETE;
            }

            if ( *(p->js_cur) == '+' || *(p->js_cur) == '-' )
                p->js_cur++;

            if ( p->js_cur >= p->js_end ) {
                return FJ_EC_INCOMPLETE;
            }
            if ( !details::fj_is_digit(*(p->js_cur)) ) {
                return FJ_EC_INVALID;
            }

            for ( ; p->js_cur < p->js_end && details::fj_is_digit(*(p->js_cur)); ++p->js_cur )
                ;
        }
    }

    if ( (p->js_cur - start) > 1 && (start[0] == '0' && start[1] != '.') ) {
        return FJ_EC_INVALID;
    }

    __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
        *ptr = start;
        *size = p->js_cur - start;
    }

    return FJ_EC_OK;
}

template<std::size_t M, typename Iterator>
int fj_parse_value(fj_parser<Iterator> *p, Iterator *ptr, std::size_t *size, e_fj_token_type *toktype, fj_token<Iterator> *parent);

template<std::size_t M, typename Iterator>
int fj_parse_array(fj_parser<Iterator> *p, fj_token<Iterator> *parent) {
    int ec = fj_check_and_skip(p, '[');
    if ( ec ) {
        return ec;
    }

    __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse && p->jstok_cur == p->jstok_end ) {
        return FJ_EC_NO_FREE_TOKENS;
    }

    fj_token<Iterator> *startarr = p->jstok_cur++;
    __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
        startarr->type(FJ_TYPE_ARRAY);
        startarr->parent(parent);
        if ( startarr->parent() ) {
            __FLATJSON__CHECK_OVERFLOW(startarr->parent()->childs(), __FLATJSON__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
            startarr->parent()->childs(startarr->parent()->childs() + 1);
        }
    }

    while ( fj_current_char(p) != ']' ) {
        __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse && p->jstok_cur == p->jstok_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }

        fj_token<Iterator> *pair = p->jstok_cur++;

        char ch = fj_current_char(p);
        if ( ch == '{' || ch == '[' ) {
            p->jstok_cur -= 1;
        } else {
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                pair->parent(startarr);

                __FLATJSON__CHECK_OVERFLOW(startarr->childs(), __FLATJSON__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
                startarr->childs(startarr->childs() + 1);
            }
        }

        std::size_t size = 0;
        e_fj_token_type toktype{};
        ec = fj_parse_value<M>(
             p
            ,pair->value_iterator()
            ,&size
            ,&toktype
            ,startarr
        );
        if ( ec ) {
            return ec;
        }
        __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
            pair->type(toktype);
            __FLATJSON__CHECK_OVERFLOW(size, __FLATJSON__VLEN_TYPE, FJ_EC_VLEN_OVERFLOW);
            pair->vlen(size);
        }

        if ( fj_current_char(p) == ',' ) {
            p->js_cur++;
            if ( *(p->js_cur) == ']' ) {
                return FJ_EC_INVALID;
            }
        }
    }

    ec = fj_check_and_skip(p, ']');
    if ( ec ) {
        return ec;
    }

    __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
        if ( p->jstok_cur == p->jstok_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }
        fj_token<Iterator> *endarr = p->jstok_cur++;
        endarr->type(FJ_TYPE_ARRAY_END);
        endarr->parent(startarr);
        __FLATJSON__CHECK_OVERFLOW(endarr->parent()->childs(), __FLATJSON__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
        endarr->parent()->childs(endarr->parent()->childs() + 1);
        startarr->end(endarr);
    } else {
        ++p->jstok_cur;
    }

    return 0;
}

template<std::size_t M, typename Iterator>
int fj_parse_object(fj_parser<Iterator> *p, fj_token<Iterator> *parent) {
    int ec = fj_check_and_skip(p, '{');
    if ( ec ) {
        return ec;
    }

    if ( (M & parser_mode::parse) && p->jstok_cur == p->jstok_end ) {
        return FJ_EC_NO_FREE_TOKENS;
    }

    fj_token<Iterator> *startobj = p->jstok_cur++;
    __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
        startobj->type(FJ_TYPE_OBJECT);
        startobj->parent(parent);
        if ( startobj->parent() ) {
            __FLATJSON__CHECK_OVERFLOW(startobj->parent()->childs(), __FLATJSON__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
            startobj->parent()->childs(startobj->parent()->childs() + 1);
        }
    }

    while ( fj_current_char(p) != '}' ) {
        char ch = fj_current_char(p);
        if ( ch != '"' ) {
            if ( ch == ((char)-1) ) {
                return FJ_EC_INCOMPLETE;
            }

            return FJ_EC_INVALID;
        }

        if ( (M & parser_mode::parse) && p->jstok_cur == p->jstok_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }

        fj_token<Iterator> *current = p->jstok_cur++;

        std::size_t size = 0;
        e_fj_token_type toktype{};
        ec = fj_parse_value<M>(
             p
            ,current->key_iterator()
            ,&size
            ,&toktype
            ,startobj
        );
        if ( ec ) {
            return ec;
        }
        __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
            current->type(toktype);
            __FLATJSON__CHECK_OVERFLOW(size, __FLATJSON__KLEN_TYPE, FJ_EC_KLEN_OVERFLOW);
            current->klen(size);
        }

        ec = fj_check_and_skip(p, ':');
        if ( ec ) {
            return ec;
        }

        ch = fj_current_char(p);
        if ( ch == '[' || ch == '{' ) {
            p->jstok_cur -= 1;
            static Iterator unused_str{};
            std::size_t unused_size{};
            ec = fj_parse_value<M>(
                 p
                ,&unused_str
                ,&unused_size
                ,&toktype
                ,startobj
            );
        } else {
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                current->type(toktype);
                current->parent(startobj);
                __FLATJSON__CHECK_OVERFLOW(startobj->childs(), __FLATJSON__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
                startobj->childs(startobj->childs() + 1);
            }

            ec = fj_parse_value<M>(
                 p
                ,current->value_iterator()
                ,&size
                ,&toktype
                ,startobj
            );
            __FLATJSON__CHECK_OVERFLOW(size, __FLATJSON__VLEN_TYPE, FJ_EC_VLEN_OVERFLOW);
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                current->type(toktype);
                current->vlen(size);
            }
        }

        if ( ec ) {
            return ec;
        }

        if ( fj_current_char(p) == ',' ) {
            p->js_cur++;
            if ( *(p->js_cur) == '}' ) {
                return FJ_EC_INVALID;
            }
        }

        __FLATJSON__CONSTEXPR_IF ( M & parser_mode::sorted_objects ) {
            if ( startobj->childs() > 1 ) {
                fj_token<Iterator> *it = lower_bound(
                     startobj + 1
                    ,startobj + startobj->childs()
                    ,current
                    ,[](const fj_token<Iterator> *l, const fj_token<Iterator> *r) {
                         const auto lkey = l->key();
                         const auto rkey = r->key();

                         if ( lkey.size() < rkey.size() ) {
                             return true;
                         }

                         bool res = lkey < rkey;
                         return res;
                     }
                );
#if 0
                std::printf("'current'(\"%.*s\")=%p, 'place'(\"%.*s\")=%p, 'current' %s 'place', so 'current' should be %s\n"
                    ,(int)current->klen(), current->key().data()
                    ,current
                    ,(int)it->klen(), it->key().data()
                    ,it
                    ,(current->key() < it->key() ? "<" : current->key() > it->key() ? ">" : "=")
                    ,(current->key() < it->key() ? "before 'place'" : current->key() > it->key() ? "after 'place'" : "in his position")
                );
#endif // 1
                if ( it != current ) {
                    if ( current->is_simple_type() ) {
                        if ( it->is_simple_type() ) {
                            swap_simple_with_simple(it, current);
                        } else {
                            swap_simple_with_complex(it, it->end(), current);
                        }
                    } else {
                        if ( it->is_simple_type() ) {
                            swap_complex_with_simple(it, current, current->end());
                        } else {
                            swap_complex_with_complex(it, it->end(), current, current->end());
                        }
                    }
                }
            }
        }
    }

    ec = fj_check_and_skip(p, '}');
    if ( ec ) {
        return ec;
    }

    __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
        if ( p->jstok_cur == p->jstok_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }

        fj_token<Iterator> *endobj = p->jstok_cur++;
        endobj->type(FJ_TYPE_OBJECT_END);
        endobj->parent(startobj);
        __FLATJSON__CHECK_OVERFLOW(endobj->parent()->childs(), __FLATJSON__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
        endobj->parent()->childs(endobj->parent()->childs() + 1);
        startobj->end(endobj);
    } else {
        ++p->jstok_cur;
    }

    return FJ_EC_OK;
}

template<std::size_t M, typename Iterator>
int fj_parse_value(fj_parser<Iterator> *p, Iterator *ptr, std::size_t *size, e_fj_token_type *toktype, fj_token<Iterator> *parent) {
    auto ch = fj_current_char(p);
    switch ( ch ) {
        case '{': {
            int ec = fj_parse_object<M>(p, parent);
            if ( ec ) {
                return ec;
            }
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                *toktype = FJ_TYPE_OBJECT;
            }
            break;
        }
        case '[': {
            int ec = fj_parse_array<M>(p, parent);
            if ( ec ) {
                return ec;
            }
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                *toktype = FJ_TYPE_ARRAY;
            }
            break;
        }
        case 'n': {
            int ec = fj_expect<M>(p, "null", ptr, size);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                *toktype = FJ_TYPE_NULL;
            }
            break;
        }
        case 't': {
            int ec = fj_expect<M>(p, "true", ptr, size);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                *toktype = FJ_TYPE_BOOL;
            }
            break;
        }
        case 'f': {
            int ec = fj_expect<M>(p, "false", ptr, size);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                *toktype = FJ_TYPE_BOOL;
            }
            break;
        }
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            int ec = fj_parse_number<M>(p, ptr, size);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                *toktype = FJ_TYPE_NUMBER;
            }
            break;
        }
        case '"': {
            int ec = fj_parse_string<M>(p, ptr, size);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            __FLATJSON__CONSTEXPR_IF ( M & parser_mode::parse ) {
                *toktype = FJ_TYPE_STRING;
            }
            break;
        }
        default:
            if ( ch == ((char)-1) ) {
                return FJ_EC_INCOMPLETE;
            } else {
                return FJ_EC_INVALID;
            }
    }

    return FJ_EC_OK;
}

struct parse_result {
    e_fj_error_code ec;
    std::size_t toknum;
};

template<typename Iterator>
void fj_init(fj_parser<Iterator> *parser, Iterator beg, Iterator end, fj_token<Iterator> *tokbeg, fj_token<Iterator> *tokend) {
    // root token
    tokbeg->type(FJ_TYPE_INVALID);
    tokbeg->key(Iterator{});
    tokbeg->klen(0);
    tokbeg->value(Iterator{});
    tokbeg->vlen(0);
    tokbeg->parent(nullptr);
    tokbeg->childs(0);

    *parser = {beg, end, tokbeg, tokbeg, tokend};
}

template<typename Iterator>
fj_parser<Iterator> fj_make_parser(fj_token<Iterator> *tokbeg, fj_token<Iterator> *tokend, Iterator beg, Iterator end) {
    fj_parser<Iterator> parser{};
    fj_init(std::addressof(parser), beg, end, tokbeg, tokend);

    return parser;
}

template<typename Iterator>
parse_result fj_parse(fj_parser<Iterator> *parser, bool sorted = false) {
    parse_result res{};

    std::size_t vlen = 0;
    e_fj_token_type toktype{};
    if ( sorted ) {
        res.ec = static_cast<e_fj_error_code>(fj_parse_value<parser_mode::parse | parser_mode::sorted_objects, Iterator>(
             parser
            ,parser->jstok_beg->value_iterator()
            ,&vlen
            ,&toktype
            ,nullptr
        ));
    } else {
        res.ec = static_cast<e_fj_error_code>(fj_parse_value<parser_mode::parse, Iterator>(
             parser
            ,parser->jstok_beg->value_iterator()
            ,&vlen
            ,&toktype
            ,nullptr
        ));
    }

    parser->jstok_beg->type(toktype);
    assert(vlen <= std::numeric_limits<__FLATJSON__VLEN_TYPE>::max());
    parser->jstok_beg->vlen(vlen);

    if ( res.ec ) {
        res.toknum = 0;
    }

    res.toknum = parser->jstok_cur - parser->jstok_beg;

    return res;
}

template<typename Iterator>
parse_result fj_num_tokens(Iterator beg, Iterator end) {
    parse_result res{};
    fj_token<Iterator> tokens[1]{};
    fj_parser<Iterator> parser = fj_make_parser(std::begin(tokens), std::end(tokens), beg, end);
    std::size_t vlen = 0;
    e_fj_token_type toktype{};
    res.ec = static_cast<e_fj_error_code>(fj_parse_value<parser_mode::count_tokens, Iterator>(
         &parser
        ,tokens[0].value_iterator()
        ,&vlen
        ,&toktype
        ,nullptr
    ));

    if ( res.ec == FJ_EC_OK && parser.js_cur+1 != parser.js_end ) {
        fj_skip_ws(&parser);
        if ( parser.js_cur != parser.js_end ) {
            res.ec = FJ_EC_INVALID;
        }
    }

    assert(vlen <= std::numeric_limits<__FLATJSON__VLEN_TYPE>::max());
    parser.jstok_beg->vlen(vlen);

    res.toknum = parser.jstok_cur - parser.jstok_beg;

    return res;
}

// for debugging purposes
template<typename Iterator>
void fj_dump_tokens(std::FILE *stream, fj_token<Iterator> *toks, std::size_t num) {
    for ( auto *it = toks; it != toks+num; ++it ) {
        const auto key = it->key();
        const auto val = it->value();
        std::fprintf(stream, "%2d: type=%12s, parent=%p, addr=%p, end=%p, childs=%d, key=\"%.*s\", val=\"%.*s\"\n"
            ,(int)(it - toks)
            ,fj_token_type_name(it->type())
            ,it->parent()
            ,it
            ,it->end()
            ,(int)it->childs()
            ,(int)key.size(), key.data()
            ,(int)val.size(), val.data()
        );
        std::fflush(stream);
    }
}

template<typename Iterator>
using fj_gather_cb_t = void(*)(void *userdata, const Iterator ptr, std::size_t len);

template<typename Iterator, bool CalcLength = false>
std::size_t fj_get_tokens(const fj_token<Iterator> *toks, std::size_t num, std::size_t indent, void *userdata, fj_gather_cb_t<Iterator> cb) {
    static const char indent_str[] = "                                                                                ";
    std::size_t indent_scope = 0;
    std::size_t length = 0;
    for ( auto *it = toks; it != toks+num; ++it ) {
        if ( it != toks ) {
            e_fj_token_type ctype = it->type();
            e_fj_token_type ptype = (it-1)->type();
            if ( (ctype != FJ_TYPE_ARRAY_END && ctype != FJ_TYPE_OBJECT_END ) &&
                 (ptype != FJ_TYPE_OBJECT && ptype != FJ_TYPE_ARRAY) )
            {
                if ( !CalcLength ) {
                    cb(userdata, ",", 1);
                    if ( indent ) {
                        cb(userdata, "\n", 1);
                    }
                }
                length += 1;
                if ( indent ) {
                    length += 1;
                }
            }
        }

        switch ( it->type() ) {
            case FJ_TYPE_OBJECT: {
                const auto key = it->key();
                if ( !key.empty() ) {
                    if ( !CalcLength ) {
                        if ( indent ) {
                            cb(userdata, indent_str, indent_scope);
                        }
                        cb(userdata, "\"", 1);
                        cb(userdata, key.data(), key.size());
                        cb(userdata, "\":", 2);
                    }
                    length += 1+2;
                    length += key.size();
                    if ( indent ) {
                        length += indent_scope;
                    }
                }
                if ( !CalcLength ) {
                    cb(userdata, "{", 1);
                    if ( indent ) {
                        cb(userdata, "\n", 1);
                    }
                }
                length += 1;
                if ( indent ) {
                    length += 1;
                    indent_scope += indent;
                }
                break;
            }
            case FJ_TYPE_OBJECT_END: {
                if ( !CalcLength ) {
                    if ( indent ) {
                        cb(userdata, "\n", 1);
                        cb(userdata, indent_str, indent_scope-indent);
                    }
                    cb(userdata, "}", 1);
                }
                length += 1;
                if ( indent ) {
                    length += 1;
                    indent_scope -= indent;
                    length += indent_scope;
                }
                break;
            }
            case FJ_TYPE_ARRAY: {
                const auto key = it->key();
                if ( !key.empty() ) {
                    if ( !CalcLength ) {
                        if ( indent ) {
                            cb(userdata, indent_str, indent_scope);
                        }
                        cb(userdata, "\"", 1);
                        cb(userdata, key.data(), key.size());
                        cb(userdata, "\":", 2);
                    }
                    length += 1+2;
                    length += key.size();
                    if ( indent ) {
                        length += indent_scope;
                    }
                }
                if ( !CalcLength ) {
                    cb(userdata, "[", 1);
                    if ( indent ) {
                        cb(userdata, "\n", 1);
                    }
                }
                length += 1;
                if ( indent ) {
                    length += 1;
                    indent_scope += indent;
                }
                break;
            }
            case FJ_TYPE_ARRAY_END: {
                if ( !CalcLength ) {
                    if ( indent ) {
                        cb(userdata, "\n", 1);
                        cb(userdata, indent_str, indent_scope-indent);
                    }
                    cb(userdata, "]", 1);
                }
                length += 1;
                if ( indent ) {
                    length += 1;
                    indent_scope -= indent;
                    length += indent_scope;
                }
                break;
            }
            case FJ_TYPE_NULL:
            case FJ_TYPE_BOOL:
            case FJ_TYPE_NUMBER:
            case FJ_TYPE_STRING: {
                if ( it->parent()->type() != FJ_TYPE_ARRAY ) {
                    const auto key = it->key();
                    if ( !CalcLength ) {
                        if ( indent ) {
                            cb(userdata, indent_str, indent_scope);
                        }
                        cb(userdata, "\"", 1);
                        cb(userdata, key.data(), key.size());
                        cb(userdata, "\":", 2);
                    }
                    length += 1+2;
                    length += key.size();
                    if ( indent ) {
                        length += indent_scope;
                    }
                } else if ( it->parent()->type() == FJ_TYPE_ARRAY ) {
                    if ( !CalcLength ) {
                        if ( indent ) {
                            cb(userdata, indent_str, indent_scope);
                        }
                    }
                    if ( indent ) {
                        length += indent_scope;
                    }
                }
                switch ( it->type() ) {
                    case FJ_TYPE_NULL:
                    case FJ_TYPE_BOOL:
                    case FJ_TYPE_NUMBER: {
                        const auto val = it->value();
                        if ( !CalcLength ) {
                            cb(userdata, val.data(), val.size());
                        }
                        length += val.size();
                        break;
                    }
                    case FJ_TYPE_STRING: {
                        const auto val = it->value();
                        if ( !CalcLength ) {
                            cb(userdata, "\"", 1);
                            cb(userdata, val.data(), val.size());
                            cb(userdata, "\"", 1);
                        }
                        length += 2;
                        length += val.size();
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

/*************************************************************************************************/

template<typename Iterator>
void tokens_to_stream_cb_0(void *userdata, const Iterator ptr, std::size_t len) {
    auto *stream = static_cast<std::FILE*>(userdata);
    std::fwrite(ptr, len, 1, stream);
}

template<typename Iterator>
std::size_t fj_tokens_to_stream(std::FILE *stream, const fj_token<Iterator> *toks, std::size_t num, std::size_t indent = 0) {
    return fj_get_tokens(toks, num, indent, stream, tokens_to_stream_cb_0);
}

/*************************************************************************************************/

template<typename Iterator>
void tokens_to_stream_cb_1(void *userdata, const Iterator ptr, std::size_t len) {
    auto *stream = static_cast<std::ostream *>(userdata);
    stream->write(ptr, static_cast<std::streamsize>(len));
}

template<typename Iterator>
std::size_t fj_tokens_to_stream(std::ostream &stream, const fj_token<Iterator> *toks, std::size_t num, std::size_t indent = 0) {
    return fj_get_tokens(toks, num, indent, &stream, tokens_to_stream_cb_1);
}

/*************************************************************************************************/

struct tokens_to_buf_userdata {
    char *ptr;
    const char *end;
};

template<typename Iterator>
void tokens_to_buf_cb(void *userdata, const Iterator ptr, std::size_t len) {
    auto *p = static_cast<tokens_to_buf_userdata *>(userdata);
    assert(p->ptr + len <= p->end);
    std::memcpy(p->ptr, ptr, len);
    p->ptr += len;
}

template<typename Iterator>
std::size_t fj_tokens_to_buf(const fj_token<Iterator> *toks, std::size_t num, char *buf, std::size_t size, std::size_t indent = 0) {
    tokens_to_buf_userdata userdata{buf, buf+size};
    return fj_get_tokens(toks, num, indent, &userdata, tokens_to_buf_cb);
}

/*************************************************************************************************/

template<typename Iterator>
std::size_t fj_str_length(const fj_token<Iterator> *toks, std::size_t num, std::size_t indent = 0) {
    return fj_get_tokens<Iterator, true>(toks, num, indent, nullptr, nullptr);
}

/*************************************************************************************************/

template<typename Iterator>
std::size_t fj_get_keys(const fj_token<Iterator> *toks, std::size_t num, void *userdata, fj_gather_cb_t<Iterator> cb) {
    std::size_t cnt{};

    if ( !toks->is_object() ) {
        return 0;
    }

    const fj_token<Iterator> *parent = toks;
    for ( const auto *it = toks; it != toks+num; ++it ) {
        if ( it->parent() != parent || it->type() == FJ_TYPE_OBJECT_END ) {
            continue;
        }

        const auto key = it->key();
        cb(userdata, key.data(), key.size());

        ++cnt;
    }

    return cnt;
}

} // ns details

/*************************************************************************************************/

struct fjson {
    using InputIterator = const char *;
    using element_type = fj_token<InputIterator>;
    using storage_type = std::vector<element_type>;
    using storage_ptr = std::shared_ptr<storage_type>;

private:
    template<typename T>
    struct tokens_iterator_impl {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = value_type *;
        using reference = value_type &;

        explicit tokens_iterator_impl(pointer c)
            :m_cur{c}
        {}

        pointer operator->() const { return m_cur; }
        tokens_iterator_impl& operator++() { ++m_cur; return *this; }
        tokens_iterator_impl operator++(int) { tokens_iterator_impl tmp{m_cur}; ++(*this); return tmp; }
        reference operator* () { return *m_cur; }

        friend difference_type operator- (const tokens_iterator_impl &l, const tokens_iterator_impl &r)
        { return l.m_cur - r.m_cur; }
        friend bool operator== (const tokens_iterator_impl &l, const tokens_iterator_impl &r)
        { return l.m_cur == r.m_cur; }
        friend bool operator!= (const tokens_iterator_impl &l, const tokens_iterator_impl &r)
        { return !operator==(l, r); }

    private:
        pointer m_cur;
    };

public:
    using iterator = tokens_iterator_impl<element_type>;
    using const_iterator = tokens_iterator_impl<const element_type>;

    iterator begin()              { return iterator{m_beg}; }
    iterator end()                { return iterator{m_end}; }
    const_iterator begin()  const { return const_iterator{m_beg}; }
    const_iterator end()    const { return const_iterator{m_end}; }
    const_iterator cbegin() const { return const_iterator{m_beg}; }
    const_iterator cend()   const { return const_iterator{m_end}; }

    fjson(const fjson &) = default;
    fjson(fjson &&) = default;

    fjson& operator= (const fjson &) = default;
    fjson& operator= (fjson &&) = default;

    explicit fjson(std::size_t reserved = 0)
        :m_storage{std::make_shared<storage_type>(reserved)}
        ,m_src_beg{nullptr}
        ,m_src_end{nullptr}
        ,m_beg{nullptr}
        ,m_end{nullptr}
        ,m_err{}
    {}
    template<
         std::size_t L
        ,typename CharT = typename std::iterator_traits<InputIterator>::value_type
    >
    explicit fjson(const CharT (&str)[L], std::size_t reserved = 0)
        :m_storage{L-1 ? std::make_shared<storage_type>(reserved) : storage_ptr{}}
        ,m_src_beg{str}
        ,m_src_end{str+L-1}
        ,m_beg{nullptr}
        ,m_end{nullptr}
        ,m_err{}
    {
        load(str, L-1);
    }
    fjson(InputIterator ptr, std::size_t size, std::size_t reserved = 0)
        :m_storage{size ? std::make_shared<storage_type>(reserved) : storage_ptr{}}
        ,m_src_beg{ptr}
        ,m_src_end{ptr+size}
        ,m_beg{nullptr}
        ,m_end{nullptr}
        ,m_err{}
    {
        load(ptr, size);
    }
    fjson(InputIterator beg, InputIterator end, std::size_t reserved = 0)
        :m_storage{beg != end ? std::make_shared<storage_type>(reserved) : storage_ptr{}}
        ,m_src_beg{beg}
        ,m_src_end{end}
        ,m_beg{nullptr}
        ,m_end{nullptr}
        ,m_err{}
    {
        load(beg, end);
    }

    virtual ~fjson() = default;

private:
    fjson(storage_ptr storage, element_type *beg, element_type *end)
        :m_storage{std::move(storage)}
        ,m_src_beg{nullptr}
        ,m_src_end{nullptr}
        ,m_beg{beg}
        ,m_end{end}
        ,m_err{}
    {}

public:
    bool is_valid() const { return m_beg && m_end && !m_storage->empty() && m_err == FJ_EC_OK; }
    e_fj_error_code error() const { return m_err; }
    const char* error_string() const { return fj_error_string(m_err); }

    std::size_t size() const {
        return (!details::fj_is_simple_type(m_beg->type()))
           ? m_beg->childs() - 1
           : static_cast<std::size_t>(m_beg->type() != FJ_TYPE_INVALID)
        ;
    }
    std::size_t tokens() const {
        return (!details::fj_is_simple_type(m_beg->type()))
            ? (m_end-m_beg) + (m_beg->parent() && m_beg->parent()->type() == FJ_TYPE_ARRAY ? 1 : 0)
            : static_cast<std::size_t>(m_beg->type() != FJ_TYPE_INVALID)
        ;
    }
    bool is_empty() const { return size() == 0; }
    void clear() {
        m_storage->clear();
        m_beg = m_end = nullptr;
    }

    e_fj_token_type type() const { return m_beg->type(); }
    const char* type_name() const { return m_beg->type_name(); }

    bool is_array() const { return m_beg->is_array(); }
    bool is_object() const { return m_beg->is_object(); }
    bool is_null() const { return m_beg->is_null(); }
    bool is_bool() const { return m_beg->is_bool(); }
    bool is_number() const { return m_beg->is_number(); }
    bool is_string() const { return m_beg->is_string(); }
    bool is_simple_type() const { return m_beg->is_simple_type(); }

    static_string to_sstring() const { return m_beg->to_sstring(); }
    std::string to_string() const { return m_beg->to_string(); }
    template<typename T>
    T to() const { return m_beg->template to<T>(); }
    bool to_bool() const { return m_beg->to_bool(); }
    std::uint32_t to_uint() const { return m_beg->to_uint(); }
    std::int32_t to_int() const { return m_beg->to_int(); }
    std::uint64_t to_uint64() const { return m_beg->to_uint64(); }
    std::int64_t to_int64() const { return m_beg->to_int64(); }
    double to_double() const { return m_beg->to_double(); }
    float to_float() const { return m_beg->to_float(); }

    template<std::size_t KL>
    bool contains(const char (&key)[KL]) const { return contains(key, KL-1); }
    template<
        typename ConstCharPtr
        ,typename = typename std::enable_if<
            std::is_same<ConstCharPtr, const char*>::value
        >::type
    >
    bool contains(ConstCharPtr key) const { return contains(key, std::strlen(key)); }
    bool contains(const char *key, std::size_t len) const {
        auto res = find(key, len);
        return res.first != nullptr;
    }

    // for objects
    template<std::size_t KL>
    fjson at(const char (&key)[KL]) const { return at(key, KL-1); }
    template<
        typename ConstCharPtr
        ,typename = typename std::enable_if<
            std::is_same<ConstCharPtr, const char*>::value
        >::type
    >
    fjson at(ConstCharPtr key) const { return at(key, std::strlen(key)); }
    fjson at(const char *key, std::size_t len) const {
        auto res = find(key, len);
        if ( res.first ) {
            return {m_storage, res.first, res.second};
        }

        throw std::runtime_error(__FLATJSON__MAKE_ERROR_MESSAGE("key not found"));
    }
    // for arrays
    fjson at(std::size_t idx) const {
        auto res = find(idx);
        if ( res.first ) {
            return {m_storage, res.first, res.second};
        }

        throw std::out_of_range(__FLATJSON__MAKE_ERROR_MESSAGE("out of range"));
    }

    // for arrays
    fjson operator[](std::size_t idx) const { return at(idx); }

    // for objects
    template<std::size_t KL>
    fjson operator[](const char (&key)[KL]) const { return at(key, KL-1); }
    template<
         typename ConstCharPtr
        ,typename = typename std::enable_if<
            std::is_same<ConstCharPtr, const char*>::value
        >::type
    >
    fjson operator[](ConstCharPtr key) const { return at(key, std::strlen(key)); }

    template<std::size_t N, typename CharT = typename std::iterator_traits<InputIterator>::value_type>
    bool load(const char (&str)[N]) { return load(str, str+N-1); }
    bool load(InputIterator beg, std::size_t size) { return load(beg, beg+size); }
    bool load(InputIterator beg, InputIterator end) {
        if ( beg == end ) {
            return false;
        }

        if ( m_storage->empty() ) {
            auto res = details::fj_num_tokens(beg, end);
            if ( res.ec ) {
                m_err = res.ec;

                return false;
            } else {
                m_storage->resize(res.toknum);
            }
        }

        details::fj_parser<InputIterator> parser{};
        details::fj_init(
             &parser
            ,beg
            ,end
            ,std::addressof(*m_storage->begin())
            ,std::addressof(*m_storage->end())
        );
        details::parse_result res = details::fj_parse(&parser);
        if ( res.ec ) {
            m_err = res.ec;
            return false;
        }

        m_storage->resize(res.toknum);
        m_beg = m_storage->data();
        m_end = m_beg + m_storage->size();

        return true;
    }

    std::string dump(std::size_t indent = 0) const {
        std::size_t strlen = details::fj_str_length(m_beg, m_storage->size(), indent);
        std::string res(strlen, 0);
        assert(strlen == details::fj_tokens_to_buf(
                 m_beg
                ,m_storage->size()
                ,&res[0]
                ,res.size()
                ,indent
            )
        );

        return res;
    }
    std::ostream& dump(std::ostream &os, std::size_t indent = 0) const {
        details::fj_tokens_to_stream(os, m_beg, m_end - m_beg, indent);

        return os;
    }
    friend std::ostream& operator<< (std::ostream &os, const fjson &fj) {
        details::fj_tokens_to_stream(os, fj.m_beg, fj.m_end - fj.m_beg);
        return os;
    }

    std::pair<const fj_token<InputIterator>*, std::size_t>
    data() const { return {m_beg, m_end-m_beg}; }

    std::vector<static_string>
    get_keys() const {
        std::vector<static_string> res{};

        const auto d = data();
        details::fj_get_keys(d.first, d.second, &res, &get_keys_cb);

        return res;
    }

    std::pair<InputIterator, InputIterator>
    get_source_data() const { return {m_src_beg, m_src_end}; }

private:
    static void get_keys_cb(void *userdata, const char *ptr, std::size_t len) {
        auto *vec = static_cast<std::vector<static_string> *>(userdata);
        vec->push_back(static_string{ptr, len});
    }

private:
    std::pair<element_type *, element_type *>
    find(const char *key, std::size_t len) const {
        if ( type() != FJ_TYPE_OBJECT ) {
            throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("not OBJECT type"));
        }

        element_type *parent = m_beg;
        element_type *beg = parent+1;
        element_type *end = parent->end();
        while ( beg != end ) {
            if ( beg->type() == FJ_TYPE_OBJECT_END ) {
                return {nullptr, nullptr};
            }
            if ( beg->klen() == len && std::strncmp(beg->key().data(), key, len) == 0 ) {
                break;
            }

            beg = details::fj_is_simple_type(beg->type()) ? beg + 1 : beg->end() + 1;
        }

        const auto type = beg->type();
        if ( details::fj_is_simple_type(type) ) {
            return {beg, beg+1};
        } else if ( type == FJ_TYPE_OBJECT || type == FJ_TYPE_ARRAY ) {
            return {beg, beg->end()};
        } else if ( beg == end && type == FJ_TYPE_OBJECT_END ) {
            return {nullptr, nullptr};
        }

        throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("unreachable!"));
    }
    std::pair<element_type *, element_type *>
    find(std::size_t idx) const {
        if ( type() != FJ_TYPE_ARRAY ) {
            throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("not ARRAY type"));
        }

        if ( idx >= static_cast<__FLATJSON__CHILDS_TYPE>(m_beg->childs() - 1) ) { // one for END token
            return {nullptr, nullptr};
        }

        element_type *parent = m_beg;
        element_type *beg = parent+1;
        element_type *end = parent->end();
        for ( ; beg != end && idx; --idx ) {
            beg = details::fj_is_simple_type(beg->type()) ? beg + 1 : beg->end() + 1;
        }

        const auto type = beg->type();
        if ( details::fj_is_simple_type(type) ) {
            return {beg, beg+1};
        } else if ( type == FJ_TYPE_OBJECT || type == FJ_TYPE_ARRAY ) {
            return {beg, beg->end()};
        } else if ( beg == end && type == FJ_TYPE_ARRAY_END ) {
            return {nullptr, nullptr};
        }

        throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("unreachable!"));
    }

private:
    storage_ptr m_storage;
    InputIterator m_src_beg;
    InputIterator m_src_end;
    element_type *m_beg;
    element_type *m_end;
    e_fj_error_code m_err;
};

/*************************************************************************************************/

template<typename Iterator>
fjson parse(Iterator beg, Iterator end) {
    fjson json{beg, end};

    return json;
}

template<typename Iterator>
fjson parse(Iterator beg) {
    auto end = beg + std::strlen(beg);

    return parse(beg, end);
}

} // ns flatjson

/*************************************************************************************************/

// undef internally used macro-vars
#undef __FLATJSON__FALLTHROUGH
#undef __FLATJSON__STRINGIZE_I
#undef __FLATJSON__STRINGIZE
#undef __FLATJSON__MAKE_ERROR_MESSAGE
#undef __FLATJSON__CHECK_OVERFLOW
#undef __FLATJSON__KLEN_TYPE
#undef __FLATJSON__VLEN_TYPE
#undef __FLATJSON__CHILDS_TYPE

/*************************************************************************************************/

#endif // __FLATJSON__FLATJSON_HPP
