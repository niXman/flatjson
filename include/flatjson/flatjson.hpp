
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

#include <ostream>
#include <vector>
#include <string>
#include <memory>
#include <limits>

#include <cassert>
#include <cstdint>
#include <cstring>

#ifdef __FJ__SUPPORTS_STDIO
#   include <cstdio>
#   include <unistd.h>
#endif // __FJ__SUPPORTS_STDIO

/*************************************************************************************************/

#if defined(__clang__)
#   define __FJ__FALLTHROUGH [[clang::fallthrough]]
#elif defined(__GNUC__)
#   define __FJ__FALLTHROUGH __attribute__ ((fallthrough))
#elif defined(_MSC_VER)
#   define __FJ__FALLTHROUGH
#else
#   error "Unknown compiler"
#endif //

/*************************************************************************************************/

#define __FJ__STRINGIZE_I(x) #x
#define __FJ__STRINGIZE(x) __FJ__STRINGIZE_I(x)

#define __FJ__MAKE_ERROR_MESSAGE(msg) \
    __FILE__ "(" __FJ__STRINGIZE(__LINE__) "): " msg

#ifdef __FJ__DONT_CHECK_OVERFLOW
#   define __FJ__CHECK_OVERFLOW(expr, type, err)
#else
#   define __FJ__CHECK_OVERFLOW(expr, type, err) \
        if ( (expr) >= (std::numeric_limits<type>::max)() ) return err
#endif //__FLATJSON__SHOULD_CHECK_OVERFLOW

#ifndef __FJ__KLEN_TYPE
#   define __FJ__KLEN_TYPE std::uint8_t
#endif // __FJ__KLEN_TYPE
#ifndef __FJ__VLEN_TYPE
#   define __FJ__VLEN_TYPE std::uint32_t
#endif // __FJ__VLEN_TYPE
#ifndef __FJ__CHILDS_TYPE
#   define __FJ__CHILDS_TYPE std::uint16_t
#endif // __FJ__CHILDS_TYPE

/*************************************************************************************************/

namespace flatjson {

template<typename T>
struct enable_if_const_char_ptr;

template<>
struct enable_if_const_char_ptr<const char *> {
    using type = void;
};

} // ns flatjson

/*************************************************************************************************/

#if __cplusplus >= 201703L
#   include <string_view>
#   define __FJ__CONSTEXPR_IF(...) if constexpr (__VA_ARGS__)
namespace flatjson {
using string_view = std::string_view;
} // ns flatjson
#else
#   define __FJ__CONSTEXPR_IF(...) if (__VA_ARGS__)

namespace flatjson {

/*************************************************************************************************/

struct string_view {
    string_view() = default;
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

    int compare(std::size_t, std::size_t len, const char *r) const { return std::strncmp(m_ptr, r, len); }
    template<std::size_t N>
    int compare(const char (&r)[N]) const { return compare(0, N-1, r); }
    int compare(const string_view &r) const { return compare(0, r.size(), r.data()); }

    template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
    friend bool operator==(const string_view &l, T r) { return l.compare(r) == 0; }
    template<std::size_t N>
    friend bool operator==(const string_view &l, const char (&r)[N]) { return l.compare(0, N-1, r) == 0; }
    friend bool operator==(const string_view &l, const string_view &r) { return l.compare(r) == 0; }

    template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
    friend bool operator!=(const string_view &l, T r) { return !l.compare(r); }
    template<std::size_t N>
    friend bool operator!=(const string_view &l, const char (&r)[N]) { return !l.compare(0, N-1, r); }
    friend bool operator!=(const string_view &l, const string_view &r) { return !l.compare(r); }

    friend std::ostream& operator<< (std::ostream &os, const string_view &s) {
        os.write(s.m_ptr, s.m_len);

        return os;
    }

private:
    const char *m_ptr;
    std::size_t m_len;
};

/*************************************************************************************************/

} // ns flatjson

#endif // __cplusplus >= 201703L

namespace flatjson {

/*************************************************************************************************/

enum fj_token_type: std::uint8_t {
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

inline const char *fj_type_name(fj_token_type t) {
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

enum fj_error_code {
     FJ_EC_OK = 0
    ,FJ_EC_INVALID = -1
    ,FJ_EC_INCOMPLETE = -2
    ,FJ_EC_NO_FREE_TOKENS = -3
    ,FJ_EC_KLEN_OVERFLOW = -4
    ,FJ_EC_VLEN_OVERFLOW = -5
    ,FJ_EC_CHILDS_OVERFLOW = -6
};

inline const char* fj_error_string(fj_error_code e) {
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

inline bool fj_is_simple_type(char v) {
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

inline bool fj_is_digit(char ch) {
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

inline bool fj_is_hex_digit(char ch) {
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

inline bool fj_is_hex_digit4(char ch0, char ch1, char ch2, char ch3) {
    return fj_is_hex_digit(ch0) && fj_is_hex_digit(ch1) && fj_is_hex_digit(ch2) && fj_is_hex_digit(ch3);
}

inline std::size_t fj_utf8_char_len(char ch) {
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

#undef _FJ_CASE_1
#undef _FJ_CASE_3
#undef _FJ_CASE_5
#undef _FJ_CASE_10
#undef _FJ_CASE_20
#undef _FJ_CASE_40

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
            assert("unreachable!" == nullptr);
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
        if ( res > std::numeric_limits<To>::max() ) {
            assert("overflow detected!");
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

#ifndef __FJ__DONT_PACK_TOKENS
#pragma pack(push, 1)
#endif // __FJ__DONT_PACK_TOKENS

struct fj_token {
    fj_token_type m_type;
    const char *m_key;
    __FJ__KLEN_TYPE m_klen;
    const char *m_val;
    __FJ__VLEN_TYPE m_vlen;
    fj_token *m_parent;
    __FJ__CHILDS_TYPE m_childs;
    fj_token *m_end; // pointing to the last token for arrays and objects
};

#ifndef __FJ__DONT_PACK_TOKENS
#pragma pack(pop)
#endif // __FJ__DONT_PACK_TOKENS

/*************************************************************************************************/

using fj_alloc_fnptr = void*(*)(std::size_t);
using fj_free_fnptr = void(*)(void *);

struct fj_parser {
    const char *str_beg;
    const char *str_cur;
    const char *str_end;

    fj_token *toks_beg;
    fj_token *toks_cur;
    fj_token *toks_end;

    fj_alloc_fnptr alloc_fn;
    fj_free_fnptr  free_fn;
    fj_error_code  error;
    bool dyn_tokens;
    bool dyn_parser;

    // intrusive pointer part
    std::uint32_t ref_cnt;

    static std::size_t inc_refcnt(fj_parser *parser) { return ++parser->ref_cnt; }
    static std::size_t dec_refcnt(fj_parser *parser) { return --parser->ref_cnt; }
};

/*************************************************************************************************/

namespace details {

/*************************************************************************************************/
// for debugging purposes

inline void fj_dump_tokens_impl(
     std::FILE *stream
    ,const fj_token *beg
    ,const fj_token *cur
    ,const fj_token *end)
{
    static const char* tnames[] = { "INV", "STR", "NUM", "BOL", "NUL", "+OB", "-OB", "+AR", "-AR" };
    static const char spaces[] = "                                                         ";

    int indent = 0;
    for ( auto it = beg; it != end; ++it ) {
        if ( it->m_type == FJ_TYPE_ARRAY_END || it->m_type == FJ_TYPE_OBJECT_END ) {
            assert(indent > 0);
            indent -= 2;
        }
        std::fprintf(stream, "%2d:%c type=%.*s%3s, addr=%p, end=%p, parent=%p, childs=%d, key=\"%.*s\", val=\"%.*s\"\n"
            ,(int)(it - beg)
            ,(it == cur ? '>' : ' ')
            ,indent, spaces, tnames[it->m_type]
            ,it
            ,it->m_end
            ,it->m_parent
            ,(int)it->m_childs
            ,(int)(it->m_klen ? it->m_klen : 5), (it->m_klen ? it->m_key : "(nil)")
            ,(int)(it->m_vlen ? it->m_vlen : 5), (it->m_vlen ? it->m_val : "(nil)")
        );
        std::fflush(stream);
        if ( it->m_type == FJ_TYPE_ARRAY || it->m_type == FJ_TYPE_OBJECT ) {
            indent += 2;
        }
    }
}

// dump using parser
inline void fj_dump_tokens(std::FILE *stream, const char *caption, fj_parser *parser) {
    std::fprintf(stream, "%s:\n", caption);
    fj_dump_tokens_impl(stream, parser->toks_beg, parser->toks_beg, parser->toks_end);
}

/*************************************************************************************************/

inline void fj_skip_ws(fj_parser *parser) {
    for (
        ;parser->str_cur < parser->str_end
            && (
                *parser->str_cur == ' '
             || *parser->str_cur == '\t'
             || *parser->str_cur == '\r'
             || *parser->str_cur == '\n'
        )
        ;++parser->str_cur
    )
        ;
}

#define __FJ__CUR_CHAR(pparser) \
    ((fj_skip_ws(parser)), (parser->str_cur >= parser->str_end ? ((int)-1) : *(parser->str_cur)))

inline int fj_check_and_skip(fj_parser *parser, char expected) {
    char ch = __FJ__CUR_CHAR(parser);
    if ( ch == expected ) {
        parser->str_cur++;

        return FJ_EC_OK;
    }

    if ( ch == ((int)-1) ) {
        return FJ_EC_INCOMPLETE;
    }

    return FJ_EC_INVALID;
}

inline int fj_escape_len(const char *s, std::ptrdiff_t len) {
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

template<bool ParseMode, std::size_t ExLen>
inline int fj_expect(fj_parser *p, const char (&s)[ExLen], const char **ptr, std::size_t *size) {
    if ( p->str_cur + (ExLen-1) > p->str_end )
        return FJ_EC_INCOMPLETE;

    if ( std::strncmp(p->str_cur, s, ExLen - 1) != 0 ) {
        return FJ_EC_INVALID;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        *ptr = p->str_cur;
        *size = ExLen - 1;
    }

    p->str_cur += ExLen - 1;

    return FJ_EC_OK;
}

template<bool ParseMode>
int fj_parse_string(fj_parser *parser, const char **value, std::size_t *vlen) {
    int ec = fj_check_and_skip(parser, '"');
    if ( ec ) {
        return ec;
    }

    int ch = 0;
    auto *start = parser->str_cur;
    for ( std::size_t len = 0; parser->str_cur < parser->str_end; parser->str_cur += len ) {
        ch = static_cast<unsigned char>(*(parser->str_cur));
        len = fj_utf8_char_len((unsigned char)ch);
        if ( !(ch >= 32 && len > 0) ) {
            return FJ_EC_INVALID;
        }
        if ( static_cast<std::ptrdiff_t>(len) > (parser->str_end - parser->str_cur) ) {
            return FJ_EC_INCOMPLETE;
        }

        if ( ch == '\\' ) {
            int n = fj_escape_len(parser->str_cur + 1, parser->str_end - parser->str_cur);
            if ( n <= 0 ) {
                return n;
            }
            len += n;
        } else if ( ch == '"' ) {
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *value = start;
                *vlen = parser->str_cur - start;
            }

            ++parser->str_cur;

            break;
        }
    }

    return ch == '"' ? FJ_EC_OK : FJ_EC_INCOMPLETE;
}

template<bool ParseMode>
int fj_parse_number(fj_parser *parser, const char **value, std::size_t *vlen) {
    auto start = parser->str_cur;
    if ( __FJ__CUR_CHAR(parser) == '-' ) {
        parser->str_cur++;
    }

    if ( parser->str_cur >= parser->str_end ) {
        return FJ_EC_INCOMPLETE;
    }
    if ( parser->str_cur + 1 < parser->str_end && *(parser->str_cur) == '0' && *(parser->str_cur+1) == 'x' ) {
        parser->str_cur += 2;

        if ( parser->str_cur >= parser->str_end ) {
            return FJ_EC_INCOMPLETE;
        }
        if ( !details::fj_is_hex_digit(*(parser->str_cur)) ) {
            return FJ_EC_INVALID;
        }

        for ( ; parser->str_cur < parser->str_end && details::fj_is_hex_digit(*(parser->str_cur)); ++parser->str_cur )
            ;
    } else {
        if ( !details::fj_is_digit(*(parser->str_cur)) ) {
            return FJ_EC_INVALID;
        }
        for ( ; parser->str_cur < parser->str_end && details::fj_is_digit(*(parser->str_cur)); ++parser->str_cur )
            ;

        if ( parser->str_cur < parser->str_end && *(parser->str_cur) == '.' ) {
            parser->str_cur++;

            if ( parser->str_cur >= parser->str_end ) {
                return FJ_EC_INCOMPLETE;
            }
            if ( !details::fj_is_digit(*(parser->str_cur)) ) {
                return FJ_EC_INVALID;
            }

            for ( ; parser->str_cur < parser->str_end && details::fj_is_digit(*(parser->str_cur)); ++parser->str_cur )
                ;
        }
        if ( parser->str_cur < parser->str_end && (*(parser->str_cur) == 'e' || *(parser->str_cur) == 'E') ) {
            parser->str_cur++;

            if ( parser->str_cur >= parser->str_end ) {
                return FJ_EC_INCOMPLETE;
            }
            if ( *(parser->str_cur) == '+' || *(parser->str_cur) == '-' ) {
                parser->str_cur++;
            }
            if ( parser->str_cur >= parser->str_end ) {
                return FJ_EC_INCOMPLETE;
            }
            if ( !details::fj_is_digit(*(parser->str_cur)) ) {
                return FJ_EC_INVALID;
            }

            for ( ; parser->str_cur < parser->str_end && details::fj_is_digit(*(parser->str_cur)); ++parser->str_cur )
                ;
        }
    }

    if ( (parser->str_cur - start) > 1 && (start[0] == '0' && start[1] != '.') ) {
        return FJ_EC_INVALID;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        *value = start;
        *vlen = parser->str_cur - start;
    }

    return FJ_EC_OK;
}

template<bool ParseMode>
int fj_parse_value(fj_parser *parser, const char **value, std::size_t *vlen, fj_token_type *toktype, fj_token *parent);

template<bool ParseMode>
int fj_parse_array(fj_parser *parser, fj_token *parent) {
    int ec = fj_check_and_skip(parser, '[');
    if ( ec ) {
        return ec;
    }

    if ( ParseMode && parser->toks_cur == parser->toks_end ) {
        return FJ_EC_NO_FREE_TOKENS;
    }

    auto *startarr = parser->toks_cur++;
    __FJ__CONSTEXPR_IF( ParseMode ) {
        startarr->m_type = FJ_TYPE_ARRAY;
        startarr->m_parent = parent;
        if ( startarr->m_parent ) {
            __FJ__CHECK_OVERFLOW(startarr->m_parent->m_childs, __FJ__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
            ++startarr->m_parent->m_childs;
        }
    }

    while ( __FJ__CUR_CHAR(parser) != ']' ) {
        if ( ParseMode && parser->toks_cur == parser->toks_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }

        auto *current_token = parser->toks_cur++;
        char ch = __FJ__CUR_CHAR(parser);
        if ( ch == '{' || ch == '[' ) {
            parser->toks_cur -= 1;
        } else {
            __FJ__CONSTEXPR_IF( ParseMode ) {
                current_token->m_parent = startarr;

                __FJ__CHECK_OVERFLOW(startarr->m_childs, __FJ__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
                ++startarr->m_childs;
            }
        }

        std::size_t size = 0;
        ec = fj_parse_value<ParseMode>(
             parser
            ,std::addressof(current_token->m_val)
            ,std::addressof(size)
            ,std::addressof(current_token->m_type)
            ,startarr
        );
        if ( ec ) {
            return ec;
        }
        __FJ__CONSTEXPR_IF( ParseMode ) {
            __FJ__CHECK_OVERFLOW(size, __FJ__VLEN_TYPE, FJ_EC_VLEN_OVERFLOW);
            current_token->m_vlen = size;
        }

        if ( __FJ__CUR_CHAR(parser) == ',' ) {
            parser->str_cur++;
            if ( *(parser->str_cur) == ']' ) {
                return FJ_EC_INVALID;
            }
        }
    }

    ec = fj_check_and_skip(parser, ']');
    if ( ec ) {
        return ec;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        if ( parser->toks_cur == parser->toks_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }
        auto *endarr = parser->toks_cur++;
        endarr->m_type = FJ_TYPE_ARRAY_END;
        endarr->m_parent = startarr;
        __FJ__CHECK_OVERFLOW(endarr->m_parent->m_childs, __FJ__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
        ++endarr->m_parent->m_childs;
        startarr->m_end = endarr;
    } else {
        ++parser->toks_cur;
    }

    return 0;
}

template<bool ParseMode>
int fj_parse_object(fj_parser *parser, fj_token *parent) {
    int ec = fj_check_and_skip(parser, '{');
    if ( ec ) {
        return ec;
    }

    if ( ParseMode && parser->toks_cur == parser->toks_end ) {
        return FJ_EC_NO_FREE_TOKENS;
    }

    auto *startobj = parser->toks_cur++;
    __FJ__CONSTEXPR_IF( ParseMode ) {
        startobj->m_type = FJ_TYPE_OBJECT;
        startobj->m_parent = parent;
        if ( startobj->m_parent ) {
            __FJ__CHECK_OVERFLOW(startobj->m_parent->m_childs, __FJ__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
            ++startobj->m_parent->m_childs;
        }
    }

    while ( __FJ__CUR_CHAR(parser) != '}' ) {
        char ch = __FJ__CUR_CHAR(parser);
        if ( ch != '"' ) {
            if ( ch == ((int)-1) ) {
                return FJ_EC_INCOMPLETE;
            }

            return FJ_EC_INVALID;
        }

        if ( ParseMode && parser->toks_cur == parser->toks_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }

        auto *current_token = parser->toks_cur++;
        std::size_t size = 0;
        ec = fj_parse_value<ParseMode>(
             parser
            ,std::addressof(current_token->m_key)
            ,std::addressof(size)
            ,std::addressof(current_token->m_type)
            ,startobj
        );
        if ( ec ) {
            return ec;
        }
        __FJ__CONSTEXPR_IF( ParseMode ) {
            __FJ__CHECK_OVERFLOW(size, __FJ__KLEN_TYPE, FJ_EC_KLEN_OVERFLOW);
            current_token->m_klen = size;
        }

        ec = fj_check_and_skip(parser, ':');
        if ( ec ) {
            return ec;
        }

        ch = __FJ__CUR_CHAR(parser);
        if ( ch == '[' || ch == '{' ) {
            parser->toks_cur -= 1;
            const char *unused_str{};
            std::size_t unused_size{};
            ec = fj_parse_value<ParseMode>(
                 parser
                ,std::addressof(unused_str)
                ,std::addressof(unused_size)
                ,std::addressof(current_token->m_type)
                ,startobj
            );
        } else {
            __FJ__CONSTEXPR_IF( ParseMode ) {
                current_token->m_parent = startobj;
                __FJ__CHECK_OVERFLOW(startobj->m_childs, __FJ__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
                ++startobj->m_childs;
            }

            ec = fj_parse_value<ParseMode>(
                 parser
                ,std::addressof(current_token->m_val)
                ,std::addressof(size)
                ,std::addressof(current_token->m_type)
                ,startobj
            );
            __FJ__CONSTEXPR_IF( ParseMode ) {
                __FJ__CHECK_OVERFLOW(size, __FJ__VLEN_TYPE, FJ_EC_VLEN_OVERFLOW);
                current_token->m_vlen = size;
            }
        }

        if ( ec ) {
            return ec;
        }

        if ( __FJ__CUR_CHAR(parser) == ',' ) {
            parser->str_cur++;
            if ( *(parser->str_cur) == '}' ) {
                return FJ_EC_INVALID;
            }
        }
    }

    ec = fj_check_and_skip(parser, '}');
    if ( ec ) {
        return ec;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        if ( parser->toks_cur == parser->toks_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }
        auto *endobj = parser->toks_cur++;
        endobj->m_type = FJ_TYPE_OBJECT_END;
        endobj->m_parent = startobj;
        __FJ__CHECK_OVERFLOW(endobj->m_parent->m_childs, __FJ__CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
        ++endobj->m_parent->m_childs;
        startobj->m_end = endobj;
    } else {
        ++parser->toks_cur;
    }

    return FJ_EC_OK;
}

template<bool ParseMode>
int fj_parse_value(
     fj_parser *parser
    ,const char **value
    ,std::size_t *vlen
    ,fj_token_type *toktype
    ,fj_token *parent)
{
    char ch = __FJ__CUR_CHAR(parser);
    switch ( ch ) {
        case '{': {
            int ec = fj_parse_object<ParseMode>(parser, parent);
            if ( ec ) {
                return ec;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_OBJECT;
            }
            break;
        }
        case '[': {
            int ec = fj_parse_array<ParseMode>(parser, parent);
            if ( ec ) {
                return ec;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_ARRAY;
            }
            break;
        }
        case 'n': {
            int ec = fj_expect<ParseMode>(parser, "null", value, vlen);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( parser->toks_cur == parser->toks_beg ) {
                ++parser->toks_cur;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_NULL;
            }
            break;
        }
        case 't': {
            int ec = fj_expect<ParseMode>(parser, "true", value, vlen);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( parser->toks_cur == parser->toks_beg ) {
                ++parser->toks_cur;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_BOOL;
            }
            break;
        }
        case 'f': {
            int ec = fj_expect<ParseMode>(parser, "false", value, vlen);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( parser->toks_cur == parser->toks_beg ) {
                ++parser->toks_cur;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
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
            int ec = fj_parse_number<ParseMode>(parser, value, vlen);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( parser->toks_cur == parser->toks_beg ) {
                ++parser->toks_cur;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_NUMBER;
            }
            break;
        }
        case '"': {
            int ec = fj_parse_string<ParseMode>(parser, value, vlen);
            if ( ec ) {
                return ec;
            }
            // on root token
            if ( parser->toks_cur == parser->toks_beg ) {
                ++parser->toks_cur;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_STRING;
            }
            break;
        }
        default:
            if ( ch == ((int)-1) ) {
                return FJ_EC_INCOMPLETE;
            } else {
                return FJ_EC_INVALID;
            }
    }

    return FJ_EC_OK;
}

/*************************************************************************************************/

inline void fj_init_parser(
     fj_parser *parser
    ,fj_token *toksbeg
    ,fj_token *toksend
    ,const char *strbeg
    ,const char *strend
    ,fj_alloc_fnptr alloc_fn
    ,fj_free_fnptr free_fn
    ,bool dyn_parser
    ,bool dyn_tokens)
{
    // root token
    if ( toksbeg ) {
        toksbeg->m_type   = FJ_TYPE_INVALID;
        toksbeg->m_key    = nullptr;
        toksbeg->m_klen   = 0;
        toksbeg->m_val    = nullptr;
        toksbeg->m_vlen   = 0;
        toksbeg->m_parent = nullptr;
        toksbeg->m_childs = 0;
    }

    parser->str_beg   = strbeg;
    parser->str_cur   = strbeg;
    parser->str_end   = strend;
    parser->toks_beg  = toksbeg;
    parser->toks_cur  = toksbeg;
    parser->toks_end  = toksend;
    parser->alloc_fn  = alloc_fn;
    parser->free_fn   = free_fn;
    parser->error     = FJ_EC_INVALID;
    parser->dyn_parser= dyn_parser;
    parser->dyn_tokens= dyn_tokens;

    parser->ref_cnt   = 0;
}

} // ns details

/*************************************************************************************************/

// zero-alloc routines

inline fj_parser fj_make_parser(
     fj_token *toksbeg
    ,fj_token *toksend
    ,const char *strbeg
    ,const char *strend)
{
    fj_parser parser;
    details::fj_init_parser(
         std::addressof(parser)
        ,toksbeg
        ,toksend
        ,strbeg
        ,strend
        ,nullptr
        ,nullptr
        ,false
        ,false
    );

    return parser;
}

inline fj_parser fj_init_parser() {
    fj_parser parser;
    details::fj_init_parser(
         std::addressof(parser)
        ,nullptr
        ,nullptr
        ,nullptr
        ,nullptr
        ,nullptr
        ,nullptr
        ,false
        ,false
    );

    return parser;
}

inline fj_parser* fj_alloc_parser(
     fj_token *toksbeg
    ,fj_token *toksend
    ,const char *strbeg
    ,const char *strend
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    auto *parser = static_cast<fj_parser *>(alloc_fn(sizeof(fj_parser)));
    if ( parser ) {
        details::fj_init_parser(
             parser
            ,toksbeg
            ,toksend
            ,strbeg
            ,strend
            ,alloc_fn
            ,free_fn
            ,true
            ,false
        );
    }

    return parser;
}

/*************************************************************************************************/
// dyn-alloc routines

inline std::size_t fj_num_tokens(fj_error_code *ecptr, const char *beg, const char *end) {
    static fj_token fake;
    auto parser = fj_make_parser(std::addressof(fake), std::addressof(fake), beg, end);

    std::size_t vlen = 0;
    fj_token_type toktype{};
    fj_error_code ec = static_cast<fj_error_code>(details::fj_parse_value<false>(
         std::addressof(parser)
        ,std::addressof(parser.toks_beg->m_val)
        ,std::addressof(vlen)
        ,std::addressof(toktype)
        ,nullptr
    ));

    if ( !ec && parser.str_cur+1 != parser.str_end ) {
        details::fj_skip_ws(std::addressof(parser));
        if ( parser.str_cur != parser.str_end ) {
            if ( ecptr ) {
                *ecptr = FJ_EC_INVALID;
            }

            return 0;
        }
    }

    assert(vlen <= std::numeric_limits<__FJ__VLEN_TYPE>::max());
    parser.toks_beg->m_vlen = static_cast<__FJ__VLEN_TYPE>(vlen);

    std::size_t toknum = parser.toks_cur - parser.toks_beg;

    return toknum;
}

inline fj_parser fj_make_parser(
     const char *strbeg
    ,const char *strend
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    fj_parser parser;
    details::fj_init_parser(
         std::addressof(parser)
        ,nullptr
        ,nullptr
        ,nullptr
        ,nullptr
        ,alloc_fn
        ,free_fn
        ,false
        ,true
    );

    if ( strend - strbeg == 1 && *strbeg == '\0' ) {
        return parser;
    }

    fj_error_code ec{};
    auto toknum = fj_num_tokens(std::addressof(ec), strbeg, strend);
    if ( ec ) {
        parser.error = ec;

        return parser;
    }

    auto in_bytes = sizeof(fj_token) * toknum;
    auto *toksbeg = static_cast<fj_token *>(alloc_fn(in_bytes));
    auto *toksend = toksbeg + toknum;

    details::fj_init_parser(
         std::addressof(parser)
        ,toksbeg
        ,toksend
        ,strbeg
        ,strend
        ,alloc_fn
        ,free_fn
        ,false
        ,true
    );

    return parser;
}

inline fj_parser* fj_alloc_parser(
     const char *strbeg
    ,const char *strend
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    auto *parser = static_cast<fj_parser *>(alloc_fn(sizeof(fj_parser)));
    if ( parser ) {
        details::fj_init_parser(
             parser
            ,nullptr
            ,nullptr
            ,nullptr
            ,nullptr
            ,alloc_fn
            ,free_fn
            ,true
            ,true
        );
    } else {
        return nullptr;
    }

    if ( strend - strbeg == 1 && *strbeg == '\0' ) {
        return parser;
    }

    fj_error_code ec{};
    auto toknum = fj_num_tokens(std::addressof(ec), strbeg, strend);
    if ( ec ) {
        parser->error = ec;

        return parser;
    }

    auto in_bytes = sizeof(fj_token) * toknum;
    auto *toksbeg = static_cast<fj_token *>(alloc_fn(in_bytes));
    std::memset(toksbeg, 0, in_bytes);
    auto *toksend = toksbeg + toknum;

    if ( parser ) {
        details::fj_init_parser(
             parser
            ,toksbeg
            ,toksend
            ,strbeg
            ,strend
            ,alloc_fn
            ,free_fn
            ,true
            ,true
        );
    }

    return parser;
}

inline void fj_free_parser(fj_parser *parser) {
    if ( parser->dyn_tokens && parser->toks_beg ) {
        parser->free_fn(parser->toks_beg);
    }

    parser->toks_beg = nullptr;
    parser->toks_cur = nullptr;
    parser->toks_end = nullptr;
    parser->error = FJ_EC_INVALID;

    if ( parser->dyn_parser ) {
        parser->free_fn(parser);
    }
}

/*************************************************************************************************/
// parsing

inline std::size_t fj_parse(fj_parser *parser) {
    std::size_t vlen = 0;
    assert(parser);
    if ( !parser->toks_beg ) {
        return 0;
    }

    parser->error = static_cast<fj_error_code>(details::fj_parse_value<true>(
         parser
        ,std::addressof(parser->toks_beg->m_val)
        ,std::addressof(vlen)
        ,std::addressof(parser->toks_beg->m_type)
        ,nullptr
    ));
    assert(vlen <= std::numeric_limits<__FJ__VLEN_TYPE>::max());
    parser->toks_beg->m_vlen = static_cast<__FJ__VLEN_TYPE>(vlen);
    parser->toks_end = parser->toks_cur;

    return parser->toks_cur - parser->toks_beg;
}

// returns the num of tokens
inline std::size_t fj_parse(fj_token *tokbeg, fj_token *tokend, const char *strbeg, const char *strend) {
    auto parser = fj_make_parser(tokbeg, tokend, strbeg, strend);

    return fj_parse(std::addressof(parser));
}

// returns the dyn-allocated parser
inline fj_parser* fj_parse(
     const char *strbeg
    ,const char *strend
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    auto *parser = fj_alloc_parser(strbeg, strend, alloc_fn, free_fn);
    fj_parse(parser);

    return parser;
}

/*************************************************************************************************/
// parser state

inline bool fj_is_valid(const fj_parser *parser) {
    return parser && parser->error == FJ_EC_OK;
}

inline fj_error_code fj_error(const fj_parser *parser) {
    return parser->error;
}

inline std::size_t fj_tokens(const fj_parser *parser) {
    return parser->toks_end - parser->toks_beg;
}

inline std::size_t fj_members(const fj_parser *parser) {
    return (!details::fj_is_simple_type(parser->toks_beg->m_type))
       ? parser->toks_beg->m_childs - 1
       : static_cast<std::size_t>(parser->toks_beg->m_type != FJ_TYPE_INVALID)
    ;
}

inline bool fj_empty(const fj_parser *parser) {
    return parser == nullptr
        || parser->toks_beg == parser->toks_end
        || fj_members(parser) == 0
    ;
}

inline bool fj_is_array(const fj_parser *parser)
{ return parser->toks_beg->m_type == FJ_TYPE_ARRAY; }

inline bool fj_is_object(const fj_parser *parser)
{ return parser->toks_beg->m_type == FJ_TYPE_OBJECT; }

inline bool fj_is_null(const fj_parser *parser)
{ return parser->toks_beg->m_type == FJ_TYPE_NULL; }

inline bool fj_is_bool(const fj_parser *parser)
{ return parser->toks_beg->m_type == FJ_TYPE_BOOL; }

inline bool fj_is_number(const fj_parser *parser)
{ return parser->toks_beg->m_type == FJ_TYPE_NUMBER; }

inline bool fj_is_string(const fj_parser *parser)
{ return parser->toks_beg->m_type == FJ_TYPE_STRING; }

inline bool fj_is_simple_type(const fj_parser *parser)
{ return details::fj_is_simple_type(parser->toks_beg->m_type); }

/*************************************************************************************************/
// iterators

struct fj_iterator {
    fj_token *m_beg;
    fj_token *m_cur;
    fj_token *m_end;

    string_view key() const { return {m_cur->m_key, m_cur->m_klen}; }
    string_view value() const { return {m_cur->m_val, m_cur->m_vlen}; }
    std::size_t childs() const { return m_cur->m_childs; }
    const fj_token* parent() const { return m_cur->m_parent; }
    const fj_token* end() const { return m_cur->m_end; }

    fj_token_type type() const { return m_cur->m_type; }
    const char* type_name() const { return fj_type_name(type()); }
    bool is_valid() const { return m_cur && type() != FJ_TYPE_INVALID; }
    bool is_array() const { return type() == FJ_TYPE_ARRAY; }
    bool is_object() const { return type() == FJ_TYPE_OBJECT; }
    bool is_null() const { return type() == FJ_TYPE_NULL; }
    bool is_bool() const { return type() == FJ_TYPE_BOOL; }
    bool is_number() const { return type() == FJ_TYPE_NUMBER; }
    bool is_string() const { return type() == FJ_TYPE_STRING; }
    bool is_simple_type() const { return details::fj_is_simple_type(type()); }

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

namespace details {

// dump using iterator
inline void fj_dump_tokens(std::FILE *stream, const char *caption, const fj_iterator &it) {
    std::fprintf(stream, "%s:\n", caption);
    fj_dump_tokens_impl(stream, it.m_beg, it.m_cur, it.m_end);
}

} // ns details

inline fj_iterator fj_iter_begin(const fj_parser *parser) {
    assert(parser && parser->toks_beg && parser->toks_end);
    return {parser->toks_beg, parser->toks_beg, parser->toks_end-1};
}

inline fj_iterator fj_iter_end(const fj_parser *parser) {
    assert(parser && parser->toks_beg && parser->toks_end);
    return {parser->toks_end-1, parser->toks_end-1, parser->toks_end-1};
}

inline fj_iterator fj_iter_begin(const fj_iterator &it) {
    return {it.m_cur, it.m_cur, it.m_cur->m_end};
}

inline fj_iterator fj_iter_end(const fj_iterator &it) {
    if ( !it.is_simple_type() ) {
        return {it.m_cur->m_end, it.m_cur->m_end, it.m_cur->m_end};
    }

    return {it.m_end, it.m_end, it.m_end};
}

inline fj_iterator fj_iter_next(const fj_iterator &it) {
    assert(it.m_cur != it.m_end);

    auto next = it.m_cur + 1;
    if ( next != it.m_end && next->m_parent == it.m_beg ) {
        return {it.m_beg, next, it.m_end};
    }

    for ( ; next != it.m_end && next->m_parent != it.m_beg; ++next )
        ;

    return {it.m_beg, next, it.m_end};
}

inline bool fj_iter_equal(const fj_iterator &l, const fj_iterator &r)
{ return l.m_cur == r.m_cur; }

inline bool fj_iter_not_equal(const fj_iterator &l, const fj_iterator &r)
{ return !fj_iter_equal(l, r); }

inline std::size_t fj_iter_members(const fj_iterator &it) {
    if ( it.is_simple_type() ) {
        return 0;
    }

    return it.m_cur->m_childs - 1;
}

namespace details {

// find by key name
inline fj_iterator fj_iter_find(const char *key, std::size_t len, fj_iterator it, const fj_iterator &end) {
    if ( !it.m_cur ) {
        return end;
    }
    if ( it.m_cur && it.m_cur->m_parent && it.m_cur->m_parent->m_type != FJ_TYPE_OBJECT ) {
        return end;
    }

    while ( fj_iter_not_equal(it, end) ) {
        if ( it.type() == FJ_TYPE_OBJECT_END ) {
            return end;
        }
        const auto sv = it.key();
        if ( sv.size() == len && sv.compare(0, len, key) == 0 ) {
            break;
        }

        it = it.is_simple_type()
            ? fj_iterator{it.m_beg, it.m_cur + 1, it.m_end}
            : fj_iterator{it.m_beg, it.m_cur->m_end + 1, it.m_end}
        ;
    }

    const auto type = it.type();
    switch ( type ) {
        case FJ_TYPE_STRING:
        case FJ_TYPE_NUMBER:
        case FJ_TYPE_BOOL:
        case FJ_TYPE_NULL: {
            return {it.m_beg, it.m_cur, it.m_cur + 1};
        }
        case FJ_TYPE_OBJECT:
        case FJ_TYPE_ARRAY: {
            return {it.m_cur, it.m_cur, it.m_cur->m_end + 1};
        }
        default: {
            if ( fj_iter_equal(it, end) && it.type() == FJ_TYPE_OBJECT_END ) {
                return end;
            }
        }
    }

    assert(!"unreachable!");
}

} // ns details

// at by key name, from the parser
inline fj_iterator fj_iter_at(const char *key, std::size_t klen, const fj_parser *parser) {
    assert(parser);
    assert(parser->toks_beg);

    bool is_simple = details::fj_is_simple_type(parser->toks_beg->m_type);
    fj_iterator beg = {
         parser->toks_beg
        ,parser->toks_beg + static_cast<std::size_t>(!is_simple)
        ,parser->toks_end
    };
    fj_iterator end = fj_iter_end(parser);

    return details::fj_iter_find(key, klen, beg, end);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
fj_iterator fj_iter_at(T key, const fj_parser *parser) {
    assert(parser);
    assert(parser->toks_beg);

    bool is_simple = details::fj_is_simple_type(parser->toks_beg->m_type);
    fj_iterator beg = {
         parser->toks_beg
        ,parser->toks_beg + static_cast<std::size_t>(!is_simple)
        ,parser->toks_end
    };
    fj_iterator end = fj_iter_end(parser);

    return details::fj_iter_find(key, std::strlen(key), beg, end);
}

template<std::size_t N>
fj_iterator fj_iter_at(const char (&key)[N], const fj_parser *parser) {
    assert(parser);
    assert(parser->toks_beg);

    bool is_simple = details::fj_is_simple_type(parser->toks_beg->m_type);
    fj_iterator beg = {
         parser->toks_beg
        ,parser->toks_beg + static_cast<std::size_t>(!is_simple)
        ,parser->toks_end
    };
    fj_iterator end = fj_iter_end(parser);

    return details::fj_iter_find(key, N-1, beg, end);
}

// at by key name, from the iterators
inline fj_iterator fj_iter_at(const char *key, std::size_t klen, const fj_iterator &it) {
    assert(it.m_cur != it.m_end);

    fj_iterator beg = {
         it.m_beg
        ,it.m_cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.m_end
    };
    auto end = fj_iter_end(it);

    return details::fj_iter_find(key, klen, beg, end);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
fj_iterator fj_iter_at(T key, const fj_iterator &it) {
    assert(it.m_cur != it.m_end);

    fj_iterator beg = {
         it.m_beg
        ,it.m_cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.m_end
    };
    auto end = fj_iter_end(it);

    return details::fj_iter_find(key, std::strlen(key), beg, end);
}

template<std::size_t N>
fj_iterator fj_iter_at(const char (&key)[N], const fj_iterator &it) {
    assert(it.m_cur != it.m_end);

    fj_iterator beg = {
         it.m_beg
        ,it.m_cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.m_end
    };
    auto end = fj_iter_end(it);

    return details::fj_iter_find(key, N-1, beg, end);
}

namespace details {

// find by index
inline fj_iterator fj_iter_find(std::size_t idx, fj_iterator it, const fj_iterator &end) {
    if ( !it.m_cur ) {
        return end;
    }
    if ( it.m_cur && it.m_cur->m_parent->m_type != FJ_TYPE_ARRAY ) {
        return end;
    }
    if ( idx >= it.m_cur->m_parent->m_childs ) {
        return end;
    }

    for ( ; fj_iter_not_equal(it, end) && idx; --idx ) {
        if ( it.type() == FJ_TYPE_ARRAY_END ) {
            return end;
        }

        it = it.is_simple_type()
            ? fj_iterator{it.m_beg, it.m_cur + 1, it.m_end}
            : fj_iterator{it.m_beg, it.m_cur->m_end + 1, it.m_end}
        ;
    }

    const auto type = it.type();
    switch ( type ) {
        case FJ_TYPE_STRING:
        case FJ_TYPE_NUMBER:
        case FJ_TYPE_BOOL:
        case FJ_TYPE_NULL: {
            return {it.m_beg, it.m_cur, it.m_cur + 1};
        }
        case FJ_TYPE_OBJECT:
        case FJ_TYPE_ARRAY: {
            return {it.m_cur, it.m_cur, it.m_cur->m_end};
        }
        default: {
            if ( fj_iter_equal(it, end) && it.type() == FJ_TYPE_ARRAY_END ) {
                return end;
            }
        }
    }

    assert(!"unreachable!");
}

} // ns details

// at by index, from a parser
inline fj_iterator fj_iter_at(std::size_t idx, const fj_parser *parser) {
    assert(parser);
    assert(parser->toks_beg);

    bool is_simple = details::fj_is_simple_type(parser->toks_beg->m_type);
    fj_iterator beg = {
         parser->toks_beg
        ,parser->toks_beg + static_cast<std::size_t>(!is_simple)
        ,parser->toks_end
    };
    fj_iterator end = fj_iter_end(parser);

    return details::fj_iter_find(idx, beg, end);
}

// at by index, from a iterator
inline fj_iterator fj_iter_at(std::size_t idx, const fj_iterator &it) {
    assert(it.m_cur != it.m_end);

    fj_iterator beg = {
         it.m_beg
        ,it.m_cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.m_end
    };
    fj_iterator end = fj_iter_end(it);

    return details::fj_iter_find(idx, beg, end);
}

/*************************************************************************************************/

template<bool CalcLength>
std::size_t fj_walk_tokens(
     const fj_token *toksbeg
    ,const fj_token *toksend
    ,std::size_t indent
    ,void *userdata
    ,void(*cb)(void *userdata, const char *ptr, std::size_t len))
{
    static const char indent_str[] = "                                                                                ";
    std::size_t indent_scope = 0;
    std::size_t length = 0;
    for ( auto *it = toksbeg; it != toksend; ++it ) {
        if ( it != toksbeg ) {
            fj_token_type ctype = it->m_type;
            fj_token_type ptype = (it-1)->m_type;
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

        switch ( it->m_type ) {
            case FJ_TYPE_OBJECT: {
                if ( it->m_key ) {
                    if ( !CalcLength ) {
                        if ( indent ) {
                            cb(userdata, indent_str, indent_scope);
                        }
                        cb(userdata, "\"", 1);
                        cb(userdata, it->m_key, it->m_klen);
                        cb(userdata, "\":", 2);
                    }
                    length += 1+2;
                    length += it->m_klen;
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
                        if ( indent_scope - indent ) {
                            cb(userdata, indent_str, indent_scope - indent);
                        }
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
                if ( it->m_key ) {
                    if ( !CalcLength ) {
                        if ( indent ) {
                            cb(userdata, indent_str, indent_scope);
                        }
                        cb(userdata, "\"", 1);
                        cb(userdata, it->m_key, it->m_klen);
                        cb(userdata, "\":", 2);
                    }
                    length += 1+2;
                    length += it->m_klen;
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
                if ( it->m_parent->m_type != FJ_TYPE_ARRAY ) {
                    if ( !CalcLength ) {
                        if ( indent ) {
                            cb(userdata, indent_str, indent_scope);
                        }
                        cb(userdata, "\"", 1);
                        cb(userdata, it->m_key, it->m_klen);
                        cb(userdata, "\":", 2);
                    }
                    length += 1+2;
                    length += it->m_klen;
                    if ( indent ) {
                        length += indent_scope;
                    }
                } else if ( it->m_parent->m_type == FJ_TYPE_ARRAY ) {
                    if ( !CalcLength ) {
                        if ( indent ) {
                            cb(userdata, indent_str, indent_scope);
                        }
                    }
                    if ( indent ) {
                        length += indent_scope;
                    }
                }
                switch ( it->m_type ) {
                    case FJ_TYPE_NULL:
                    case FJ_TYPE_BOOL:
                    case FJ_TYPE_NUMBER: {
                        if ( !CalcLength ) {
                            cb(userdata, it->m_val, it->m_vlen);
                        }
                        length += it->m_vlen;
                        break;
                    }
                    case FJ_TYPE_STRING: {
                        if ( !CalcLength ) {
                            cb(userdata, "\"", 1);
                            cb(userdata, it->m_val, it->m_vlen);
                            cb(userdata, "\"", 1);
                        }
                        length += 2;
                        length += it->m_vlen;
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

inline std::size_t fj_length_for_string(
     const fj_iterator &beg
    ,const fj_iterator &end
    ,std::size_t indent = 0)
{
    return fj_walk_tokens<true>(beg.m_cur, end.m_end + 1, indent, nullptr, nullptr);
}

/*************************************************************************************************/

#ifdef __FJ__SUPPORTS_STDIO

namespace details {

static void tokens_to_stream_cb_0(void *userdata, const char *ptr, std::size_t len) {
    auto *fd = static_cast<int *>(userdata);
    auto wr = ::write(*fd, ptr, len);
    assert(wr > 0);
    assert(len == static_cast<std::size_t>(wr));
}

static void tokens_to_stream_cb_1(void *userdata, const char *ptr, std::size_t len) {
    auto *stream = static_cast<std::FILE*>(userdata);
    auto wr = std::fwrite(ptr, 1, len, stream);
    assert(wr == len);
}

} // ns details

inline std::size_t fj_serialize(
     int fd
    ,const fj_iterator &beg
    ,const fj_iterator &end
    ,std::size_t indent = 0)
{
    return fj_walk_tokens<false>(
         beg.m_cur
        ,end.m_end + 1
        ,indent
        ,std::addressof(fd)
        ,details::tokens_to_stream_cb_0
    );
}

inline std::size_t fj_serialize(
     std::FILE *stream
    ,const fj_iterator &beg
    ,const fj_iterator &end
    ,std::size_t indent = 0)
{
    return fj_walk_tokens<false>(
         beg.m_cur
        ,end.m_end + 1
        ,indent
        ,stream
        ,details::tokens_to_stream_cb_1
    );
}

#endif // __FJ__SUPPORTS_STDIO

/*************************************************************************************************/

namespace details {

static void tokens_to_stream_cb_2(void *userdata, const char *ptr, std::size_t len) {
    auto *stream = static_cast<std::ostream *>(userdata);
    stream->write(ptr, len);
}

inline std::size_t fj_tokens_to_stream(
     std::ostream &stream
    ,const fj_token *beg
    ,const fj_token *end
    ,std::size_t indent)
{
    return fj_walk_tokens<false>(
         beg
        ,end
        ,indent
        ,std::addressof(stream)
        ,details::tokens_to_stream_cb_2
    );
}

} // ns details

inline std::size_t fj_serialize(
     std::ostream &stream
    ,const fj_iterator &beg
    ,const fj_iterator &end
    ,std::size_t indent = 0)
{
    return details::fj_tokens_to_stream(stream, beg.m_cur, end.m_end + 1, indent);
}

/*************************************************************************************************/

namespace details {

struct tokens_to_buf_userdata {
    char *ptr;
    const char *end;
};

static void tokens_to_buf_cb(void *userdata, const char *ptr, std::size_t len) {
    auto *p = static_cast<tokens_to_buf_userdata *>(userdata);
    assert(p->ptr + len <= p->end);
    std::memcpy(p->ptr, ptr, len);
    p->ptr += len;
}

} // ns details

/*************************************************************************************************/

inline std::size_t fj_serialize(
     const fj_iterator &beg
    ,const fj_iterator &end
    ,char *buf
    ,std::size_t bufsize
    ,std::size_t indent = 0)
{
    details::tokens_to_buf_userdata userdata{buf, buf + bufsize};
    return fj_walk_tokens<false>(
         beg.m_cur
        ,end.m_end + 1
        ,indent
        ,std::addressof(userdata)
        ,details::tokens_to_buf_cb
    );
}

inline std::string fj_to_string(
     const fj_iterator &beg
    ,const fj_iterator &end
    ,std::size_t indent = 0)
{
    std::string res;
    auto length = fj_length_for_string(beg, end, indent);
    res.resize(length);

    fj_serialize(beg, end, std::addressof(res[0]), res.size(), indent);

    return res;
}

/*************************************************************************************************/

inline std::size_t fj_get_keys(
     fj_iterator it
    ,const fj_iterator &end
    ,void *userdata
    ,void(*cb)(void *userdata, const char *ptr, std::size_t len))
{
    std::size_t cnt{};

    if ( !it.is_object() ) {
        return 0;
    }

    it.m_cur += 1;
    for ( ; fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        if ( cb ) {
            auto key = it.key();
            cb(userdata, key.data(), key.size());
        }

        ++cnt;
    }

    return cnt;
}

namespace details {

static void get_fj_keys_cb(void *userdata, const char *ptr, std::size_t len) {
    auto *vec = static_cast<std::vector<string_view> *>(userdata);
    vec->push_back(string_view{ptr, len});
}

} // ns details

inline std::vector<string_view> fj_get_keys(fj_iterator it,const fj_iterator &end) {
    auto num = fj_get_keys(it, end, nullptr, nullptr);
    std::vector<string_view> res;
    res.reserve(num);

    fj_get_keys(it, end, std::addressof(res), details::get_fj_keys_cb);

    return res;
}

/*************************************************************************************************/

struct fjson {
    struct const_iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = const fj_iterator;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using reference = value_type &;
        using const_reference = const value_type &;

        explicit const_iterator(fj_iterator it)
            :m_it{std::move(it)}
        {}

        const_pointer operator->() const { return std::addressof(m_it); }
        const_iterator& operator++() { m_it = fj_iter_next(m_it); return *this; }
        const_iterator operator++(int) { const_iterator tmp{*this}; ++(*this); return tmp; }
        reference operator* () { return m_it; }
        const_reference operator* () const { return m_it; }
        friend bool operator== (const const_iterator &l, const const_iterator &r)
        { return fj_iter_equal(l.m_it, r.m_it); }
        friend bool operator!= (const const_iterator &l, const const_iterator &r)
        { return !operator==(l, r); }

        fj_iterator m_it;
    };

private:
    struct intrusive_ptr {
        using deleter_fn_ptr = void(*)(fj_parser *);

        intrusive_ptr(bool manage, fj_parser *parser, deleter_fn_ptr free_fn)
            :m_manage{manage}
            ,m_parser{parser}
            ,m_free_fn{free_fn}
        {
            if ( m_manage && m_parser ) {
                fj_parser::inc_refcnt(m_parser);
            }
        }
        virtual ~intrusive_ptr() {
            if ( m_manage && m_parser ) {
                auto refcnt = fj_parser::dec_refcnt(m_parser);
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
                fj_parser::inc_refcnt(m_parser);
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
                fj_parser::inc_refcnt(m_parser);
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
        const fj_parser* get()         const { return m_parser; }
        fj_parser*       get()               { return m_parser; }
        const fj_parser* operator-> () const { return m_parser; }
        fj_parser*       operator-> ()       { return m_parser; }
        const fj_parser& operator*  () const { return *m_parser; }
        fj_parser&       operator*  ()       { return *m_parser; }

    private:
        bool m_manage;
        fj_parser *m_parser;
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
    fjson(fj_parser *parser)
        :m_parser{false, parser, [](fj_parser *){}}
        ,m_beg{fj_iter_begin(parser)}
        ,m_end{fj_iter_end(parser)}
    {
        assert(fj_is_valid(parser));
    }

    // construct and parse using user-provided array of tokens and parser
    template<std::size_t N>
    fjson(
         fj_parser *parser
        ,fj_token *toksbeg
        ,fj_token *toksend
        ,const char (&str)[N]
    )
        :m_parser{
             false
            ,(*parser = fj_make_parser(toksbeg, toksend, std::begin(str), std::end(str)), parser)
            ,[](fj_parser *){}
        }
        ,m_beg{}
        ,m_end{}
    {
        fj_parse(m_parser.get());
        if ( fj_is_valid(m_parser.get()) ) {
            m_beg = fj_iter_begin(m_parser.get());
            m_end = fj_iter_end(m_parser.get());
        }
    }

    // construct and parse using user-provided array of tokens and parser
    fjson(
         fj_parser *parser
        ,fj_token *toksbeg
        ,fj_token *toksend
        ,const char *strbeg
        ,const char *strend
    )
        :m_parser{
             false
            ,(*parser = fj_make_parser(toksbeg, toksend, strbeg, strend), parser)
            ,[](fj_parser *){}
        }
        ,m_beg{}
        ,m_end{}
    {
        fj_parse(m_parser.get());
        if ( fj_is_valid(m_parser.get()) ) {
            m_beg = fj_iter_begin(m_parser.get());
            m_end = fj_iter_end(m_parser.get());
        }
    }

    // construct and parse using user-provided array of tokens and dyn-allocated parser
    fjson(
         fj_token *toksbeg
        ,fj_token *toksend
        ,const char *strbeg
        ,const char *strend
    )
        :m_parser{true, fj_alloc_parser(toksbeg, toksend, strbeg, strend), fj_free_parser}
        ,m_beg{}
        ,m_end{}
    {
        fj_parse(m_parser.get());
        if ( fj_is_valid(m_parser.get()) ) {
            m_beg = fj_iter_begin(m_parser.get());
            m_end = fj_iter_end(m_parser.get());
        }
    }

    // construct and parse using user-provided parser and dyn-allocated tokens
    template<std::size_t N>
    fjson(
         fj_parser *parser
        ,const char (&str)[N]
        ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
        ,fj_free_fnptr free_fn = std::addressof(free)
    )
        :m_parser{
             true
            ,(*parser = fj_make_parser(std::begin(str), std::end(str), alloc_fn, free_fn), parser)
            ,fj_free_parser
        }
        ,m_beg{}
        ,m_end{}
    {
        fj_parse(m_parser.get());
        if ( fj_is_valid(m_parser.get()) ) {
            m_beg = fj_iter_begin(m_parser.get());
            m_end = fj_iter_end(m_parser.get());
        }
    }

    // construct and parse using user-provided parser and dyn-allocated tokens
    fjson(
         fj_parser *parser
        ,const char *strbeg
        ,const char *strend
        ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
        ,fj_free_fnptr free_fn = std::addressof(free)
    )
        :m_parser{
             true
            ,(*parser = fj_make_parser(strbeg, strend, alloc_fn, free_fn), parser)
            ,fj_free_parser
        }
        ,m_beg{}
        ,m_end{}
    {
        fj_parse(m_parser.get());
        if ( fj_is_valid(m_parser.get()) ) {
            m_beg = fj_iter_begin(m_parser.get());
            m_end = fj_iter_end(m_parser.get());
        }
    }

    // construct and parse using dyn-allocated parser and dyn-allocated tokens
    template<std::size_t N>
    fjson(
         const char (&str)[N]
        ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
        ,fj_free_fnptr free_fn = std::addressof(free)
    )
        :m_parser{
             true
            ,fj_alloc_parser(std::begin(str), std::end(str), alloc_fn, free_fn)
            ,fj_free_parser
        }
        ,m_beg{}
        ,m_end{}
    {
        fj_parse(m_parser.get());
        if ( fj_is_valid(m_parser.get()) ) {
            m_beg = fj_iter_begin(m_parser.get());
            m_end = fj_iter_end(m_parser.get());
        }
    }

    // construct and parse using dyn-allocated tokens and dyn-allocated parser
    fjson(
         const char *beg
        ,const char *end
        ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
        ,fj_free_fnptr free_fn = std::addressof(free)
    )
        :m_parser{true, fj_alloc_parser(beg, end, alloc_fn, free_fn), fj_free_parser}
        ,m_beg{}
        ,m_end{}
    {
        fj_parse(m_parser.get());
        if ( fj_is_valid(m_parser.get()) ) {
            m_beg = fj_iter_begin(m_parser.get());
            m_end = fj_iter_end(m_parser.get());
        }
    }

    virtual ~fjson() = default;

private:
    fjson(intrusive_ptr parser, fj_iterator beg, fj_iterator end)
        :m_parser{std::move(parser)}
        ,m_beg{std::move(beg)}
        ,m_end{std::move(end)}
    {}

public:
    bool is_valid() const { return m_parser && fj_is_valid(m_parser.get()); }
    fj_error_code error() const { return m_parser->error; }
    const char* error_string() const { return fj_error_string(m_parser->error); }

    // number of direct childs for OBJECT/ARRAY, or 1 for a valid SIMPLE type
    std::size_t size() const { return m_beg.members(); }
    std::size_t members() const { return m_beg.members(); }
    bool is_empty() const { return size() == 0; }

    // total number of tokens
    std::size_t tokens() const { return m_parser->toks_end - m_parser->toks_beg; }

    fj_token_type type() const { return m_beg.type(); }
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
    { auto it = fj_iter_at(key, len, m_beg); return fj_iter_not_equal(it, m_end); }

    // for objects
    template<std::size_t N>
    fjson at(const char (&key)[N]) const { return at(key, N-1); }
    template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
    fjson at(T key) const { return at(key, std::strlen(key)); }
    fjson at(const char *key, std::size_t len) const {
        auto it = fj_iter_at(key, len, m_beg);
        if ( fj_iter_equal(it, m_end) ) {
            return {m_parser, m_end, m_end};
        }

        return {m_parser, it, fj_iter_end(it)};
    }
    // for arrays
    fjson at(std::size_t idx) const {
        auto it = fj_iter_at(idx, m_beg);
        if ( fj_iter_equal(it, m_end) ) {
            return {m_parser, m_end, m_end};
        }

        return {m_parser, it, fj_iter_end(it)};
    }

    // get a fjson object at iterator position
    fjson at(const const_iterator &it) const
    { return {m_parser, fj_iter_begin(it.m_it), fj_iter_end(it.m_it)}; }

    // for arrays
    fjson operator[](std::size_t idx) const { return at(idx); }

    // for objects
    template<std::size_t N>
    fjson operator[](const char (&key)[N]) const { return at(key, N-1); }
    template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
    fjson operator[](T key) const { return at(key, std::strlen(key)); }

    std::string dump(std::size_t indent = 0) const
    { return fj_to_string(m_beg, m_end, indent); }
    std::ostream& dump(std::ostream &os, std::size_t indent = 0) const
    { fj_serialize(os, m_beg, m_end, indent); return os; }
    friend std::ostream& operator<< (std::ostream &os, const fjson &fj)
    { return fj.dump(os); }

    // for top level object/array only
    std::size_t keys_num() const
    { return fj_get_keys(m_beg, m_end, nullptr, nullptr); }
    std::vector<string_view>
    keys() const
    { return fj_get_keys(m_beg, m_end); }

private:
    intrusive_ptr m_parser;
    fj_iterator   m_beg;
    fj_iterator   m_end;
};

/*************************************************************************************************/

// dyn tokens and dyn parser
inline fjson parse(
     const char *beg
    ,const char *end
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    return fjson{beg, end, alloc_fn, free_fn};
}

template<std::size_t N>
inline fjson parse(
     const char (&str)[N]
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    return parse(str, str + N-1, alloc_fn, free_fn);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
inline fjson parse(
     T beg
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    const auto *end = beg + std::strlen(beg);

    return parse(beg, end, alloc_fn, free_fn);
}

// user-provided parser and dyn tokens
inline fjson parse(
     fj_parser *parser
    ,const char *beg
    ,const char *end
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    return fjson{parser, beg, end, alloc_fn, free_fn};
}

template<std::size_t N>
inline fjson parse(
     fj_parser *parser
    ,const char (&str)[N]
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    return parse(parser, str, str + N-1, alloc_fn, free_fn);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
inline fjson parse(
     fj_parser *parser
    ,T beg
    ,fj_alloc_fnptr alloc_fn = std::addressof(malloc)
    ,fj_free_fnptr free_fn = std::addressof(free))
{
    const auto *end = beg + std::strlen(beg);

    return parse(parser, beg, end, alloc_fn, free_fn);
}

// user-provided parser and tokens
inline fjson parse(
     fj_parser *parser
    ,fj_token *toksbeg
    ,fj_token *toksend
    ,const char *strbeg
    ,const char *strend)
{
    return fjson{parser, toksbeg, toksend, strbeg, strend};
}

template<std::size_t N>
inline fjson parse(
     fj_parser *parser
    ,fj_token *toksbeg
    ,fj_token *toksend
    ,const char (&str)[N])
{
    return parse(parser, toksbeg, toksend, str, str + N-1);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
inline fjson parse(
     fj_parser *parser
    ,fj_token *toksbeg
    ,fj_token *toksend
    ,T beg)
{
    const auto *end = beg + std::strlen(beg);

    return parse(parser, toksbeg, toksend, beg, end);
}

} // ns flatjson

/*************************************************************************************************/

// undef internally used macro-vars
#undef __FJ__FALLTHROUGH
#undef __FJ__STRINGIZE_I
#undef __FJ__STRINGIZE
#undef __FJ__MAKE_ERROR_MESSAGE
#undef __FJ__CHECK_OVERFLOW
#undef __FJ__KLEN_TYPE
#undef __FJ__VLEN_TYPE
#undef __FJ__CHILDS_TYPE
#undef __FJ__CUR_CHAR

/*************************************************************************************************/

#endif // __FLATJSON__FLATJSON_HPP
