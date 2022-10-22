
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

#include "version.hpp"

//#include <iostream> // TODO: comment out

#include <ostream>
#include <vector>
#include <string>
#include <limits>

#include <cassert>
#include <cstdint>
#include <cstring>

/*************************************************************************************************/

#ifdef __FJ__DONT_CHECK_OVERFLOW
#   define __FJ__CHECK_OVERFLOW(expr, type, err)
#else
#   define __FJ__CHECK_OVERFLOW(expr, type, err) \
        if ( (expr) >= (std::numeric_limits<type>::max)() ) return err
#endif //__FLATJSON__SHOULD_CHECK_OVERFLOW

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

enum error_code: int {
     FJ_EC_OK = 0
    ,FJ_EC_INVALID = -1
    ,FJ_EC_INCOMPLETE = -2
    ,FJ_EC_NO_FREE_TOKENS = -3
    ,FJ_EC_KLEN_OVERFLOW = -4
    ,FJ_EC_VLEN_OVERFLOW = -5
    ,FJ_EC_CHILDS_OVERFLOW = -6
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

/*************************************************************************************************/

namespace details {

#define fj_is_simple_type_macro(v) \
    (v > ::flatjson::FJ_TYPE_INVALID && v < ::flatjson::FJ_TYPE_OBJECT)

#define fj_is_digit_macro(ch) \
    (ch >= '0' && ch <= '9')

#define fj_is_hex_digit_macro(ch) \
    (fj_is_digit_macro(ch) || ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')))

#define fj_is_hex_digit4_macro(ch0, ch1, ch2, ch3) \
    (fj_is_hex_digit_macro(ch0) && fj_is_hex_digit_macro(ch1) \
        && fj_is_hex_digit_macro(ch2) && fj_is_hex_digit_macro(ch3))

inline std::size_t fj_utf8_char_len(std::uint8_t ch) {
    static constexpr std::uint8_t map[] = {
         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        ,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        ,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        ,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
        ,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
        ,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
        ,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
    };
    static_assert(sizeof(map) == UINT8_MAX+1, "");

    return map[ch];
}

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
    const char *str_beg;
    const char *str_cur;
    const char *str_end;

    token *toks_beg;
    token *toks_cur;
    token *toks_end;

    alloc_fnptr alloc_fn;
    free_fnptr  free_fn;
    error_code  error;
    bool dyn_tokens;
    bool dyn_parser;
    std::uint32_t ref_cnt;

    static std::size_t inc_refcnt(parser *parser) { return ++parser->ref_cnt; }
    static std::size_t dec_refcnt(parser *parser) { return --parser->ref_cnt; }
};

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

// dump using parser
inline void dump_tokens(std::FILE *stream, const char *caption, parser *parser, std::size_t indent = 3) {
    std::fprintf(stream, "%s:\n", caption);
    dump_tokens_impl(stream, parser->toks_beg, parser->toks_beg, parser->toks_end, indent);
}

/*************************************************************************************************/

inline const char* fj_skip_ws(const char *s) {
    constexpr bool t = true;
    constexpr bool f = false;
    static const bool map[] = {
         f,f,f,f,f,f,f,f,f,t,t,f,f,t,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,t,f,f,f,f
        ,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f
        ,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f
        ,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f
        ,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f
        ,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f
        ,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f
    };
    static_assert(sizeof(map) == 256, "");

    for ( ;map[static_cast<std::uint8_t>(*s)]; ++s );

    return s;
}

#define __FJ__CUR_CHAR(p) \
    ((p->str_cur = fj_skip_ws(p->str_cur)), (*(p->str_cur) == 0 ? ((int)-1) : *(p->str_cur)))

inline error_code check_and_skip(parser *p, char expected) {
    char ch = *(p->str_cur);
    if ( ch == expected ) {
        p->str_cur++;

        return FJ_EC_OK;
    }

    if ( ch == ((int)-1) ) {
        return FJ_EC_INCOMPLETE;
    }

    return FJ_EC_INVALID;
}

inline error_code escape_len(int *escape_len, const char *s, std::size_t len) {
    switch ( *s ) {
        case 'u': {
            if ( len < 6 ) {
                return FJ_EC_INCOMPLETE;
            }
            if ( fj_is_hex_digit4_macro(*(s+1), *(s+2), *(s+3), *(s+4)) ) {
                *escape_len = 5;

                return FJ_EC_OK;
            }

            return FJ_EC_INVALID;
        }
        case '"':
        case '\\':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't': {
            if ( len < 2 ) {
                return FJ_EC_INCOMPLETE;
            }

            *escape_len = 1;

            return FJ_EC_OK;
        }
    }

    return FJ_EC_INVALID;
}

/*************************************************************************************************/

template<bool ParseMode, std::size_t ExLen>
inline error_code expect(parser *p, const char (&s)[ExLen], const char **ptr, std::size_t *size) {
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
inline error_code parse_string(parser *p, const char **value, std::size_t *vlen) {
    auto ec = check_and_skip(p, '"');
    if ( ec != FJ_EC_OK ) {
        return ec;
    }

    std::uint8_t ch = 0;
    auto *start = p->str_cur;
    for ( std::size_t len = 0; p->str_cur < p->str_end; p->str_cur += len ) {
        ch = *(p->str_cur);
        len = fj_utf8_char_len((unsigned char)ch);
        if ( !(ch >= 32 && len > 0) ) {
            return FJ_EC_INVALID;
        }
        if ( static_cast<std::ptrdiff_t>(len) > (p->str_end - p->str_cur) ) {
            return FJ_EC_INCOMPLETE;
        }

        if ( ch == '\\' ) {
            int n = 0;
            ec = escape_len(&n, p->str_cur + 1, p->str_end - p->str_cur);
            if ( ec != FJ_EC_OK ) {
                return ec;
            }
            len += n;
        } else if ( ch == '"' ) {
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *value = start;
                *vlen = p->str_cur - start;
            }

            ++p->str_cur;

            break;
        }
    }

    return ch == '"' ? FJ_EC_OK : FJ_EC_INCOMPLETE;
}

template<bool ParseMode>
inline error_code parse_number(parser *p, const char **value, std::size_t *vlen) {
    auto *start = p->str_cur;
    p->str_cur = (*(p->str_cur) == '-') ? p->str_cur + 1 : p->str_cur;
    if ( p->str_cur >= p->str_end ) {
        return FJ_EC_INCOMPLETE;
    }
    if ( p->str_cur + 1 < p->str_end && *(p->str_cur) == '0' && *(p->str_cur+1) == 'x' ) {
        p->str_cur += 2;
        if ( p->str_cur >= p->str_end ) {
            return FJ_EC_INCOMPLETE;
        }
        if ( !fj_is_hex_digit_macro(*(p->str_cur)) ) {
            return FJ_EC_INVALID;
        }
        for ( ; p->str_cur < p->str_end && fj_is_hex_digit_macro(*(p->str_cur))
              ; ++p->str_cur )
        {}
    } else {
        if ( !fj_is_digit_macro(*(p->str_cur)) ) {
            return FJ_EC_INVALID;
        }
        for ( ; p->str_cur < p->str_end && fj_is_digit_macro(*(p->str_cur))
              ; ++p->str_cur )
        {}
        if ( p->str_cur < p->str_end && *(p->str_cur) == '.' ) {
            p->str_cur++;
            if ( p->str_cur >= p->str_end ) {
                return FJ_EC_INCOMPLETE;
            }
            if ( !fj_is_digit_macro(*(p->str_cur)) ) {
                return FJ_EC_INVALID;
            }
            for ( ; p->str_cur < p->str_end && fj_is_digit_macro(*(p->str_cur))
                  ; ++p->str_cur )
            {}
        }
        if ( p->str_cur < p->str_end && (*(p->str_cur) == 'e' || *(p->str_cur) == 'E') ) {
            p->str_cur++;
            if ( p->str_cur >= p->str_end ) {
                return FJ_EC_INCOMPLETE;
            }
            if ( *(p->str_cur) == '+' || *(p->str_cur) == '-' ) {
                p->str_cur++;
            }
            if ( p->str_cur >= p->str_end ) {
                return FJ_EC_INCOMPLETE;
            }
            if ( !fj_is_digit_macro(*(p->str_cur)) ) {
                return FJ_EC_INVALID;
            }
            for ( ; p->str_cur < p->str_end && fj_is_digit_macro(*(p->str_cur))
                  ; ++p->str_cur )
            {}
        }
    }

    if ( (p->str_cur - start) > 1 && (start[0] == '0' && start[1] != '.') ) {
        return FJ_EC_INVALID;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        *value = start;
        *vlen = p->str_cur - start;
    }

    return FJ_EC_OK;
}

template<bool ParseMode>
inline error_code parse_value(
     parser *p
    ,const char **value
    ,std::size_t *vlen
    ,token_type *toktype
    ,token *parent
);

template<bool ParseMode>
inline error_code parse_array(parser *p, token *parent) {
    auto ec = check_and_skip(p, '[');
    if ( ec != FJ_EC_OK ) {
        return ec;
    }

    __FJ__CONSTEXPR_IF ( ParseMode ) {
        if ( p->toks_cur == p->toks_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }
    }

    auto *startarr = p->toks_cur++;
    __FJ__CONSTEXPR_IF( ParseMode ) {
        startarr->type = FJ_TYPE_ARRAY;
        startarr->flags = 1;
        startarr->parent = parent;
        if ( startarr->parent ) {
            __FJ__CHECK_OVERFLOW(startarr->parent->childs, FJ_CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
            ++startarr->parent->childs;
        }
    }

    while ( __FJ__CUR_CHAR(p) != ']' ) {
        __FJ__CONSTEXPR_IF ( ParseMode ) {
            if ( p->toks_cur == p->toks_end ) {
                return FJ_EC_NO_FREE_TOKENS;
            }
        }

        auto *current_token = p->toks_cur++;
        __FJ__CONSTEXPR_IF( ParseMode ) {
            *current_token = token{};
        }
        char ch = *(p->str_cur);
        if ( ch == '{' || ch == '[' ) {
            p->toks_cur -= 1;
        } else {
            __FJ__CONSTEXPR_IF( ParseMode ) {
                current_token->parent = startarr;
                __FJ__CHECK_OVERFLOW(startarr->childs, FJ_CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
                ++startarr->childs;
            }
        }

        std::size_t size = 0;
        ec = parse_value<ParseMode>(
             p
            ,&(current_token->val)
            ,&size
            ,&(current_token->type)
            ,startarr
        );
        if ( ec != FJ_EC_OK ) {
            return ec;
        }
        __FJ__CONSTEXPR_IF( ParseMode ) {
            startarr->flags = (startarr->flags == 0)
                ? startarr->flags
                : fj_is_simple_type_macro(current_token->type)
            ;
            __FJ__CHECK_OVERFLOW(size, FJ_VLEN_TYPE, FJ_EC_VLEN_OVERFLOW);
            current_token->vlen = static_cast<FJ_VLEN_TYPE>(size);
        }

        if ( __FJ__CUR_CHAR(p) == ',' ) {
            p->str_cur++;
            if ( *(p->str_cur) == ']' ) {
                return FJ_EC_INVALID;
            }
        }
    }

    ec = check_and_skip(p, ']');
    if ( ec != FJ_EC_OK ) {
        return ec;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        if ( p->toks_cur == p->toks_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }
        auto *endarr = p->toks_cur++;
        *endarr = token{};
        endarr->type = FJ_TYPE_ARRAY_END;
        endarr->parent = startarr;
        __FJ__CHECK_OVERFLOW(endarr->parent->childs, FJ_CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
        ++endarr->parent->childs;
        startarr->end = endarr;
    } else {
        ++p->toks_cur;
    }

    return FJ_EC_OK;
}

template<bool ParseMode>
inline error_code parse_object(parser *p, token *parent) {
    auto ec = check_and_skip(p, '{');
    if ( ec != FJ_EC_OK ) {
        return ec;
    }

    __FJ__CONSTEXPR_IF ( ParseMode ) {
        if ( p->toks_cur == p->toks_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }
    }

    auto *startobj = p->toks_cur++;
    __FJ__CONSTEXPR_IF( ParseMode ) {
        startobj->type = FJ_TYPE_OBJECT;
        startobj->flags = 1;
        startobj->parent = parent;
        if ( startobj->parent ) {
            __FJ__CHECK_OVERFLOW(startobj->parent->childs, FJ_CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
            ++startobj->parent->childs;
        }
    }

    while ( __FJ__CUR_CHAR(p) != '}' ) {
        char ch = *(p->str_cur);
        if ( ch != '"' ) {
            if ( ch == ((int)-1) ) {
                return FJ_EC_INCOMPLETE;
            }

            return FJ_EC_INVALID;
        }

        __FJ__CONSTEXPR_IF ( ParseMode ) {
            if ( p->toks_cur == p->toks_end ) {
                return FJ_EC_NO_FREE_TOKENS;
            }
        }

        auto *current_token = p->toks_cur++;
        __FJ__CONSTEXPR_IF( ParseMode ) {
            *current_token = token{};
        }
        std::size_t size = 0;
        ec = parse_value<ParseMode>(
             p
            ,&(current_token->key)
            ,&size
            ,&(current_token->type)
            ,startobj
        );
        if ( ec != FJ_EC_OK ) {
            return ec;
        }
        __FJ__CONSTEXPR_IF( ParseMode ) {
            __FJ__CHECK_OVERFLOW(size, FJ_KLEN_TYPE, FJ_EC_KLEN_OVERFLOW);
            current_token->klen = static_cast<FJ_KLEN_TYPE>(size);
        }

        ec = check_and_skip(p, ':');
        if ( ec != FJ_EC_OK ) {
            return ec;
        }

        ch = __FJ__CUR_CHAR(p);
        if ( ch == '[' || ch == '{' ) {
            p->toks_cur -= 1;
            const char *unused_str;
            std::size_t unused_size;
            ec = parse_value<ParseMode>(
                 p
                ,&unused_str
                ,&unused_size
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
                __FJ__CHECK_OVERFLOW(startobj->childs, FJ_CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
                ++startobj->childs;
            }

            ec = parse_value<ParseMode>(
                 p
                ,&(current_token->val)
                ,&size
                ,&(current_token->type)
                ,startobj
            );
            __FJ__CONSTEXPR_IF( ParseMode ) {
                __FJ__CHECK_OVERFLOW(size, FJ_VLEN_TYPE, FJ_EC_VLEN_OVERFLOW);
                current_token->vlen = static_cast<FJ_VLEN_TYPE>(size);
            }
        }

        if ( ec != FJ_EC_OK ) {
            return ec;
        }

        if ( __FJ__CUR_CHAR(p) == ',' ) {
            p->str_cur++;
            if ( *(p->str_cur) == '}' ) {
                return FJ_EC_INVALID;
            }
        }
    }

    ec = check_and_skip(p, '}');
    if ( ec != FJ_EC_OK ) {
        return ec;
    }

    __FJ__CONSTEXPR_IF( ParseMode ) {
        if ( p->toks_cur == p->toks_end ) {
            return FJ_EC_NO_FREE_TOKENS;
        }
        auto *endobj = p->toks_cur++;
        *endobj = token{};
        endobj->type = FJ_TYPE_OBJECT_END;
        endobj->parent = startobj;
        __FJ__CHECK_OVERFLOW(endobj->parent->childs, FJ_CHILDS_TYPE, FJ_EC_CHILDS_OVERFLOW);
        endobj->parent->childs += 1;
        startobj->end = endobj;
    } else {
        ++p->toks_cur;
    }

    return FJ_EC_OK;
}

template<bool ParseMode>
inline error_code parse_value(
     parser *p
    ,const char **value
    ,std::size_t *vlen
    ,token_type *toktype
    ,token *parent)
{
    char ch = __FJ__CUR_CHAR(p);
    switch ( ch ) {
        case '{': {
            auto ec = parse_object<ParseMode>(p, parent);
            if ( ec != FJ_EC_OK ) {
                return ec;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_OBJECT;
            }
            break;
        }
        case '[': {
            auto ec = parse_array<ParseMode>(p, parent);
            if ( ec != FJ_EC_OK ) {
                return ec;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_ARRAY;
            }
            break;
        }
        case 'n': {
            auto ec = expect<ParseMode>(p, "null", value, vlen);
            if ( ec != FJ_EC_OK ) {
                return ec;
            }
            // on root token
            if ( p->toks_cur == p->toks_beg ) {
                ++p->toks_cur;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_NULL;
            }
            break;
        }
        case 't': {
            auto ec = expect<ParseMode>(p, "true", value, vlen);
            if ( ec != FJ_EC_OK ) {
                return ec;
            }
            // on root token
            if ( p->toks_cur == p->toks_beg ) {
                ++p->toks_cur;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_BOOL;
            }
            break;
        }
        case 'f': {
            auto ec = expect<ParseMode>(p, "false", value, vlen);
            if ( ec != FJ_EC_OK ) {
                return ec;
            }
            // on root token
            if ( p->toks_cur == p->toks_beg ) {
                ++p->toks_cur;
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
            auto ec = parse_number<ParseMode>(p, value, vlen);
            if ( ec != FJ_EC_OK ) {
                return ec;
            }
            // on root token
            if ( p->toks_cur == p->toks_beg ) {
                ++p->toks_cur;
            }
            __FJ__CONSTEXPR_IF( ParseMode ) {
                *toktype = FJ_TYPE_NUMBER;
            }
            break;
        }
        case '"': {
            auto ec = parse_string<ParseMode>(p, value, vlen);
            if ( ec != FJ_EC_OK ) {
                return ec;
            }
            // on root token
            if ( p->toks_cur == p->toks_beg ) {
                ++p->toks_cur;
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

    p->str_beg   = strbeg;
    p->str_cur   = strbeg;
    p->str_end   = strend;
    p->toks_beg  = toksbeg;
    p->toks_cur  = toksbeg;
    p->toks_end  = toksend;
    p->alloc_fn  = alloc_fn;
    p->free_fn   = free_fn;
    p->error     = FJ_EC_INVALID;
    p->dyn_parser= dyn_parser;
    p->dyn_tokens= dyn_tokens;
    p->ref_cnt   = 0;
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

template<std::size_t N>
inline parser make_parser(
     token *toksbeg
    ,token *toksend
    ,const char (&str)[N])
{
    return make_parser(toksbeg, toksend, str, &str[N]);
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
    if ( p ) {
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
    }

    return p;
}

template<std::size_t N>
inline parser* alloc_parser(
     token *toksbeg
    ,token *toksend
    ,const char (&str)[N]
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    return alloc_parser(toksbeg, toksend, str, &str[N], alloc_fn, free_fn);
}

inline std::size_t num_tokens(error_code *ecptr, const char *beg, const char *end) {
    static token fake;
    auto p = make_parser(&fake, &fake, beg, end);

    std::size_t vlen;
    token_type type;
    error_code ec = details::parse_value<false>(
         &p
        ,&(p.toks_beg->val)
        ,&vlen
        ,&type
        ,nullptr
    );
    (void)type;

    if ( !ec && p.str_cur+1 != p.str_end ) {
        p.str_cur = details::fj_skip_ws(p.str_cur);
        if ( p.str_cur != p.str_end ) {
            if ( ecptr ) { *ecptr = FJ_EC_INVALID; }

            return 0;
        }
    }

    std::size_t toknum = p.toks_cur - p.toks_beg;

    return toknum;
}

inline parser make_parser(
     const char *strbeg
    ,const char *strend
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

    if ( strend - strbeg == 1 && *strbeg == '\0' ) {
        return p;
    }

    error_code ec{};
    auto toknum = num_tokens(&ec, strbeg, strend);
    if ( ec ) {
        p.error = ec;
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

template<std::size_t N>
inline parser make_parser(
     const char (&str)[N]
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    return make_parser(str, &str[N], alloc_fn, free_fn);
}

inline parser* alloc_parser(
     const char *strbeg
    ,const char *strend
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    auto *p = static_cast<parser *>(alloc_fn(sizeof(parser)));
    if ( p ) {
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
    } else {
        return nullptr;
    }

    if ( strend - strbeg == 1 && *strbeg == '\0' ) {
        return p;
    }

    error_code ec{};
    auto toknum = num_tokens(&ec, strbeg, strend);
    if ( ec ) {
        p->error = ec;
        return p;
    }
    auto in_bytes = sizeof(token) * toknum;
    auto *toksbeg = static_cast<token *>(alloc_fn(in_bytes));
    auto *toksend = toksbeg + toknum;

    if ( p ) {
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
    }

    return p;
}

template<std::size_t N>
inline parser* alloc_parser(
     const char (&str)[N]
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    return alloc_parser(str, &str[N], alloc_fn, free_fn);
}

inline void free_parser(parser *p) {
    if ( p->dyn_tokens && p->toks_beg ) {
        p->free_fn(p->toks_beg);
    }

    p->toks_beg = nullptr;
    p->toks_cur = nullptr;
    p->toks_end = nullptr;
    p->error = FJ_EC_INVALID;

    if ( p->dyn_parser ) {
        p->free_fn(p);
    }
}

/*************************************************************************************************/
// parsing

inline std::size_t parse(parser *p) {
    if ( !p->toks_beg ) {
        return 0;
    }

    std::size_t vlen = 0;
    token_type type;
    p->error = details::parse_value<true>(
         p
        ,&(p->toks_beg->val)
        ,&vlen
        ,&type
        ,nullptr
    );
    p->toks_beg->type = type;
    assert(vlen <= std::numeric_limits<FJ_VLEN_TYPE>::max());
    p->toks_beg->vlen = static_cast<FJ_VLEN_TYPE>(vlen);
    p->toks_beg->end = p->toks_cur;
    p->toks_end = p->toks_cur;

    return p->toks_cur - p->toks_beg;
}

// returns the num of tokens
inline std::size_t parse(token *tokbeg, token *tokend, const char *strbeg, const char *strend) {
    auto p = make_parser(tokbeg, tokend, strbeg, strend);

    return parse(&p);
}

// returns the dyn-allocated parser
inline parser* parse(
     const char *strbeg
    ,const char *strend
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    auto *p = alloc_parser(strbeg, strend, alloc_fn, free_fn);
    parse(p);

    return p;
}

/*************************************************************************************************/
// parser state

inline bool is_valid(const parser *p) {
    return p && p->error == FJ_EC_OK;
}

inline error_code get_error(const parser *p) {
    return p->error;
}

inline const char* get_error_message(const parser *p) {
    return error_string(p->error);
}

inline std::size_t num_tokens(const parser *p) {
    return p->toks_end - p->toks_beg;
}

inline std::size_t num_childs(const parser *p) {
    return (!fj_is_simple_type_macro(p->toks_beg->type))
       ? p->toks_beg->childs - 1
       : static_cast<std::size_t>(p->toks_beg->type != FJ_TYPE_INVALID)
    ;
}

inline bool is_empty(const parser *p) {
    return p == nullptr || p->toks_beg == p->toks_end;
}

inline bool is_array(const parser *p)
{ return p->toks_beg->type == FJ_TYPE_ARRAY; }

inline bool is_object(const parser *p)
{ return p->toks_beg->type == FJ_TYPE_OBJECT; }

inline bool is_null(const parser *p)
{ return p->toks_beg->type == FJ_TYPE_NULL; }

inline bool is_bool(const parser *p)
{ return p->toks_beg->type == FJ_TYPE_BOOL; }

inline bool is_number(const parser *p)
{ return p->toks_beg->type == FJ_TYPE_NUMBER; }

inline bool is_string(const parser *p)
{ return p->toks_beg->type == FJ_TYPE_STRING; }

inline bool is_simple_type(const parser *p)
{ return fj_is_simple_type_macro(p->toks_beg->type); }

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
//    const token* end() const { return cur->end; }

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

namespace details {

// dump using iterator
inline void dump_tokens(std::FILE *stream, const char *caption, const iterator &it, std::size_t indent = 3) {
    std::fprintf(stream, "%s:\n", caption);
    dump_tokens_impl(stream, it.beg, it.cur, it.end, indent);
}

} // ns details

inline iterator iter_begin(const parser *p) {
    assert(p && p->toks_beg);
    return {p->toks_beg, p->toks_beg, p->toks_end-1};
}

inline iterator iter_end(const parser *p) {
    assert(p && p->toks_beg);
    return {p->toks_end-1, p->toks_end-1, p->toks_end-1};
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
        ;

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
    if ( !beg.cur ) {
        return end;
    }
    if ( beg.cur && beg.cur->parent && beg.cur->parent->type != FJ_TYPE_OBJECT ) {
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
    assert(p && p->toks_beg);

    bool is_simple = fj_is_simple_type_macro(p->toks_beg->type);
    iterator beg = {
         p->toks_beg
        ,p->toks_beg + static_cast<std::size_t>(!is_simple)
        ,p->toks_end
    };
    iterator end = iter_end(p);

    return details::iter_find(key, klen, beg, end);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
iterator iter_at(T key, const parser *p) {
    assert(p && p->toks_beg);

    bool is_simple = fj_is_simple_type_macro(p->toks_beg->type);
    iterator beg = {
         p->toks_beg
        ,p->toks_beg + static_cast<std::size_t>(!is_simple)
        ,p->toks_end
    };
    iterator end = iter_end(p);

    return details::iter_find(key, std::strlen(key), beg, end);
}

template<std::size_t N>
iterator iter_at(const char (&key)[N], const parser *p) {
    assert(p && p->toks_beg);

    bool is_simple = fj_is_simple_type_macro(p->toks_beg->type);
    iterator beg = {
         p->toks_beg
        ,p->toks_beg + static_cast<std::size_t>(!is_simple)
        ,p->toks_end
    };
    iterator end = iter_end(p);

    return details::iter_find(key, N-1, beg, end);
}

// at by key name, from the iterators
inline iterator iter_at(const char *key, std::size_t klen, const iterator &it) {
    assert(it.cur != it.end);

    iterator beg = {
         it.beg
        ,it.cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.end
    };
    auto end = iter_end(it);

    return details::iter_find(key, klen, beg, end);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
iterator iter_at(T key, const iterator &it) {
    assert(it.cur != it.end);

    iterator beg = {
         it.beg
        ,it.cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.end
    };
    auto end = iter_end(it);

    return details::iter_find(key, std::strlen(key), beg, end);
}

template<std::size_t N>
iterator iter_at(const char (&key)[N], const iterator &it) {
    assert(it.cur != it.end);

    iterator beg = {
         it.beg
        ,it.cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.end
    };
    auto end = iter_end(it);

    return details::iter_find(key, N-1, beg, end);
}

namespace details {

// find by index
inline iterator iter_find(std::size_t idx, const iterator &beg, const iterator &end) {
    if ( !beg.cur ) {
        return end;
    }
    if ( beg.cur && beg.cur->parent->type != FJ_TYPE_ARRAY ) {
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
    assert(p && p->toks_beg);

    bool is_simple = fj_is_simple_type_macro(p->toks_beg->type);
    iterator beg = {
         p->toks_beg
        ,p->toks_beg + static_cast<std::size_t>(!is_simple)
        ,p->toks_end
    };
    iterator end = iter_end(p);

    return details::iter_find(idx, beg, end);
}

// at by index, from a iterator
inline iterator iter_at(std::size_t idx, const iterator &it) {
    assert(it.cur != it.end);

    iterator beg = {
         it.beg
        ,it.cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.end
    };
    iterator end = iter_end(it);

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
        if ( it != toksbeg ) {
            token_type ctype = it->type;
            token_type ptype = (it-1)->type;
            if ( (ctype != FJ_TYPE_ARRAY_END && ctype != FJ_TYPE_OBJECT_END ) &&
                 (ptype != FJ_TYPE_OBJECT && ptype != FJ_TYPE_ARRAY) )
            {
                if ( !CalcLength ) {
                    if ( WithIndentation ) {
                        __FJ_IO_CALL_CB_WITH_CHECK_1(",\n", 2);
                    } else {
                        __FJ_IO_CALL_CB_WITH_CHECK_1(",", 1);
                    }
                }
                length += 1;
                if ( WithIndentation ) {
                    length += 1;
                }
            }
        }

        switch ( it->type ) {
            case FJ_TYPE_OBJECT: {
                if ( it->key ) {
                    if ( !CalcLength ) {
                        if ( WithIndentation ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_1(indent_str, indent_scope);
                        }
                        __FJ_IO_CALL_CB_WITH_CHECK_3("\"", 1, it->key, it->klen, "\":", 2);
                    }
                    if ( WithIndentation ) {
                        length += indent_scope;
                    }
                    length += 1 + it->klen + 2;
                }
                if ( !CalcLength ) {
                    if ( WithIndentation ) {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("{\n", 2);
                    } else {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("{", 1);
                    }
                }
                length += 1;
                if ( WithIndentation ) {
                    length += 1;
                    indent_scope += indent;
                }
                break;
            }
            case FJ_TYPE_OBJECT_END: {
                if ( !CalcLength ) {
                    if ( WithIndentation ) {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("\n", 1);
                        __FJ_IO_CALL_CB_WITH_CHECK_1(indent_str, indent_scope - indent);
                    }
                    __FJ_IO_CALL_CB_WITH_CHECK_1("}", 1);
                }
                length += 1; // for '}'
                if ( WithIndentation ) {
                    length += 1; // for '\n'
                    indent_scope -= indent;
                    length += indent_scope;
                }
                break;
            }
            case FJ_TYPE_ARRAY: {
                if ( it->key ) {
                    if ( !CalcLength ) {
                        if ( WithIndentation ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_1(indent_str, indent_scope);
                        }
                        __FJ_IO_CALL_CB_WITH_CHECK_3("\"", 1, it->key, it->klen, "\":", 2);
                    }
                    length += 1;
                    length += it->klen;
                    length += 2;
                    if ( WithIndentation ) {
                        length += indent_scope;
                    }
                }
                if ( !CalcLength ) {
                    if ( WithIndentation ) {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("[\n", 2);
                    } else {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("[", 1);
                    }
                }
                length += 1;
                if ( WithIndentation ) {
                    length += 1;
                    indent_scope += indent;
                }
                break;
            }
            case FJ_TYPE_ARRAY_END: {
                if ( !CalcLength ) {
                    if ( WithIndentation ) {
                        __FJ_IO_CALL_CB_WITH_CHECK_3("\n", 1, indent_str, indent_scope - indent, "]", 1);
                    } else {
                        __FJ_IO_CALL_CB_WITH_CHECK_1("]", 1);
                    }
                }
                length += 1; // for ']'
                if ( WithIndentation ) {
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
                    if ( !CalcLength ) {
                        if ( WithIndentation ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_4(indent_str, indent_scope, "\"", 1, it->key, it->klen, "\":", 2);
                        } else {
                            __FJ_IO_CALL_CB_WITH_CHECK_3("\"", 1, it->key, it->klen, "\":", 2);
                        }
                    }
                    length += 1 + it->klen + 2;
                    if ( WithIndentation ) {
                        length += indent_scope;
                    }
                } else if ( it->parent->type == FJ_TYPE_ARRAY ) {
                    if ( !CalcLength ) {
                        if ( WithIndentation ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_1(indent_str, indent_scope);
                        }
                    }
                    if ( WithIndentation ) {
                        length += indent_scope;
                    }
                }
                switch ( it->type ) {
                    case FJ_TYPE_NULL:
                    case FJ_TYPE_BOOL:
                    case FJ_TYPE_NUMBER: {
                        if ( !CalcLength ) {
                            __FJ_IO_CALL_CB_WITH_CHECK_1(it->val, it->vlen);
                        }
                        length += it->vlen;
                        break;
                    }
                    case FJ_TYPE_STRING: {
                        if ( !CalcLength ) {
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
             { return string_view{l->val, l->vlen} == string_view{r->val, r->vlen} ? compare_result::equal : compare_result::value; }
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
                    ? it.value() == found.value() ? compare_result::equal : compare_result::value
                    : cmpmode == compare_mode::length_only
                        ? it.value().size() == found.value().size() ? compare_result::equal : compare_result::length
                        : it.type() == found.type() ? compare_result::equal : compare_result::type
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
    auto ltokens = left_parser->toks_cur - left_parser->toks_beg;
    auto rtokens = right_parser->toks_cur - right_parser->toks_beg;
    if ( ltokens != rtokens ) {
        return (ltokens < rtokens)
            ? compare_result::longer
            : compare_result::shorter
        ;
    }

    if ( left_parser->toks_beg->type != right_parser->toks_beg->type ) {
        return compare_result::type;
    }

    auto left_beg  = iter_begin(left_parser);
    auto left_end  = iter_end  (left_parser);
    auto right_beg = iter_begin(right_parser);
    auto right_end = iter_end  (right_parser);
    if ( !left_beg.is_simple_type() ) {
        auto lchilds = left_parser->toks_beg->childs;
        auto rchilds = right_parser->toks_beg->childs;
        if ( lchilds != rchilds ) {
            return (lchilds < rchilds)
                ? compare_result::longer
                : compare_result::shorter
            ;
        }
    } else {
        return cmpr == compare_mode::full
            ? left_beg.value() == right_beg.value() ? compare_result::equal : compare_result::value
            : cmpr == compare_mode::length_only
                ? left_beg.value().size() == right_beg.value().size() ? compare_result::equal : compare_result::length
                : left_beg.type() == right_beg.type() ? compare_result::equal : compare_result::type
        ;
    }

    return left_beg.is_array()
        ? compare_impl(left_diff_ptr, right_diff_ptr, iter_next(left_beg), left_end, iter_next(right_beg), right_end, cmpr)
        : compare_impl(left_diff_ptr, right_diff_ptr ,iter_next(left_beg), left_end, iter_next(right_beg), right_end, cmpr)
    ;
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

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
                parser::inc_refcnt(m_parser);
            }
        }
        virtual ~intrusive_ptr() {
            if ( m_manage && m_parser ) {
                auto refcnt = parser::dec_refcnt(m_parser);
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
                parser::inc_refcnt(m_parser);
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
                parser::inc_refcnt(m_parser);
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

    // construct and parse using user-provided array of tokens and parser
    template<std::size_t N>
    fjson(
         parser *p
        ,token *toksbeg
        ,token *toksend
        ,const char (&str)[N]
    )
        :fjson{p, toksbeg, toksend, std::begin(str), std::end(str)}
    {}

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
    template<std::size_t N>
    fjson(
         token *toksbeg
        ,token *toksend
        ,const char (&str)[N]
    )
        :fjson{toksbeg, toksend, std::begin(str), std::end(str)}
    {}

    // construct and parse using user-provided parser and dyn-allocated tokens
    fjson(
         parser *p
        ,const char *strbeg
        ,const char *strend
        ,alloc_fnptr alloc_fn = &malloc
        ,free_fnptr free_fn = &free
    )
        :m_parser{
             true
            ,(*p = make_parser(strbeg, strend, alloc_fn, free_fn), p)
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

    // construct and parse using user-provided parser and dyn-allocated tokens
    template<std::size_t N>
    fjson(
         parser *p
        ,const char (&str)[N]
        ,alloc_fnptr alloc_fn = &malloc
        ,free_fnptr free_fn = &free
    )
        :fjson{p, std::begin(str), std::end(str), alloc_fn, free_fn}
    {}

    // construct and parse using dyn-allocated tokens and dyn-allocated parser
    fjson(
         const char *beg
        ,const char *end
        ,alloc_fnptr alloc_fn = &malloc
        ,free_fnptr free_fn = &free
    )
        :m_parser{true, alloc_parser(beg, end, alloc_fn, free_fn), free_parser}
        ,m_beg{}
        ,m_end{}
    {
        parse(m_parser.get());
        if ( flatjson::is_valid(m_parser.get()) ) {
            m_beg = iter_begin(m_parser.get());
            m_end = iter_end(m_parser.get());
        }
    }

    // construct and parse using dyn-allocated parser and dyn-allocated tokens
    template<std::size_t N>
    fjson(
         const char (&str)[N]
        ,alloc_fnptr alloc_fn = &malloc
        ,free_fnptr free_fn = &free
    )
        :fjson{std::begin(str), std::end(str), alloc_fn, free_fn}
    {}

    virtual ~fjson() = default;

private:
    fjson(intrusive_ptr p, iterator beg, iterator end)
        :m_parser{std::move(p)}
        ,m_beg{std::move(beg)}
        ,m_end{std::move(end)}
    {}

public:
    bool is_valid() const { return m_parser && flatjson::is_valid(m_parser.get()); }
    int error() const { return m_parser->error; }
    const char* error_string() const { return flatjson::error_string(m_parser->error); }

    // number of direct childs for OBJECT/ARRAY, or 1 for a valid SIMPLE type
    std::size_t size() const { return m_beg.members(); }
    std::size_t members() const { return m_beg.members(); }
    bool is_empty() const { return size() == 0; }

    // total number of tokens
    std::size_t tokens() const { return m_parser->toks_end - m_parser->toks_beg; }

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
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    return fjson{beg, end, alloc_fn, free_fn};
}

template<std::size_t N>
inline fjson pparse(
     const char (&str)[N]
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    return pparse(str, str + N-1, alloc_fn, free_fn);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
inline fjson pparse(
     T beg
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    const auto *end = beg + std::strlen(beg);

    return pparse(beg, end, alloc_fn, free_fn);
}

// user-provided parser and dyn tokens
inline fjson pparse(
     parser *p
    ,const char *beg
    ,const char *end
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    return fjson{p, beg, end, alloc_fn, free_fn};
}

template<std::size_t N>
inline fjson pparse(
     parser *p
    ,const char (&str)[N]
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    return pparse(p, str, str + N-1, alloc_fn, free_fn);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
inline fjson pparse(
     parser *p
    ,T beg
    ,alloc_fnptr alloc_fn = &malloc
    ,free_fnptr free_fn = &free)
{
    const auto *end = beg + std::strlen(beg);

    return pparse(p, beg, end, alloc_fn, free_fn);
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

template<std::size_t N>
inline fjson pparse(
     parser *p
    ,token *toksbeg
    ,token *toksend
    ,const char (&str)[N])
{
    return pparse(p, toksbeg, toksend, str, str + N-1);
}

template<typename T, typename = typename enable_if_const_char_ptr<T>::type>
inline fjson pparse(
     parser *p
    ,token *toksbeg
    ,token *toksend
    ,T beg)
{
    const auto *end = beg + std::strlen(beg);

    return pparse(p, toksbeg, toksend, beg, end);
}

} // ns flatjson

/*************************************************************************************************/

// undef internally used macro-vars
#undef __FJ__FALLTHROUGH
#undef __FJ__CHECK_OVERFLOW
#undef FJ_KLEN_TYPE
#undef FJ_VLEN_TYPE
#undef FJ_CHILDS_TYPE
#undef __FJ__CUR_CHAR

/*************************************************************************************************/

#endif // __FLATJSON__FLATJSON_HPP
