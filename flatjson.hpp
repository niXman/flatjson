
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2020 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__FLATJSON_HPP
#define __FLATJSON__FLATJSON_HPP

#include <limits>
#include <ostream>
#include <iterator>
#include <vector>
#include <string>
#if __cplusplus >= 201703L
#   include <string_view>
#endif // __cplusplus >= 201703L

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if defined(__clang__)
#   define __FLATJSON__FALLTHROUGH [[clang::fallthrough]]
#elif defined(__GNUC__)
#   define __FLATJSON__FALLTHROUGH __attribute__ ((fallthrough))
#elif defined(_MSC_VER)
#   define __FLATJSON__FALLTHROUGH
#else
#   error "Unknown compiler"
#endif //

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
    switch ( t ) {
        case FJ_TYPE_INVALID: return "INVALID";
        case FJ_TYPE_STRING: return "STRING";
        case FJ_TYPE_NUMBER: return "NUMBER";
        case FJ_TYPE_BOOL: return "BOOL";
        case FJ_TYPE_NULL: return "NULL";
        case FJ_TYPE_OBJECT: return "OBJECT";
        case FJ_TYPE_OBJECT_END: return "OBJECT_END";
        case FJ_TYPE_ARRAY: return "ARRAY";
        case FJ_TYPE_ARRAY_END: return "ARRAY_END";
    }

    return "UNKNOWN TYPE";
}

enum e_fj_error_code {
     FJ_OK = 0
    ,FJ_INVALID = -1
    ,FJ_INCOMPLETE = -2
    ,FJ_NO_FREE_TOKENS = -3
};

/*************************************************************************************************/

namespace details {

/*************************************************************************************************/

#define __FLATJSON__STRINGIZE_I(x) #x
#define __FLATJSON__STRINGIZE(x) __FLATJSON__STRINGIZE_I(x)

#define __FLATJSON__MAKE_ERROR_MESSAGE(msg)\
    __FILE__ "(" __FLATJSON__STRINGIZE(__LINE__) "): " msg

#ifdef __FLATJSON__DONT_CHECK_OVERFLOW
#   define __FLATJSON__CHECK_OVERFLOW(expr, type)
#else
#   define __FLATJSON__CHECK_OVERFLOW(expr, type) \
        if ( (expr) >= std::numeric_limits<type>::max() ) \
            throw std::overflow_error(__FLATJSON__MAKE_ERROR_MESSAGE("overflow detected!"))
#endif //__FLATJSON__SHOULD_CHECK_OVERFLOW

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

#pragma pack(push, 1)

struct fj_pair {
    fj_pair()
        :type{}
        ,k{}
        ,klen{}
        ,v{}
        ,vlen{}
        ,parent{}
        ,childs{}
        ,end{}
    {}

    e_fj_token_type type;
    const char *k;
    __FLATJSON__KLEN_TYPE klen;
    const char *v;
    __FLATJSON__VLEN_TYPE vlen;
    fj_pair *parent;
    __FLATJSON__CHILDS_TYPE childs;
    fj_pair *end; // pointing to the last token for arrays and objects
};

#pragma pack(pop)

/*************************************************************************************************/

#define __FLATJSON__IS_SIMPLE_TYPE(type) \
    (type == flatjson::FJ_TYPE_STRING || \
     type == flatjson::FJ_TYPE_NUMBER || \
     type == flatjson::FJ_TYPE_BOOL || \
     type == flatjson::FJ_TYPE_NULL)

#define __FLATJSON__IS_DIGIT(ch) \
    (ch >= '0' && ch <= '9')

#define __FLATJSON__IS_HEX_DIGIT(ch) \
    (__FLATJSON__IS_DIGIT(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))

#define __FLATJSON__IS_HEX_DIGIT4(ch0, ch1, ch2, ch3) \
    (__FLATJSON__IS_HEX_DIGIT(ch0) && \
     __FLATJSON__IS_HEX_DIGIT(ch1) && \
     __FLATJSON__IS_HEX_DIGIT(ch2) && \
     __FLATJSON__IS_HEX_DIGIT(ch3) \
    )

#define __FLATJSON__CUR_CHAR(p) \
    ((fj_skip_ws(p)), (p->js_cur >= p->js_end ? ((char)-1) : *(p->js_cur)))

#define __FLATJSON__UTF8_CHAR_LEN(ch) \
    ( \
        ((ch & 0x80u) == 0) \
            ? 1 \
            : ((ch & 0xf0u) == 0xf0) \
                ? 4 \
                : ((ch & 0xf0u) == 0xe0) \
                    ? 3 \
                    : 2 \
    )

/*************************************************************************************************/

struct fj_parser {
    const char *js_cur;
    const char *js_end;

    fj_pair *jstok_beg;
    fj_pair *jstok_cur;
    fj_pair *jstok_end;
};

/*************************************************************************************************/

inline void fj_skip_ws(fj_parser *p) {
    for (
        ;p->js_cur < p->js_end && (*p->js_cur == ' ' || *p->js_cur == '\t' || *p->js_cur == '\r' || *p->js_cur == '\n')
        ;++p->js_cur
    )
        ;
}

inline int fj_check_and_skip(fj_parser *p, char expected) {
    char ch = __FLATJSON__CUR_CHAR(p);
    if ( ch == expected ) {
        p->js_cur++;

        return FJ_OK;
    }

    return ch == ((char)-1) ? FJ_INCOMPLETE : FJ_INVALID;
}

inline int fj_escape_len(const char *s, std::ptrdiff_t len) {
    switch ( *s ) {
        case 'u':
            return len < 6
                ? FJ_INCOMPLETE
                : __FLATJSON__IS_HEX_DIGIT4(*(s+1), *(s+2), *(s+3), *(s+4))
                    ? 5
                    : FJ_INVALID
            ;
        case '"':
        case '\\':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
            return len < 2 ? FJ_INCOMPLETE : 1;
        default:
            return FJ_INVALID;
    }
}

/*************************************************************************************************/

template<bool RO, std::size_t ExLen>
inline int fj_expect(fj_parser *p, const char *s, const char **ptr, std::size_t *size) {
    if ( p->js_cur + ExLen > p->js_end )
        return FJ_INCOMPLETE;

    if ( std::memcmp(p->js_cur, s, ExLen) != 0 )
        return FJ_INVALID;

    if ( !RO ) {
        *ptr = p->js_cur;
        *size = ExLen;
    }

    p->js_cur += ExLen;

    return FJ_OK;
}

template<bool RO>
inline int fj_parse_string(fj_parser *p, const char **ptr, std::size_t *size) {
    int ec = fj_check_and_skip(p, '"');
    if ( ec ) return ec;

    int ch = 0;
    const char *start = p->js_cur;
    for ( std::ptrdiff_t len = 0; p->js_cur < p->js_end; p->js_cur += len ) {
        ch = *(unsigned char *)p->js_cur;
        len = __FLATJSON__UTF8_CHAR_LEN((unsigned char)ch);
        if ( !(ch >= 32 && len > 0) ) return FJ_INVALID;
        if ( len > (p->js_end - p->js_cur) ) return FJ_INCOMPLETE;

        if (ch == '\\') {
            int n = fj_escape_len(p->js_cur + 1, p->js_end - p->js_cur);
            if ( n <= 0 ) return n;
            len += n;
        } else if (ch == '"') {
            if ( !RO ) {
                *ptr = start;
                *size = p->js_cur - start;
            }

            ++p->js_cur;

            break;
        }
    }

    return ch == '"' ? FJ_OK : FJ_INCOMPLETE;
}

template<bool RO>
inline int fj_parse_number(fj_parser *p, const char **ptr, std::size_t *size) {
    const char *start = p->js_cur;
    if ( __FLATJSON__CUR_CHAR(p)== '-' ) p->js_cur++;

    if ( p->js_cur >= p->js_end ) return FJ_INCOMPLETE;
    if ( p->js_cur + 1 < p->js_end && *(p->js_cur) == '0' && *(p->js_cur+1) == 'x' ) {
        p->js_cur += 2;

        if ( p->js_cur >= p->js_end ) return FJ_INCOMPLETE;
        if ( !__FLATJSON__IS_HEX_DIGIT(*(p->js_cur)) ) return FJ_INVALID;

        for ( ; p->js_cur < p->js_end && __FLATJSON__IS_HEX_DIGIT(*(p->js_cur)); ++p->js_cur )
            ;
    } else {
        if ( !__FLATJSON__IS_DIGIT(*(p->js_cur)) ) return FJ_INVALID;
        for ( ; p->js_cur < p->js_end && __FLATJSON__IS_DIGIT(*(p->js_cur)); ++p->js_cur )
            ;

        if ( p->js_cur < p->js_end && *(p->js_cur) == '.' ) {
            p->js_cur++;

            if ( p->js_cur >= p->js_end ) return FJ_INCOMPLETE;
            if ( !__FLATJSON__IS_DIGIT(*(p->js_cur)) ) return FJ_INVALID;

            for ( ; p->js_cur < p->js_end && __FLATJSON__IS_DIGIT(*(p->js_cur)); ++p->js_cur )
                ;
        }
        if ( p->js_cur < p->js_end && (*(p->js_cur) == 'e' || *(p->js_cur) == 'E') ) {
            p->js_cur++;

            if ( p->js_cur >= p->js_end ) return FJ_INCOMPLETE;

            if ( *(p->js_cur) == '+' || *(p->js_cur) == '-' )
                p->js_cur++;

            if ( p->js_cur >= p->js_end ) return FJ_INCOMPLETE;
            if ( !__FLATJSON__IS_DIGIT(*(p->js_cur)) ) return FJ_INVALID;

            for ( ; p->js_cur < p->js_end && __FLATJSON__IS_DIGIT(p->js_cur[0]); ++p->js_cur )
                ;
        }
    }

    if ( (p->js_cur - start) > 1 && *start == '0') {
        return FJ_INVALID;
    }

    if ( !RO ) {
        *ptr = start;
        *size = p->js_cur - start;
    }

    return FJ_OK;
}

template<bool RO>
inline int fj_parse_value(fj_parser *p, const char **ptr, std::size_t *size, e_fj_token_type *toktype, fj_pair *parent);

template<bool RO>
inline int fj_parse_array(fj_parser *p, fj_pair *parent) {
    int ec = fj_check_and_skip(p, '[');
    if ( ec ) return ec;

    fj_pair *startarr = p->jstok_cur++;
    if ( !RO ) {
        startarr->type = FJ_TYPE_ARRAY;
        startarr->parent = parent;
        if ( startarr->parent ) {
            __FLATJSON__CHECK_OVERFLOW(startarr->parent->childs, __FLATJSON__CHILDS_TYPE);
            ++startarr->parent->childs;
        }
    }

    while ( __FLATJSON__CUR_CHAR(p) != ']' ) {
        if ( !RO && p->jstok_cur == p->jstok_end ) return FJ_NO_FREE_TOKENS;
        fj_pair *pair = p->jstok_cur++;

        char ch = __FLATJSON__CUR_CHAR(p);
        if ( ch == '{' ) {
            p->jstok_cur -= 1;
        } else {
            if ( !RO ) {
                pair->parent = startarr;

                __FLATJSON__CHECK_OVERFLOW(startarr->childs, __FLATJSON__CHILDS_TYPE);
                ++startarr->childs;
            }
        }

        std::size_t size = 0;
        ec = fj_parse_value<RO>(p, &(pair->v), &size, &(pair->type), startarr);
        if ( ec ) return ec;
        __FLATJSON__CHECK_OVERFLOW(size, __FLATJSON__VLEN_TYPE);
        if ( !RO ) pair->vlen = size;

        if (__FLATJSON__CUR_CHAR(p) == ',') {
            p->js_cur++;
            if ( *(p->js_cur) == ']' ) {
                return FJ_INVALID;
            }
        }
    }

    ec = fj_check_and_skip(p, ']');
    if ( ec ) return ec;

    if ( !RO ) {
        if ( p->jstok_cur == p->jstok_end ) return FJ_NO_FREE_TOKENS;
        fj_pair *endarr = p->jstok_cur++;
        endarr->type = FJ_TYPE_ARRAY_END;
        endarr->parent = startarr;
        __FLATJSON__CHECK_OVERFLOW(endarr->parent->childs, __FLATJSON__CHILDS_TYPE);
        ++endarr->parent->childs;
        startarr->end = endarr;
    } else {
        ++p->jstok_cur;
    }

    return 0;
}

template<bool RO>
inline int fj_parse_object(fj_parser *p, fj_pair *parent) {
    int ec = fj_check_and_skip(p, '{');
    if ( ec ) return ec;

    if ( !RO && p->jstok_cur == p->jstok_end) return FJ_NO_FREE_TOKENS;
    fj_pair *startobj = p->jstok_cur++;
    if ( !RO ) {
        startobj->type = FJ_TYPE_OBJECT;
        startobj->parent = parent;
        if ( startobj->parent ) {
            __FLATJSON__CHECK_OVERFLOW(startobj->parent->childs, __FLATJSON__CHILDS_TYPE);
            ++startobj->parent->childs;
        }
    }

    while ( __FLATJSON__CUR_CHAR(p) != '}' ) {
        char ch = __FLATJSON__CUR_CHAR(p);
        if ( ch != '"' ) return ch == ((char)-1) ? FJ_INCOMPLETE : FJ_INVALID;

        if ( !RO && p->jstok_cur == p->jstok_end ) return FJ_NO_FREE_TOKENS;
        fj_pair *pair = p->jstok_cur++;

        std::size_t size = 0;
        ec = fj_parse_value<RO>(p, &(pair->k), &size, &(pair->type), startobj);
        if ( ec ) return ec;
        __FLATJSON__CHECK_OVERFLOW(size, __FLATJSON__KLEN_TYPE);
        if ( !RO ) pair->klen = size;

        ec = fj_check_and_skip(p, ':');
        if ( ec ) return ec;

        ch = __FLATJSON__CUR_CHAR(p);
        if ( ch == '[' || ch == '{' ) {
            p->jstok_cur -= 1;
            const char *unused_str = nullptr;
            std::size_t unused_size{};
            ec = fj_parse_value<RO>(p, &unused_str, &unused_size, &(pair->type), startobj);
        } else {
            if ( !RO ) {
                pair->parent = startobj;
                __FLATJSON__CHECK_OVERFLOW(startobj->childs, __FLATJSON__CHILDS_TYPE);
                ++startobj->childs;
            }

            ec = fj_parse_value<RO>(p, &(pair->v), &size, &(pair->type), startobj);
            __FLATJSON__CHECK_OVERFLOW(size, __FLATJSON__VLEN_TYPE);
            if ( !RO ) pair->vlen = size;
        }

        if ( ec ) return ec;

        if ( __FLATJSON__CUR_CHAR(p) == ',' ) {
            p->js_cur++;
            if ( *(p->js_cur) == '}' ) {
                return FJ_INVALID;
            }
        }
// don't remove the code below!
//        if ( p->jstok_end && startobj->childs > 1 ) {
//            std::size_t num = startobj->childs-1;
//            fj_pair *beg = startobj+1;
//            fj_pair *cur = startobj+1+num;
//
//            fj_pair *it = nullptr;
//            while ( num > 0 ) {
//                it = beg;
//                std::size_t step = num / 2;
//                it += step;
//                std::size_t minlen = std::min(it->klen, cur->klen);
//                if ( std::strncmp(it->k, cur->k, minlen) < 0 ) { // strNcmp?
//                    beg = ++it;
//                    num -= step + 1;
//                } else {
//                    num = step;
//                }
//            }
////            printf("k=%.*s\n", beg->klen, beg->k);
//            fj_pair tmp = *cur;
//            std::memmove(cur, beg, sizeof(tmp));
//            *beg = tmp;
//        }
    }

    ec = fj_check_and_skip(p, '}');
    if ( ec ) return ec;

    if ( !RO ) {
        if ( p->jstok_cur == p->jstok_end ) return FJ_NO_FREE_TOKENS;
        fj_pair *endobj = p->jstok_cur++;
        endobj->type = FJ_TYPE_OBJECT_END;
        endobj->parent = startobj;
        __FLATJSON__CHECK_OVERFLOW(endobj->parent->childs, __FLATJSON__CHILDS_TYPE);
        ++endobj->parent->childs;
        startobj->end = endobj;
    } else {
        ++p->jstok_cur;
    }

    return FJ_OK;
}

template<bool RO>
inline int fj_parse_value(fj_parser *p, const char **ptr, std::size_t *size, e_fj_token_type *toktype, fj_pair *parent) {
    char ch = __FLATJSON__CUR_CHAR(p);
    switch ( ch ) {
        case '{': {
            int ec = fj_parse_object<RO>(p, parent);
            if ( ec ) return ec;
            if ( !RO ) *toktype = FJ_TYPE_OBJECT;
            break;
        }
        case '[': {
            int ec = fj_parse_array<RO>(p, parent);
            if ( ec ) return ec;
            if ( !RO ) *toktype = FJ_TYPE_ARRAY;
            break;
        }
        case 'n': {
            int ec = fj_expect<RO, 4>(p, "null", ptr, size);
            if ( ec ) return ec;
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            if ( !RO ) *toktype = FJ_TYPE_NULL;
            break;
        }
        case 't': {
            int ec = fj_expect<RO, 4>(p, "true", ptr, size);
            if ( ec ) return ec;
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            if ( !RO ) *toktype = FJ_TYPE_BOOL;
            break;
        }
        case 'f': {
            int ec = fj_expect<RO, 5>(p, "false", ptr, size);
            if ( ec ) return ec;
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            if ( !RO ) *toktype = FJ_TYPE_BOOL;
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
            int ec = fj_parse_number<RO>(p, ptr, size);
            if ( ec ) return ec;
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            if ( !RO ) *toktype = FJ_TYPE_NUMBER;
            break;
        }
        case '"': {
            int ec = fj_parse_string<RO>(p, ptr, size);
            if ( ec ) return ec;
            // on root token
            if ( p->jstok_cur == p->jstok_beg ) {
                ++p->jstok_cur;
            }
            if ( !RO ) *toktype = FJ_TYPE_STRING;
            break;
        }
        default:
            return ch == ((char)-1) ? FJ_INCOMPLETE : FJ_INVALID;
    }

    return FJ_OK;
}

struct parse_result {
    e_fj_error_code ec;
    std::size_t toknum;
};

inline void fj_init(fj_parser *parser, const char *jsstr, std::size_t jslen, fj_pair *tokbuf, std::size_t toksize) {
    // root token
    tokbuf[0].type = FJ_TYPE_INVALID;
    tokbuf[0].k = nullptr;
    tokbuf[0].klen = 0;
    tokbuf[0].v = nullptr;
    tokbuf[0].vlen = 0;
    tokbuf[0].parent = nullptr;
    tokbuf[0].childs = 0;

    *parser = {jsstr, jsstr + jslen, tokbuf, tokbuf, tokbuf+toksize};
}

inline parse_result fj_parse(fj_parser *parser) {
    parse_result res{};

    std::size_t vlen = 0;
    res.ec = static_cast<e_fj_error_code>(fj_parse_value<false>(parser, &(parser->jstok_beg->v), &vlen, &(parser->jstok_beg->type), nullptr));
    __FLATJSON__CHECK_OVERFLOW(vlen, __FLATJSON__VLEN_TYPE);
    parser->jstok_beg->vlen = static_cast<__FLATJSON__VLEN_TYPE>(vlen);

    if ( res.ec ) {
        res.toknum = 0;
    }

    res.toknum = parser->jstok_cur - parser->jstok_beg;

    return res;
}

inline parse_result fj_num_tokens(const char *jsstr, std::size_t jslen) {
    parse_result res{};
    fj_parser parser{};
    parser.js_cur = jsstr;
    parser.js_end = jsstr + jslen;

    static fj_pair fake;
    parser.jstok_beg = &fake;
    parser.jstok_cur = &fake;
    parser.jstok_end = nullptr;

    std::size_t vlen = 0;
    e_fj_token_type toktype{};
    res.ec = static_cast<e_fj_error_code>(fj_parse_value<true>(&parser, &(parser.jstok_beg->v), &vlen, &toktype, nullptr));
    if ( !res.ec && parser.js_cur != parser.js_end ) {
        fj_skip_ws(&parser);
        if ( parser.js_cur != parser.js_end ) {
            res.ec = FJ_INVALID;
        }
    }

    __FLATJSON__CHECK_OVERFLOW(vlen, __FLATJSON__VLEN_TYPE);
    parser.jstok_beg->vlen = static_cast<__FLATJSON__VLEN_TYPE>(vlen);

    res.toknum = parser.jstok_cur - parser.jstok_beg;

    return res;
}

inline void fj_dump_tokens(std::FILE *stream, fj_pair *toks, std::size_t num) {
    for ( auto *it = toks; it != toks+num; ++it ) {
        std::fprintf(stream, "%2d: type=%12s, parent=%p, addr=%p, end=%p, childs=%d, key=%.*s, val=%.*s\n"
            ,(int)(it-toks)
            ,fj_token_type_name(it->type)
            ,it->parent
            ,it
            ,it->end
            ,it->childs
            ,(int)it->klen, it->k
            ,(int)it->vlen, it->v
        );
        std::fflush(stream);
    }
}

using fj_gather_cb_t = void(*)(void *userdata, const char *ptr, std::size_t len);
template<bool CalcLength = false>
inline std::size_t fj_get_tokens(const fj_pair *toks, std::size_t num, std::size_t indent, void *userdata, fj_gather_cb_t cb) {
    static const char indent_str[] = "                                                                                ";
    std::size_t indent_scope = 0;
    std::size_t length = 0;
    for ( auto *it = toks; it != toks+num; ++it ) {
        if ( it != toks ) {
            e_fj_token_type ctype = it->type;
            e_fj_token_type ptype = (it-1)->type;
            if ( (ctype != FJ_TYPE_ARRAY_END && ctype != FJ_TYPE_OBJECT_END ) &&
                 (ptype != FJ_TYPE_OBJECT && ptype != FJ_TYPE_ARRAY) )
            {
                if ( !CalcLength ) {
                    cb(userdata, ",", 1);
                    if ( indent ) cb(userdata, "\n", 1);
                }
                length += 1;
                if ( indent ) length += 1;
            }
        }

        switch ( it->type ) {
            case FJ_TYPE_OBJECT: {
                if ( it->k ) {
                    if ( !CalcLength ) {
                        if ( indent ) cb(userdata, indent_str, indent_scope);
                        cb(userdata, "\"", 1);
                        cb(userdata, it->k, it->klen);
                        cb(userdata, "\":", 2);
                    }
                    length += 1+2;
                    length += it->klen;
                    if ( indent ) length += indent_scope;
                }
                if ( !CalcLength ) {
                    cb(userdata, "{", 1);
                    if ( indent ) cb(userdata, "\n", 1);
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
                if ( it->k ) {
                    if ( !CalcLength ) {
                        if ( indent ) cb(userdata, indent_str, indent_scope);
                        cb(userdata, "\"", 1);
                        cb(userdata, it->k, it->klen);
                        cb(userdata, "\":", 2);
                    }
                    length += 1+2;
                    length += it->klen;
                    if ( indent ) length += indent_scope;
                }
                if ( !CalcLength ) {
                    cb(userdata, "[", 1);
                    if ( indent ) cb(userdata, "\n", 1);
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
                if ( it->parent->type != FJ_TYPE_ARRAY ) {
                    if ( !CalcLength ) {
                        if ( indent ) cb(userdata, indent_str, indent_scope);
                        cb(userdata, "\"", 1);
                        cb(userdata, it->k, it->klen);
                        cb(userdata, "\":", 2);
                    }
                    length += 1+2;
                    length += it->klen;
                    if ( indent ) length += indent_scope;
                } else if ( it->parent->type == FJ_TYPE_ARRAY ) {
                    if ( !CalcLength ) {
                        if ( indent ) cb(userdata, indent_str, indent_scope);
                    }
                    if ( indent ) length += indent_scope;
                }
                switch ( it->type ) {
                    case FJ_TYPE_NULL:
                    case FJ_TYPE_BOOL:
                    case FJ_TYPE_NUMBER: {
                        if ( !CalcLength ) cb(userdata, it->v, it->vlen);
                        length += it->vlen;
                        break;
                    }
                    case FJ_TYPE_STRING: {
                        if ( !CalcLength ) {
                            cb(userdata, "\"", 1);
                            cb(userdata, it->v, it->vlen);
                            cb(userdata, "\"", 1);
                        }
                        length += 2;
                        length += it->vlen;
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

static void tokens_to_stream_cb_0(void *userdata, const char *ptr, std::size_t len) {
    std::FILE *stream = static_cast<std::FILE*>(userdata);
    std::fwrite(ptr, len, 1, stream);
}

inline std::size_t fj_tokens_to_stream(std::FILE *stream, const fj_pair *toks, std::size_t num, std::size_t indent = 0) {
    return fj_get_tokens(toks, num, indent, stream, tokens_to_stream_cb_0);
}

/*************************************************************************************************/

static void tokens_to_stream_cb_1(void *userdata, const char *ptr, std::size_t len) {
    std::ostream *stream = static_cast<std::ostream *>(userdata);
    stream->write(ptr, len);
}

inline std::size_t fj_tokens_to_stream(std::ostream &stream, const fj_pair *toks, std::size_t num, std::size_t indent = 0) {
    return fj_get_tokens(toks, num, indent, &stream, tokens_to_stream_cb_1);
}

/*************************************************************************************************/

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

inline std::size_t fj_tokens_to_buf(const fj_pair *toks, std::size_t num, char *buf, std::size_t size, std::size_t indent = 0) {
    tokens_to_buf_userdata userdata{buf, buf+size};
    return fj_get_tokens(toks, num, indent, &userdata, tokens_to_buf_cb);
}

/*************************************************************************************************/

inline std::size_t fj_str_length(const fj_pair *toks, std::size_t num, std::size_t indent = 0) {
    return fj_get_tokens<true>(toks, num, indent, nullptr, nullptr);
}

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

    std::size_t size() const { return m_len; }
    bool empty() const { return size() == 0; }
    const char* data() const { return m_ptr; }

    friend bool operator==(const static_string &l, const char *r) { return std::strncmp(l.m_ptr, r, l.m_len) == 0; }
    friend bool operator==(const static_string &l, const static_string &r) { return std::strncmp(l.m_ptr, r.m_ptr, l.m_len) == 0; }
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

/*************************************************************************************************/

#if __cplusplus < 201703L
    using static_string = details::static_string;
#else
    using static_string = std::string_view;
#endif

/*************************************************************************************************/

namespace details {

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
        return -conv_to < typename std::make_unsigned<To>::type > (ptr, len - 1);
    }

    return conv_to < typename std::make_unsigned<To>::type > (ptr, len);
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

/*************************************************************************************************/

template<typename JS, typename Storage>
struct fjson_ro_base {
    fjson_ro_base(const Storage &storage)
        :m_storage{storage}
    {}
    virtual ~fjson_ro_base() = default;

    std::size_t size() const {
        const auto *beg = m_storage.begin();
        return (!__FLATJSON__IS_SIMPLE_TYPE(type()))
            ? beg->childs-1
            : static_cast<std::size_t>(beg->type != FJ_TYPE_INVALID)
        ;
    }
    bool empty() const { return size() == 0; }

    e_fj_token_type type() const { return m_storage.begin()->type; }
    const char* type_name() const { return fj_token_type_name(type()); }

    bool is_array() const { return type() == FJ_TYPE_ARRAY; }
    bool is_object() const { return type() == FJ_TYPE_OBJECT; }
    bool is_null() const { return type() == FJ_TYPE_NULL; }
    bool is_bool() const { return type() == FJ_TYPE_BOOL; }
    bool is_number() const { return type() == FJ_TYPE_NUMBER; }
    bool is_string() const { return type() == FJ_TYPE_STRING; }

    template<std::size_t KL>
    bool contains(const char (&key)[KL]) const { return contains(key, KL-1); }
    template<
         typename ConstCharPtr
        ,typename = typename std::enable_if<
            std::is_same<ConstCharPtr, const char*>::value
        >::type
    >
    bool contains(ConstCharPtr key) { return contains(key, std::strlen(key)); }
    bool contains(const char *key, std::size_t len) const {
        auto res = find(key, len);
        return res.first != nullptr;
    }

    // for objects
    template<std::size_t KL>
    JS at(const char (&key)[KL]) const { return at(key, KL-1); }
    template<
         typename ConstCharPtr
        ,typename = typename std::enable_if<
            std::is_same<ConstCharPtr, const char*>::value
        >::type
    >
    JS at(ConstCharPtr key) const { return at(key, std::strlen(key)); }
    JS at(const char *key, std::size_t len) const {
        auto res = find(key, len);
        if ( !res.first )
            throw std::runtime_error(__FLATJSON__MAKE_ERROR_MESSAGE("key not found"));

        return {res.first, res.second};
    }
    // for arrays
    JS at(std::size_t idx) const {
        auto res = find(idx);
        if ( !res.first )
            throw std::out_of_range(__FLATJSON__MAKE_ERROR_MESSAGE("out of range"));

        return {res.first, res.second};
    }

    static_string to_sstring() const {
        const auto *beg = m_storage.begin();
        if ( __FLATJSON__IS_SIMPLE_TYPE(type()) || type() == FJ_TYPE_NULL ) {
            if ( !is_null() ) {
                return {beg->v, beg->vlen};
            }

            return {};
        }

        throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("not STRING/NUMBER/BOOL/NULL type"));
    }
    std::string to_string() const { auto s = to_sstring(); return {s.data(), s.size()}; }
    template<typename T>
    T to() const { auto s = to_sstring(); return details::conv_to<T>(s.data(), s.size()); }
    bool to_bool() const { return to<bool>(); }
    std::uint64_t to_uint() const { return to<std::uint64_t>(); }
    std::int64_t to_int() const { return to<std::int64_t>(); }
    double to_double() const { return to<double>(); }
    float to_float() const { return to<float>(); }

    std::pair<const details::fj_pair *, std::size_t>
    find(const char *key, std::size_t len) const {
        if ( type() != FJ_TYPE_OBJECT )
            throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("not OBJECT type"));

        const details::fj_pair *parent = m_storage.begin();
        const details::fj_pair *beg = parent+1;
        const details::fj_pair *end = parent->end;
        while ( beg != end ) {
            if ( beg->type == FJ_TYPE_OBJECT_END ) {
                return {nullptr, 0};
            }
            if ( std::strncmp(beg->k, key, len) == 0 ) {
                break;
            }

            beg = __FLATJSON__IS_SIMPLE_TYPE(beg->type) ? beg+1 : beg->end+1;
        }

        auto type = beg->type;
        if ( __FLATJSON__IS_SIMPLE_TYPE(type) ) {
            return {beg, 1};
        } else if ( type == FJ_TYPE_OBJECT || type == FJ_TYPE_ARRAY ) {
            return {beg, static_cast<__FLATJSON__CHILDS_TYPE>(beg->childs+1)};
        }

        throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("unreachable!"));
    }
    std::pair<const details::fj_pair *, std::size_t>
    find(std::size_t idx) const {
        if ( type() != FJ_TYPE_ARRAY )
            throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("not ARRAY type"));

        if ( idx >= static_cast<__FLATJSON__CHILDS_TYPE>(m_storage.begin()->childs - 1) ) // one for END token
            return {nullptr, 0};

        const details::fj_pair *parent = m_storage.begin();
        const details::fj_pair *beg = parent+1;
        const details::fj_pair *end = parent->end;
        for ( ; beg != end && idx; --idx ) {
            beg = __FLATJSON__IS_SIMPLE_TYPE(beg->type) ? beg+1 : beg->end+1;
        }

        auto type = beg->type;
        if ( __FLATJSON__IS_SIMPLE_TYPE(type) ) {
            return {beg, 1};
        } else if ( type == FJ_TYPE_OBJECT || type == FJ_TYPE_ARRAY ) {
            return {beg, static_cast<__FLATJSON__CHILDS_TYPE>(beg->childs+1)};
        }

        throw std::logic_error(__FLATJSON__MAKE_ERROR_MESSAGE("unreachable!"));
    }

private:
    const Storage &m_storage;
};

/*************************************************************************************************/

struct fjson_rw_base {
};

/*************************************************************************************************/

template<std::size_t N = 32>
struct fjson_static_storage {
    virtual ~fjson_static_storage() = default;

    const fj_pair* begin() const { return &m_tokens[0]; }
    const fj_pair* end() const { return &m_tokens[0] + N; }
    fj_pair* begin() { return &m_tokens[0]; }
    fj_pair* end() { return &m_tokens[0] + N; }

    std::size_t size_() const { return N; }
    void resize_(std::size_t /*new_size*/) { assert(!"unimplemented"); }
    fj_pair* insert(const fj_pair */*after*/) { assert(!"unimplemented"); }
    void assign_(const fj_pair */*ptr*/, std::size_t /*num*/) { assert(!"unimplemented"); }

private:
    fj_pair m_tokens[N];
};

/*************************************************************************************************/

struct fjson_dynamic_storage {
    virtual ~fjson_dynamic_storage() = default;

    const fj_pair* begin() const { return &(*m_tokens.begin()); }
    const fj_pair* end() const { return &(*m_tokens.end()); }
    fj_pair* begin() { return &(*m_tokens.begin()); }
    fj_pair* end() { return &(*m_tokens.end()); }

    std::size_t size_() const { return m_tokens.size(); }
    void resize_(std::size_t new_size) { m_tokens.resize(new_size); }
    fj_pair* insert(const fj_pair *after) {
        auto it = m_tokens.begin() + std::distance(static_cast<const fj_pair *>(m_tokens.data()), after);
        static const fj_pair tmp{};
        it = m_tokens.insert(std::next(it), tmp);
        return &(*it);
    }
    void assign_(const fj_pair *ptr, std::size_t num) { m_tokens.assign(ptr, ptr+num); }

private:
    std::vector<fj_pair> m_tokens;
};

/*************************************************************************************************/

} // ns details

/*************************************************************************************************/

template<std::size_t N = 32, bool CheckNumTokens = true>
struct fjson
    :private details::fjson_static_storage<N>
    ,public details::fjson_ro_base<fjson<N>, details::fjson_static_storage<N>>
{
private:
    using storage_type = details::fjson_static_storage<N>;
    using ro_base_type = details::fjson_ro_base<fjson<N>, storage_type>;

    friend ro_base_type;
public:
    fjson()
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_used{0}
        ,m_err{}
    {}
    template<std::size_t L>
    explicit fjson(const char (&str)[L])
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_used{0}
        ,m_err{}
    { load(str, L-1); }
    template<
         typename ConstCharPtr
        ,typename = typename std::enable_if<
            std::is_same<ConstCharPtr, const char*>::value
        >::type
    >
    explicit fjson(ConstCharPtr str)
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_used{0}
        ,m_err{}
    { load(str, std::strlen(str)); }
    fjson(const char *ptr, std::size_t size)
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_used{0}
        ,m_err{}
    { load(ptr, size); }

    virtual ~fjson() = default;

private:
    fjson(const details::fj_pair *tokens, std::size_t num)
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_used{num}
        ,m_err{}
    { std::memcpy(storage_type::begin(), tokens, sizeof(details::fj_pair) * num); }
    
public:

    bool valid() const { return m_used != 0; }
    e_fj_error_code error() const { return m_err; }

    // for arrays
    fjson operator[](std::size_t idx) const { return ro_base_type::at(idx);}

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

    bool load(const char *ptr, std::size_t size) {
        if ( CheckNumTokens ) {
            auto res = details::fj_num_tokens(ptr, size);
            if ( res.ec ) {
                m_used = 0;
                m_err = res.ec;
            } else {
                if ( res.toknum > N ) {
                    m_used = 0;
                    m_err = FJ_NO_FREE_TOKENS;
                }
            }
        }

        details::fj_parser parser{};
        details::fj_init(&parser, ptr, size, storage_type::begin(), N);
        details::parse_result res = details::fj_parse(&parser);
        if ( res.ec ) {
            m_used = 0;
            m_err = res.ec;
            return false;
        }

        m_used = res.toknum;

        return true;
    }

    std::string dump(std::size_t indent = 0) const {
        std::size_t strlen = details::fj_str_length(storage_type::begin(), m_used, indent);
        std::string res(strlen, 0);
        assert(strlen == details::fj_tokens_to_buf(
                 storage_type::begin()
                ,m_used
                ,const_cast<char *>(res.c_str())
                ,res.size()
                ,indent
            )
        );

        return res;
    }
    std::ostream& dump(std::ostream &os, std::size_t indent = 0) const {
        details::fj_tokens_to_stream(os, storage_type::begin(), m_used, indent);

        return os;
    }

    friend std::ostream& operator<< (std::ostream &os, const fjson &fj) {
        details::fj_tokens_to_stream(os, fj.m_tokens, fj.m_used);
        return os;
    }

private:
    std::size_t m_used;
    e_fj_error_code m_err;
};

/*************************************************************************************************/

struct fdyjson
    :private details::fjson_dynamic_storage
    ,public details::fjson_ro_base<fdyjson, details::fjson_dynamic_storage>
{
private:
    using storage_type = details::fjson_dynamic_storage;
    using ro_base_type = details::fjson_ro_base<fdyjson, storage_type>;

    friend ro_base_type;

public:
    fdyjson()
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_err{}
    {}
    template<std::size_t L>
    explicit fdyjson(const char (&str)[L])
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_err{}
    { load(str, L-1); }
    template<
         typename ConstCharPtr
        ,typename = typename std::enable_if<
            std::is_same<ConstCharPtr, const char*>::value
        >::type
    >
    explicit fdyjson(ConstCharPtr str)
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_err{}
    { load(str, std::strlen(str)); }
    fdyjson(const char *ptr, std::size_t size)
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_err{}
    { load(ptr, size); }

    virtual ~fdyjson() = default;

private:
    fdyjson(const details::fj_pair *tokens, std::size_t num)
        :storage_type{}
        ,ro_base_type{*static_cast<storage_type *>(this)}
        ,m_err{}
    { assign_(tokens, num); }

public:
    bool valid() const { return storage_type::size_() != 0 && m_err == FJ_OK; }
    e_fj_error_code error() const { return m_err; }

    // for arrays
    fdyjson operator[](std::size_t idx) const { return ro_base_type::at(idx);}

    // for objects
    template<std::size_t KL>
    fdyjson operator[](const char (&key)[KL]) const { return at(key, KL-1); }
    template<
         typename ConstCharPtr
        ,typename = typename std::enable_if<
            std::is_same<ConstCharPtr, const char*>::value
        >::type
    >
    fdyjson operator[](ConstCharPtr key) const { return at(key, std::strlen(key)); }

    bool load(const char *ptr, std::size_t size) {
        auto res = details::fj_num_tokens(ptr, size);
        if ( res.ec ) {
            m_err = res.ec;

            return false;
        }

        storage_type::resize_(res.toknum);
        details::fj_parser parser{};
        details::fj_init(&parser, ptr, size, &(*storage_type::begin()), res.toknum);
        res = details::fj_parse(&parser);
        if ( res.ec ) {
            m_err = res.ec;

            return false;
        }

        if ( res.toknum != storage_type::size_() ) {
            m_err = FJ_NO_FREE_TOKENS;

            return false;
        }

        return true;
    }

    std::string dump(std::size_t indent = 0) const {
        std::size_t strlen = details::fj_str_length(&(*storage_type::begin()), storage_type::size_(), indent);
        std::string res(strlen, 0);
        assert(strlen == details::fj_tokens_to_buf(
                 &(*storage_type::begin())
                ,storage_type::size_()
                ,const_cast<char *>(res.c_str())
                ,res.size()
                ,indent
            )
        );

        return res;
    }
    std::ostream& dump(std::ostream &os, std::size_t indent = 0) const {
        details::fj_tokens_to_stream(os, storage_type::begin(), size_(), indent);

        return os;
    }

    friend std::ostream& operator<< (std::ostream &os, const fdyjson &fj) {
        details::fj_tokens_to_stream(os, &(*fj.begin()), static_cast<const storage_type &>(fj).size_());
        return os;
    }

private:
    e_fj_error_code m_err;
};

/*************************************************************************************************/

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
#undef __FLATJSON__IS_SIMPLE_TYPE
#undef __FLATJSON__IS_DIGIT
#undef __FLATJSON__IS_HEX_DIGIT
#undef __FLATJSON__IS_HEX_DIGIT4
#undef __FLATJSON__CUR_CHAR
#undef __FLATJSON__UTF8_CHAR_LEN

/*************************************************************************************************/

#endif // __FLATJSON__FLATJSON_HPP
