
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__PARSE_HPP
#define __FLATJSON__PARSE_HPP

#include "error.hpp"
#include "types.hpp"
#include "args.hpp"

#include <iterator>
#include <utility>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace flatjson {

/*************************************************************************************************/

enum parse_flags: std::uint8_t {
     FJ_PARSE_NONE = 0
    ,FJ_PARSE_ALLOW_COMMENTS = 1u << 0
    ,FJ_PARSE_ALLOW_INF_NAN = 1u << 1
};

struct cursor {
    error_code code;
    const char *next;
    std::size_t remaining;
};

namespace details {

enum parse_lex: std::uint8_t {
     FJ_LEX_NONE = 0
    ,FJ_LEX_COMMENT_SLASH
    ,FJ_LEX_LINE_COMMENT
    ,FJ_LEX_TOP_NUMBER
};

/*************************************************************************************************/

// escaped
static const std::uint8_t fj_is_escaped_chars_map[] = {
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0
    ,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static_assert(sizeof(fj_is_escaped_chars_map) == 256, "");

// 0x00/0x09/0x0a/0x0d/0x20
static const std::uint8_t fj_is_whitespace_char_map[] = {
     0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static_assert(sizeof(fj_is_whitespace_char_map) == 256, "");

/*************************************************************************************************/
// string

inline bool is_hex4(const char *p) noexcept {
    std::uint32_t x{};
    std::memcpy(&x, p, sizeof(x));
    const std::uint32_t ones = 0x01010101u;
    const std::uint32_t highs = 0x80808080u;
    const std::uint32_t low7 = x & ~highs;
    const std::uint32_t lower = low7 | (0x20u * ones);
    const std::uint32_t digit = (low7 + 0x50u * ones) & ~(low7 + 0x46u * ones);
    const std::uint32_t alpha = (lower + 0x1fu * ones) & ~(lower + 0x19u * ones);

    return ((digit | alpha) & ~x & highs) == highs;
}

inline bool is_special_string_byte(std::uint8_t b) noexcept
{ return b < 0x20u || b == static_cast<std::uint8_t>('"') || b == static_cast<std::uint8_t>('\\'); }

inline std::uint64_t load_le_8(const char *ptr) noexcept {
    std::uint64_t x{};
    std::memcpy(&x, ptr, sizeof(x));
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    x = __builtin_bswap64(x);
#endif // defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

    return x;
}

inline std::uint64_t special_string_mask_8(const char *ptr) noexcept {
    const std::uint64_t x = load_le_8(ptr);
    static constexpr std::uint64_t ones = 0x0101010101010101ULL;
    static constexpr std::uint64_t highs = 0x8080808080808080ULL;
    const std::uint64_t q = x ^ (ones * 0x02u);
    const std::uint64_t b = x ^ (ones * static_cast<std::uint8_t>('\\'));

    return ((q - ones * 0x21u) | (b - ones)) & ~x & highs;
}

inline std::uint64_t non_digit_mask_8(const char *ptr) noexcept {
    const std::uint64_t x = load_le_8(ptr);

    return ((x - 0x3030303030303030ULL) | (x + 0x4646464646464646ULL))
        & 0x8080808080808080ULL
    ;
}

// m must be non-zero.
inline unsigned first_marked_byte(std::uint64_t m) noexcept {
#if defined(_MSC_VER)
    unsigned long idx{};
#   if defined(_M_X64) || defined(_M_ARM64)
    ::_BitScanForward64(&idx, m);
#   else
    if ( !::_BitScanForward(&idx, static_cast<unsigned long>(m)) ) {
        ::_BitScanForward(&idx, static_cast<unsigned long>(m >> 32));
        idx += 32u;
    }
#   endif // defined(_M_X64) || defined(_M_ARM64)

    return static_cast<unsigned>(idx) >> 3;
#else
    return static_cast<unsigned>(__builtin_ctzll(m)) >> 3;
#endif // defined(_MSC_VER)
}

inline const char* skip_plain_string_bytes(const char *cur, const char *end) noexcept {
    if ( end - cur >= 8 ) {
        const std::uint64_t m = special_string_mask_8(cur);
        if ( m != 0u ) {
            for ( unsigned i = 0u; i < 7u; ++i ) {
                if ( m & (0x80ull << (i * 8u)) ) {
                    return cur + i;
                }
            }

            return cur + 7;
        }
        cur += 8;
    }

    while ( end - cur >= 8 ) {
        const std::uint64_t m = special_string_mask_8(cur);
        if ( m != 0u ) {
            return cur + first_marked_byte(m);
        }
        cur += 8;
    }

    while ( cur < end && !is_special_string_byte(static_cast<std::uint8_t>(*cur)) ) {
        ++cur;
    }

    return cur;
}

/*************************************************************************************************/

inline const char* virtual_base(const parser *parser) noexcept {
    const auto addr = reinterpret_cast<std::uintptr_t>(parser->resume.chunk_beg);

    return reinterpret_cast<const char *>(addr - parser->resume.chunk_off);
}

inline error_code add_chunk(parser *parser, const char *data, std::size_t size) noexcept {
    auto &cl = parser->chunks;
    const chunk &last = cl.num > 1u ? cl.beg[cl.num - 1u] : cl.first;
    const std::uint64_t offset = cl.num ? std::uint64_t{last.offset} + last.size : 0u;
    if ( offset + size > std::uint64_t{0xffffffffu} ) {
        return FJ_INVALID;
    }

    const chunk added{data, static_cast<std::uint32_t>(size), static_cast<std::uint32_t>(offset)};
    if ( cl.num == 0u ) {
        cl.first = added;
    } else {
        if ( cl.num >= cl.cap ) {
            auto *alloc_fn = parser->alloc_fn;
            if ( (cl.beg && !cl.dyn) || !alloc_fn ) {
                return FJ_NO_FREE_CHUNKS;
            }
            const std::uint32_t ncap = cl.cap < 8u ? 8u : cl.cap * 2u;
            auto *mem = static_cast<chunk *>(alloc_fn(cl.beg, ncap * sizeof(chunk)));
            if ( !mem ) {
                return FJ_CANT_ALLOCATE;
            }
            cl.beg = mem;
            cl.cap = ncap;
            cl.dyn = true;
        }
        if ( cl.num == 1u ) {
            cl.beg[0] = cl.first;
        }
        cl.beg[cl.num] = added;
    }
    ++cl.num;
    parser->resume.chunk_beg = data;
    parser->resume.chunk_off = added.offset;
    parser->resume.json_cur = data;
    parser->resume.json_end = data + size;

    return FJ_OK;
}

inline const char* content_end(const parser *parser) noexcept {
    const char *end = parser->resume.json_end;
    if ( !parser->resume.streaming && end > parser->resume.chunk_beg && end[-1] == '\0' ) {
        return end - 1;
    }

    return end;
}

inline __FJ__NOINLINE error_code lexeme_error(
     parser *parser
    ,const char *at
    ,error_code ec
    ,const char *func
    ,const char *prev_func) noexcept
{
    parser->resume.json_cur = at;
    __FJ__INIT_ERROR_INFO(parser, ec, prev_func);
    parser->error.func = func;

    return ec;
}

template<bool Sentinel = false, bool Report = true>
__FJ__ALWAYS_INLINE error_code parse_string(
     const char *prev_func
    ,parser *parser
    ,const char *&cur
    ,const char *end) noexcept
{
    for ( ;; ) {
        cur = skip_plain_string_bytes(cur, end);
        if ( Sentinel && __FJ__LIKELY(*cur == '"') ) {
            return FJ_OK;
        }
        if ( __FJ__UNLIKELY(cur >= end) ) {
            break;
        }

        const char ch = *cur;
        if ( !Sentinel && __FJ__LIKELY(ch == '"') ) {
            return FJ_OK;
        }
        if ( ch != '\\' ) {
            const error_code ec = ch == '\0'
                ? FJ_INCOMPLETE
                : FJ_INVALID
            ;

            return Report ? lexeme_error(parser, cur, ec, __func__, prev_func) : ec;
        }

        do {
            ++cur; // move to escaped char
            if ( __FJ__UNLIKELY(cur >= end) ) {
                return Report
                    ? lexeme_error(parser, cur, FJ_INCOMPLETE, __func__, prev_func)
                    : FJ_INCOMPLETE
                ;
            }

            const std::uint8_t esc = static_cast<std::uint8_t>(*cur);
            if ( __FJ__LIKELY(esc != 'u') ) {
                if ( __FJ__LIKELY(fj_is_escaped_chars_map[esc]) ) {
                    ++cur;
                    continue;
                }
                const bool line_break = esc == '\n' || esc == '\r';
                const error_code ec = line_break
                    ? FJ_INCOMPLETE
                    : FJ_INVALID
                ;

                return Report ? lexeme_error(parser, cur, ec, __func__, prev_func) : ec;
            }

            if ( __FJ__UNLIKELY(end - cur < 5) ) {
                return Report
                    ? lexeme_error(parser, cur, FJ_INCOMPLETE, __func__, prev_func)
                    : FJ_INCOMPLETE
                ;
            }

            if ( __FJ__UNLIKELY(!is_hex4(cur + 1)) ) {
                return Report
                    ? lexeme_error(parser, cur, FJ_INVALID, __func__, prev_func)
                    : FJ_INVALID
                ;
            }
            cur += 5; // 'u' + 4 hex digits
        } while ( cur < end && *cur == '\\' );
    }

    return Report
        ? lexeme_error(parser, cur, FJ_INCOMPLETE, __func__, prev_func)
        : FJ_INCOMPLETE
    ;
}

/*************************************************************************************************/
// number

inline bool parse_number_is_digit_char(std::uint8_t c) noexcept {
    return static_cast<std::uint32_t>(c - static_cast<std::uint8_t>('0')) <= 9u;
}

__FJ__ALWAYS_INLINE const char* skip_digits(const char *cur, const char *end) noexcept {
    while ( end - cur >= 8 ) {
        const std::uint64_t m = non_digit_mask_8(cur);
        if ( m != 0u ) {
            return cur + first_marked_byte(m);
        }
        cur += 8;
    }
    while ( cur < end && parse_number_is_digit_char(static_cast<std::uint8_t>(*cur)) ) {
        ++cur;
    }

    return cur;
}

__FJ__ALWAYS_INLINE error_code parse_number(
     const char *prev_func
    ,parser *parser
    ,const char *&cur
    ,const char *end) noexcept
{
    const char *const start = cur;
    if ( cur < end && *cur == '-' ) {
        ++cur;
    }

    if ( __FJ__UNLIKELY(cur >= end) ) {
        return lexeme_error(parser, start, FJ_INCOMPLETE, __func__, prev_func);
    }

    if ( *cur == '0' ) {
        ++cur;
        if ( cur < end && parse_number_is_digit_char(static_cast<std::uint8_t>(*cur)) ) {
            return lexeme_error(parser, start, FJ_INVALID, __func__, prev_func);
        }
    } else if ( parse_number_is_digit_char(static_cast<std::uint8_t>(*cur)) ) {
        cur = skip_digits(cur + 1, end);
    } else {
        return lexeme_error(parser, start, FJ_INVALID, __func__, prev_func);
    }

    if ( cur < end && *cur == '.' ) {
        ++cur;
        if ( __FJ__UNLIKELY(cur >= end) ) {
            return lexeme_error(parser, start, FJ_INCOMPLETE, __func__, prev_func);
        }
        if ( !parse_number_is_digit_char(static_cast<std::uint8_t>(*cur)) ) {
            return lexeme_error(parser, start, FJ_INVALID, __func__, prev_func);
        }
        cur = skip_digits(cur + 1, end);
    }
    if ( cur < end && (static_cast<std::uint8_t>(*cur) | 0x20u) == 'e' ) {
        ++cur;
        if ( cur < end && (*cur == '+' || *cur == '-') ) {
            ++cur;
        }
        if ( __FJ__UNLIKELY(cur >= end) ) {
            return lexeme_error(parser, start, FJ_INCOMPLETE, __func__, prev_func);
        }
        if ( !parse_number_is_digit_char(static_cast<std::uint8_t>(*cur)) ) {
            return lexeme_error(parser, start, FJ_INVALID, __func__, prev_func);
        }
        cur = skip_digits(cur + 1, end);
    }

    if ( __FJ__UNLIKELY(cur == end && parser->resume.streaming) ) {
        return lexeme_error(parser, start, FJ_INCOMPLETE, __func__, prev_func);
    }

    return FJ_OK;
}

/*************************************************************************************************/

inline __FJ__NOINLINE error_code ensure_token_slot(parser *parser, const char *prev_func) noexcept {
    if ( parser->toks_cur != parser->toks_end ) {
        return FJ_OK;
    }
    if ( !parser->dyn_alloc ) {
        __FJ__INIT_ERROR_INFO(parser, FJ_NO_FREE_TOKENS, prev_func);
        return FJ_NO_FREE_TOKENS;
    }

    auto *alloc_fn = parser->alloc_fn;
    if ( !alloc_fn ) {
        __FJ__INIT_ERROR_INFO(parser, FJ_CANT_ALLOCATE, prev_func);
        return FJ_CANT_ALLOCATE;
    }

    const auto used = static_cast<std::size_t>(parser->toks_cur - parser->toks_beg);
    const auto cap = static_cast<std::size_t>(parser->toks_end - parser->toks_beg);
    std::size_t ncap = cap < FJ_TOKEN_INIT_CAP
        ? FJ_TOKEN_INIT_CAP
        : cap * 2u
    ;
    if ( ncap < used + 1u ) {
        ncap = used + 1u;
    }

    auto *mem = static_cast<token *>(alloc_fn(parser->toks_beg, ncap * sizeof(token)));
    if ( !mem ) {
        __FJ__INIT_ERROR_INFO(parser, FJ_CANT_ALLOCATE, prev_func);
        return FJ_CANT_ALLOCATE;
    }

    parser->toks_beg = mem;
    parser->toks_cur = mem + used;
    parser->toks_end = mem + ncap;

    return FJ_OK;
}

/*************************************************************************************************/

inline void reset_progress(parser *parser) noexcept {
    auto &rs = parser->resume;

    const auto flags = rs.flags;
    const auto streaming = rs.streaming;
    const auto eos = rs.eos;
    const auto chunk_off = rs.chunk_off;
    const auto *chunk_beg = rs.chunk_beg;
    const auto *json_end = rs.json_end;

    std::memset(&rs, 0, sizeof(rs));
    rs.flags = flags;
    rs.streaming = streaming;
    rs.eos = eos;
    rs.chunk_off = chunk_off;
    rs.chunk_beg = chunk_beg;
    rs.json_end = json_end;

    rs.json_cur = chunk_beg;
    parser->toks_cur = parser->toks_beg;
    if ( parser->toks_beg ) {
        auto *root = parser->toks_beg;
        root->type = FJ_TYPE_INVALID;
        root->vpos = 0u;
        root->vlen = 0u;
        root->parent_idx = 0u;
        root->end_idx = 0u;
        root->primitive_only = 0u;
    }
}

/*************************************************************************************************/

enum parse_state: std::uint8_t {
     ST_VALUE = 0
    ,ST_VALUE_OR_ARR_END
    ,ST_KEY_OR_OBJ_END
    ,ST_COLON
    ,ST_COMMA_OR_OBJ_END
    ,ST_COMMA_OR_ARR_END
    ,ST_DONE
    ,ST_KEY
};

__FJ__ALWAYS_INLINE bool is_whitespace_byte(char c) noexcept {
    constexpr std::uint64_t k_whitespace_mask =
        (1ull << ' ' ) |
        (1ull << '\t') |
        (1ull << '\n') |
        (1ull << '\r')
    ;

    const auto u = static_cast<std::uint8_t>(c);

    return u <= ' ' && ((k_whitespace_mask >> u) & 1u) != 0u;
}

inline __FJ__NOINLINE const char* skip_whitespace_run(const char *p, const char *limit) noexcept {
    while ( p < limit && is_whitespace_byte(*p) ) {
        const char c = *p;
        const std::uint64_t run = 0x0101010101010101ULL * static_cast<std::uint8_t>(c);
        for ( ;; ) {
            if ( limit - p < 8 ) {
                while ( p < limit && *p == c ) {
                    ++p;
                }
                break;
            }
            const std::uint64_t d = load_le_8(p) ^ run;
            if ( d != 0u ) {
                p += first_marked_byte(d);
                break;
            }
            p += 8;
        }
    }

    return p;
}

template<bool Sentinel>
__FJ__ALWAYS_INLINE const char* skip_whitespace(const char *p, const char *limit) noexcept {
    if ( (!Sentinel && p >= limit) || !is_whitespace_byte(*p) ) {
        return p;
    }

    ++p;
    if ( (!Sentinel && p >= limit) || !is_whitespace_byte(*p) ) {
        return p;
    }

    return skip_whitespace_run(p + 1, limit);
}

inline std::uint32_t load_u32(const char *p) noexcept {
    std::uint32_t v{};
    std::memcpy(&v, p, sizeof(v));

    return v;
}

#if defined(__GNUC__) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#   define __FJ__TOKEN_WORDS 1
#endif // defined(__GNUC__) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#if defined(__GNUC__) && !defined(__clang__)
#   define __FJ__HOT_LOOP __attribute__((optimize("no-tree-slp-vectorize", "ira-region=one")))
#else
#   define __FJ__HOT_LOOP
#endif // defined(__GNUC__) && !defined(__clang__)

// the parent part of a token, prepared once per container.
struct tok_parent {
#ifdef __FJ__TOKEN_WORDS
    std::uint64_t lo; // parent_idx bits 0..1 as bits 62..63 of the first word
    std::uint64_t hi; // parent_idx bits 2..29
#else
    std::uint32_t idx;
#endif // __FJ__TOKEN_WORDS
};

__FJ__ALWAYS_INLINE tok_parent make_tok_parent(std::uint32_t parent) noexcept {
#ifdef __FJ__TOKEN_WORDS
    return tok_parent{std::uint64_t{parent} << 62, std::uint64_t{parent >> 2}};
#else
    return tok_parent{parent};
#endif // __FJ__TOKEN_WORDS
}

__FJ__ALWAYS_INLINE void put_token(
     token *t
    ,std::uint32_t vpos
    ,std::uint32_t vlen
    ,tok_parent tp
    ,std::uint32_t type) noexcept
{
#ifdef __FJ__TOKEN_WORDS
    char *const raw = reinterpret_cast<char *>(t);
    const std::uint64_t lo = vpos | (std::uint64_t{vlen} << 32) | tp.lo;
    const std::uint64_t hi = tp.hi | (std::uint64_t{type & 0xfu} << 58);
    std::memcpy(raw, &lo, sizeof(lo));
    std::memcpy(raw + sizeof(lo), &hi, sizeof(hi));
#else
    token v{};
    v.vpos = vpos;
    v.vlen = vlen;
    v.parent_idx = tp.idx;
    v.type = type;
    *t = v;
#endif // __FJ__TOKEN_WORDS
}

__FJ__ALWAYS_INLINE tok_parent finish_container(
     token *t
    ,std::uint32_t end_pos
    ,std::size_t end_idx
    ,std::uint32_t prim) noexcept
{
#ifdef __FJ__TOKEN_WORDS
    char *const raw = reinterpret_cast<char *>(t);
    std::uint64_t lo{};
    std::uint64_t hi{};
    std::memcpy(&lo, raw, sizeof(lo));
    std::memcpy(&hi, raw + sizeof(lo), sizeof(hi));
    const auto vpos = static_cast<std::uint32_t>(lo);
    lo |= std::uint64_t{end_pos - vpos} << 32;
    hi |= (static_cast<std::uint64_t>(end_idx) << 28) | (std::uint64_t{prim} << 62);
    std::memcpy(raw, &lo, sizeof(lo));
    std::memcpy(raw + sizeof(lo), &hi, sizeof(hi));

    return tok_parent{lo >> 62 << 62, hi & 0x0fffffffu};
#else
    token v = *t;
    v.vlen = end_pos - v.vpos;
    v.primitive_only = prim;
    v.end_idx = static_cast<std::uint32_t>(end_idx);
    *t = v;

    return tok_parent{v.parent_idx};
#endif // __FJ__TOKEN_WORDS
}

__FJ__ALWAYS_INLINE bool is_digit_byte(char c) noexcept
{ return parse_number_is_digit_char(static_cast<std::uint8_t>(c)); }

__FJ__ALWAYS_INLINE const char* skip_digits_sentinel(const char *cur, const char *end) noexcept {
    if ( !is_digit_byte(cur[0]) ) {
        return cur;
    }
    if ( !is_digit_byte(cur[1]) ) {
        return cur + 1;
    }
    if ( !is_digit_byte(cur[2]) ) {
        return cur + 2;
    }

    return skip_digits(cur + 3, end);
}

__FJ__ALWAYS_INLINE const char* scan_number_sentinel(const char *cur, const char *end) noexcept {
    if ( *cur == '-' ) {
        ++cur;
    }
    if ( *cur == '0' ) {
        ++cur;
        if ( is_digit_byte(*cur) ) {
            return nullptr;
        }
    } else if ( is_digit_byte(*cur) ) {
        cur = skip_digits_sentinel(cur + 1, end);
    } else {
        return nullptr;
    }
    if ( *cur == '.' ) {
        ++cur;
        if ( !is_digit_byte(*cur) ) {
            return nullptr;
        }
        cur = skip_digits_sentinel(cur + 1, end);
    }
    if ( (static_cast<std::uint8_t>(*cur) | 0x20u) == 'e' ) {
        ++cur;
        if ( *cur == '+' || *cur == '-' ) {
            ++cur;
        }
        if ( !is_digit_byte(*cur) ) {
            return nullptr;
        }
        cur = skip_digits_sentinel(cur + 1, end);
    }

    return cur;
}

enum value_kind: std::uint8_t {
     VK_LEAF
    ,VK_OBJECT
    ,VK_ARRAY
    ,VK_STOP
};

template<bool Sentinel>
__FJ__ALWAYS_INLINE value_kind scan_value(
     parser *parser
    ,const char *&p
    ,const char *limit
    ,token *&tc
    ,const char *base
    ,tok_parent tp) noexcept
{
    const char c = *p;
    if ( c == '"' ) {
        const char *cur = p + 1;
        const auto ec = parse_string<Sentinel, false>(nullptr, parser, cur, limit);
        if ( __FJ__UNLIKELY(ec != FJ_OK) ) {
            return VK_STOP;
        }
        put_token(
             tc++
            ,static_cast<std::uint32_t>(p + 1 - base)
            ,static_cast<std::uint32_t>(cur - (p + 1))
            ,tp
            ,FJ_TYPE_STRING
        );
        p = cur + 1;

        return VK_LEAF;
    }
    if ( is_digit_byte(c) || c == '-' ) {
        const char *cur = p;
        if ( Sentinel ) {
            cur = scan_number_sentinel(p, limit);
            if ( __FJ__UNLIKELY(!cur) ) {
                return VK_STOP;
            }
        } else if ( __FJ__UNLIKELY(parse_number(nullptr, parser, cur, limit) != FJ_OK) ) {
            return VK_STOP;
        }
        put_token(
             tc++
            ,static_cast<std::uint32_t>(p - base)
            ,static_cast<std::uint32_t>(cur - p)
            ,tp
            ,FJ_TYPE_NUMBER
        );
        p = cur;

        return VK_LEAF;
    }
    if ( c == '{' ) {
        return VK_OBJECT;
    }
    if ( c == '[' ) {
        return VK_ARRAY;
    }

    if ( (static_cast<std::uint8_t>(c) | 0x12u) == 0x76u ) {
        const std::uint32_t is_false = c == 'f';
        const std::uint32_t t_word = load_u32("true");
        const std::uint32_t want = t_word ^ ((t_word ^ load_u32("alse")) & (0u - is_false));
        const char *word = p + is_false;
        if ( limit - word < 4 || load_u32(word) != want ) {
            return VK_STOP;
        }
        put_token(tc++, static_cast<std::uint32_t>(p - base), 4u + is_false, tp, FJ_TYPE_BOOL);
        p = word + 4;

        return VK_LEAF;
    }
    if ( c == 'n' && limit - p >= 4 && load_u32(p) == load_u32("null") ) {
        put_token(tc++, static_cast<std::uint32_t>(p - base), 4u, tp, FJ_TYPE_NULL);
        p += 4;

        return VK_LEAF;
    }

    return VK_STOP;
}

__FJ__ALWAYS_INLINE void open_container_fast(
     std::uint32_t type
    ,const char *&p
    ,const char *base
    ,const token *tb
    ,token *&tc
    ,parse_frame *&ctx
    ,tok_parent &tp) noexcept
{
    const auto idx = static_cast<std::uint32_t>(tc - tb);
    put_token(tc++, static_cast<std::uint32_t>(p - base), 0u, tp, type);
    ++p;
    ctx->primitive_only = 0u;
    ++ctx;
#ifdef __FJ__TOKEN_WORDS
    const std::uint64_t head = idx | (std::uint64_t{type} << 32) | (std::uint64_t{1u} << 48);
    std::memcpy(ctx, &head, sizeof(head));
#else
    ctx->parent_idx = idx;
    ctx->parent_type = static_cast<std::uint16_t>(type);
    ctx->primitive_only = 1u;
#endif // __FJ__TOKEN_WORDS
    tp = make_tok_parent(idx);
}

__FJ__ALWAYS_INLINE void close_container_fast(
     std::uint32_t end_type
    ,const char *&p
    ,const char *base
    ,token *tb
    ,token *&tc
    ,parse_frame *&ctx
    ,tok_parent &tp) noexcept
{
    const auto end_idx = static_cast<std::size_t>(tc - tb);
    put_token(tc++, 0u, 0u, tp, end_type);
    ++p;
    tp = finish_container(
         tb + ctx->parent_idx
        ,static_cast<std::uint32_t>(p - base)
        ,end_idx
        ,ctx->primitive_only
    );
    --ctx;
}

template<bool Sentinel>
__FJ__ALWAYS_INLINE bool fast_stop(
     const char *p
    ,const char *stop_at
    ,const token *tc
    ,const token *tc_stop) noexcept
{ return Sentinel ? tc == tc_stop : p >= stop_at; }

template<bool Sentinel, bool SkipWs, bool WsFirst>
__FJ__HOT_LOOP __FJ__NOINLINE const char* parse_doc_fast(
     parser *parser
    ,const char *p
    ,const char *limit
    ,const char *stop_at) noexcept
{
    auto &rs = parser->resume;
    const char *const base = virtual_base(parser);
    token *const tb = parser->toks_beg;
    token *tc = parser->toks_cur;
    const token *const tc_stop = parser->toks_end;
    parse_frame *ctx = &rs.stack[rs.depth - 1u];
    parse_frame *const ctx_last = &rs.stack[k_parse_stack_max - 1u];
    tok_parent tp = make_tok_parent(ctx->parent_idx);
    std::uint8_t state = 0u;

    switch ( ctx->state ) {
        case ST_VALUE: {
            if ( ctx->parent_type == FJ_TYPE_OBJECT ) {
                goto obj_value;
            }
            if ( ctx->parent_type == FJ_TYPE_ARRAY ) {
                goto arr_value;
            }
            goto root_value;
        }
        case ST_VALUE_OR_ARR_END: goto arr_first;
        case ST_KEY_OR_OBJ_END: goto obj_first;
        case ST_KEY: goto key;
        case ST_COLON: goto colon;
        case ST_COMMA_OR_OBJ_END: goto obj_next;
        case ST_COMMA_OR_ARR_END: goto arr_next;
        default: goto stop_done;
    }

root_value:
    if ( SkipWs ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
    }
    if ( __FJ__UNLIKELY(fast_stop<Sentinel>(p, stop_at, tc, tc_stop)) ) {
        goto stop_value;
    }
    if ( *p == '{' ) {
        goto open_object;
    }
    if ( *p == '[' ) {
        goto open_array;
    }
    goto stop_value;

open_object:
    if ( __FJ__UNLIKELY(ctx == ctx_last) ) {
        goto stop_value;
    }
    open_container_fast(FJ_TYPE_OBJECT, p, base, tb, tc, ctx, tp);
    goto obj_first;

open_array:
    if ( __FJ__UNLIKELY(ctx == ctx_last) ) {
        goto stop_value;
    }
    open_container_fast(FJ_TYPE_ARRAY, p, base, tb, tc, ctx, tp);
    goto arr_first;

close_object:
    close_container_fast(FJ_TYPE_OBJECT_END, p, base, tb, tc, ctx, tp);
    goto after_close;

close_array:
    close_container_fast(FJ_TYPE_ARRAY_END, p, base, tb, tc, ctx, tp);

after_close:
    if ( ctx->parent_type == FJ_TYPE_OBJECT ) {
        goto obj_next;
    }
    if ( ctx->parent_type == FJ_TYPE_ARRAY ) {
        goto arr_next;
    }
    goto stop_done;

arr_first:
    if ( WsFirst ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
    }
    if ( __FJ__UNLIKELY(fast_stop<Sentinel>(p, stop_at, tc, tc_stop)) ) {
        goto stop_arr_first;
    }
    if ( *p == ']' ) {
        goto close_array;
    }
    if ( !WsFirst && SkipWs && is_whitespace_byte(*p) ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
        goto arr_first;
    }
    goto arr_value_here;

arr_value:
    if ( WsFirst ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
    }
    if ( __FJ__UNLIKELY(fast_stop<Sentinel>(p, stop_at, tc, tc_stop)) ) {
        goto stop_value;
    }
arr_value_here:
    if ( *p == '{' ) {
        goto open_object;
    }
    if ( *p == '[' ) {
        goto open_array;
    }
    switch ( scan_value<Sentinel>(parser, p, limit, tc, base, tp) ) {
        case VK_LEAF: break;
        case VK_OBJECT: goto open_object;
        case VK_ARRAY: goto open_array;
        default: {
            if ( !WsFirst && SkipWs && is_whitespace_byte(*p) ) {
                p = skip_whitespace<Sentinel>(p, stop_at);
                goto arr_value;
            }
            goto stop_value;
        }
    }

arr_next:
    if ( WsFirst ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
    }
    if ( __FJ__UNLIKELY(fast_stop<Sentinel>(p, stop_at, tc, tc_stop)) ) {
        goto stop_arr_next;
    }
    if ( *p == ',' ) {
        ++p;
        goto arr_value;
    }
    if ( *p == ']' ) {
        goto close_array;
    }
    if ( !WsFirst && SkipWs && is_whitespace_byte(*p) ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
        goto arr_next;
    }
    goto stop_arr_next;

obj_first:
    if ( WsFirst ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
    }
    if ( __FJ__UNLIKELY(fast_stop<Sentinel>(p, stop_at, tc, tc_stop)) ) {
        goto stop_obj_first;
    }
    if ( *p == '"' ) {
        goto key_here;
    }
    if ( *p == '}' ) {
        goto close_object;
    }
    if ( !WsFirst && SkipWs && is_whitespace_byte(*p) ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
        goto obj_first;
    }
    goto key_here;

key:
    if ( WsFirst ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
    }
    if ( __FJ__UNLIKELY(fast_stop<Sentinel>(p, stop_at, tc, tc_stop)) ) {
        goto stop_key;
    }
key_here:
    if ( __FJ__UNLIKELY(*p != '"') ) {
        if ( !WsFirst && SkipWs && is_whitespace_byte(*p) ) {
            p = skip_whitespace<Sentinel>(p, stop_at);
            goto key;
        }
        goto stop_key;
    }
    {
        const char *cur = p + 1;
        const auto ec = parse_string<Sentinel, false>(nullptr, parser, cur, limit);
        if ( __FJ__UNLIKELY(ec != FJ_OK) ) {
            goto stop_key;
        }
        put_token(
             tc++
            ,static_cast<std::uint32_t>(p + 1 - base)
            ,static_cast<std::uint32_t>(cur - (p + 1))
            ,tp
            ,FJ_TYPE_STRING
        );
        p = cur + 1;
    }

colon:
    if ( WsFirst ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
    }
    if ( !Sentinel && __FJ__UNLIKELY(p >= stop_at) ) {
        goto stop_colon;
    }
    if ( __FJ__UNLIKELY(*p != ':') ) {
        if ( !WsFirst && SkipWs && is_whitespace_byte(*p) ) {
            p = skip_whitespace<Sentinel>(p, stop_at);
            goto colon;
        }
        goto stop_colon;
    }
    ++p;

obj_value:
    if ( WsFirst ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
    }
    if ( __FJ__UNLIKELY(fast_stop<Sentinel>(p, stop_at, tc, tc_stop)) ) {
        goto stop_value;
    }
    switch ( scan_value<Sentinel>(parser, p, limit, tc, base, tp) ) {
        case VK_LEAF: break;
        case VK_OBJECT: goto open_object;
        case VK_ARRAY: goto open_array;
        default: {
            if ( !WsFirst && SkipWs && is_whitespace_byte(*p) ) {
                p = skip_whitespace<Sentinel>(p, stop_at);
                goto obj_value;
            }
            goto stop_value;
        }
    }

obj_next:
    if ( WsFirst ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
    }
    if ( __FJ__UNLIKELY(fast_stop<Sentinel>(p, stop_at, tc, tc_stop)) ) {
        goto stop_obj_next;
    }
    if ( *p == ',' ) {
        ++p;
        goto key;
    }
    if ( *p == '}' ) {
        goto close_object;
    }
    if ( !WsFirst && SkipWs && is_whitespace_byte(*p) ) {
        p = skip_whitespace<Sentinel>(p, stop_at);
        goto obj_next;
    }
    goto stop_obj_next;

stop_value: state = ST_VALUE; goto stop;
stop_arr_first: state = ST_VALUE_OR_ARR_END; goto stop;
stop_arr_next: state = ST_COMMA_OR_ARR_END; goto stop;
stop_obj_first: state = ST_KEY_OR_OBJ_END; goto stop;
stop_key: state = ST_KEY; goto stop;
stop_colon: state = ST_COLON; goto stop;
stop_obj_next: state = ST_COMMA_OR_OBJ_END; goto stop;
stop_done: state = ST_DONE;

stop:
    ctx->state = state;
    rs.depth = static_cast<std::uint8_t>(ctx - rs.stack + 1);
    parser->toks_cur = tc;

    return p;
}

inline bool looks_indented(const char *p, const char *limit) noexcept {
    const std::size_t n = static_cast<std::size_t>(limit - p) < 64u
        ? static_cast<std::size_t>(limit - p)
        : 64u
    ;

    return std::memchr(p, '\n', n) != nullptr;
}

inline const char* run_parse_doc_fast(parser *parser, const char *p, const char *limit) noexcept {
    const std::size_t k_field_max = std::size_t{1} << 30;
    const auto toks_cap = parser->toks_end - parser->toks_beg;
    const auto limit_pos = parser->resume.chunk_off
        + static_cast<std::size_t>(limit - parser->resume.chunk_beg)
    ;
    if ( limit_pos >= k_field_max || static_cast<std::size_t>(toks_cap) >= k_field_max ) {
        return p;
    }

    const auto room = static_cast<std::size_t>(parser->toks_end - parser->toks_cur);
    const char *stop_at = room < static_cast<std::size_t>(limit - p) ? p + room : limit;
    const bool sentinel = !parser->resume.streaming
        && limit < parser->resume.json_end
        && *limit == '\0'
    ;
    if ( parser->despaced ) {
        return sentinel
            ? parse_doc_fast<true, false, false>(parser, p, limit, limit)
            : parse_doc_fast<false, false, false>(parser, p, limit, stop_at)
        ;
    }
    if ( looks_indented(p, limit) ) {
        return sentinel
            ? parse_doc_fast<true, true, true>(parser, p, limit, limit)
            : parse_doc_fast<false, true, true>(parser, p, limit, stop_at)
        ;
    }

    return sentinel
        ? parse_doc_fast<true, true, false>(parser, p, limit, limit)
        : parse_doc_fast<false, true, false>(parser, p, limit, stop_at)
    ;
}

inline error_code parse_doc(parser *parser, const char *prev_func) noexcept {
    auto &rs = parser->resume;
    const char *const here = __func__;
    const bool despaced = parser->despaced;
    const char *const vbase = virtual_base(parser);
    const char *const limit = content_end(parser);
    const char *p = parser->resume.json_cur;
    if ( p < rs.chunk_beg ) {
        p = rs.chunk_beg;
    }

    if ( rs.depth == 0u ) {
        rs.stack[0] = parse_frame{0u, FJ_TYPE_INVALID, 1u, ST_VALUE};
        rs.depth = 1u;
    }
    const bool allow_cmt = (rs.flags & FJ_PARSE_ALLOW_COMMENTS) != 0;
    const bool allow_inf_nan = (rs.flags & FJ_PARSE_ALLOW_INF_NAN) != 0;
    const bool gap_comments = allow_cmt
        || rs.lex == FJ_LEX_COMMENT_SLASH || rs.lex == FJ_LEX_LINE_COMMENT
    ;

    parse_frame *ctx = &rs.stack[rs.depth - 1u];

    auto push_frame = [&](parse_frame fr) -> bool {
        if ( rs.depth >= k_parse_stack_max ) {
            return false;
        }
        ctx = &rs.stack[rs.depth];
        *ctx = fr;
        ++rs.depth;

        return true;
    };

    auto pop_frame = [&]() -> parse_frame {
        const parse_frame closed = *ctx;
        --rs.depth;
        --ctx;

        return closed;
    };

    auto fail_at = [&](error_code ec) -> error_code {
        parser->resume.json_cur = p;
        __FJ__INIT_ERROR_INFO(parser, ec, prev_func);
        parser->error.func = here;

        return ec;
    };

    auto after_value = [&]() {
        if ( ctx->parent_type == FJ_TYPE_INVALID ) {
            ctx->state = ST_DONE;
        } else if ( ctx->parent_type == FJ_TYPE_OBJECT ) {
            ctx->state = ST_COMMA_OR_OBJ_END;
        } else {
            ctx->state = ST_COMMA_OR_ARR_END;
        }
    };

    auto reserve_slot = [&]() -> error_code {
        if ( __FJ__LIKELY(parser->toks_cur != parser->toks_end) ) {
            return FJ_OK;
        }
        parser->resume.json_cur = p;

        return ensure_token_slot(parser, here);
    };

    auto store_tok = [&](std::uint8_t type, std::uint32_t vpos, std::uint32_t vlen) {
        token t{};
        t.vpos = vpos;
        t.vlen = vlen;
        t.parent_idx = ctx->parent_idx;
        t.type = type;
        *parser->toks_cur = t;
    };

    auto hold_lexeme = [&](const char *start, error_code ec) -> error_code {
        p = start;
        if ( ec == FJ_SV_CROSS_CHUNK && parser->error.code != FJ_OK ) {
            parser->error.code = FJ_SV_CROSS_CHUNK;
            parser->resume.json_cur = p;

            return FJ_SV_CROSS_CHUNK;
        }

        return fail_at(ec);
    };

    auto commit_string = [&](bool is_key) -> error_code {
        const char *quote = p;
        const auto slot = reserve_slot();
        if ( slot != FJ_OK ) {
            return slot;
        }
        const char *cur = p + 1;
        const auto ec = parse_string(here, parser, cur, limit);
        if ( __FJ__UNLIKELY(ec != FJ_OK) ) {
            if ( ec == FJ_INCOMPLETE ) {
                const bool escaped_break = cur < limit && (*cur == '\n' || *cur == '\r');
                if ( escaped_break ) {
                    return hold_lexeme(quote, FJ_INCOMPLETE);
                }

                return hold_lexeme(quote, FJ_SV_CROSS_CHUNK);
            }
            p = cur;

            return ec;
        }
        store_tok(
             FJ_TYPE_STRING
            ,static_cast<std::uint32_t>(quote + 1 - vbase)
            ,static_cast<std::uint32_t>(cur - (quote + 1))
        );

        p = cur + 1;
        ++parser->toks_cur;

        if ( is_key ) {
            ctx->state = ST_COLON;
        } else {
            after_value();
        }

        return FJ_OK;
    };

    auto commit_number = [&]() -> error_code {
        const char *start = p;
        const auto slot = reserve_slot();
        if ( slot != FJ_OK ) {
            return slot;
        }

        const char *cur = p;
        const auto ec = parse_number(here, parser, cur, limit);
        if ( __FJ__UNLIKELY(ec != FJ_OK) ) {
            if ( ec != FJ_INCOMPLETE ) {
                return ec;
            }
            const bool top_complete = cur == limit
                && ctx->parent_type == FJ_TYPE_INVALID
                && parse_number_is_digit_char(static_cast<std::uint8_t>(cur[-1]))
            ;
            if ( !top_complete ) {
                return hold_lexeme(start, FJ_SV_CROSS_CHUNK);
            }
            rs.lex = FJ_LEX_TOP_NUMBER;
        }
        store_tok(
             FJ_TYPE_NUMBER
            ,static_cast<std::uint32_t>(start - vbase)
            ,static_cast<std::uint32_t>(cur - start)
        );
        p = cur;
        ++parser->toks_cur;
        after_value();

        return FJ_OK;
    };

    auto commit_span = [&](std::uint8_t type, const char *lit, std::size_t n) -> error_code {
        const auto have = static_cast<std::size_t>(limit - p);
        if ( have < n ) {
            if ( have != 0u && std::memcmp(p, lit, have) != 0 ) {
                return fail_at(FJ_INVALID);
            }

            return fail_at(FJ_SV_CROSS_CHUNK);
        }
        if ( std::memcmp(p, lit, n) != 0 ) {
            return fail_at(FJ_INVALID);
        }
        const auto slot = reserve_slot();
        if ( slot != FJ_OK ) {
            return slot;
        }
        store_tok(type, static_cast<std::uint32_t>(p - vbase), static_cast<std::uint32_t>(n));
        p += n;
        ++parser->toks_cur;
        after_value();

        return FJ_OK;
    };

    auto open_container = [&](std::uint8_t type) -> error_code {
        const auto slot = reserve_slot();
        if ( slot != FJ_OK ) {
            return slot;
        }
        const auto idx = static_cast<std::uint32_t>(parser->toks_cur - parser->toks_beg);
        store_tok(type, static_cast<std::uint32_t>(p - vbase), 0u);
        ctx->primitive_only = 0u;
        ++p;
        ++parser->toks_cur;
        const std::uint8_t next = type == FJ_TYPE_ARRAY
            ? static_cast<std::uint8_t>(ST_VALUE_OR_ARR_END)
            : static_cast<std::uint8_t>(ST_KEY_OR_OBJ_END)
        ;
        if ( !push_frame(parse_frame{idx, type, 1u, next}) ) {
            return fail_at(FJ_INVALID);
        }

        return FJ_OK;
    };

    auto close_container = [&](std::uint8_t end_type) -> error_code {
        const auto slot = reserve_slot();
        if ( slot != FJ_OK ) {
            return slot;
        }
        const auto end_idx = static_cast<std::uint32_t>(parser->toks_cur - parser->toks_beg);
        store_tok(end_type, 0u, 0u);
        const parse_frame closed = pop_frame();
        ++p;
        token *parent = parser->toks_beg + closed.parent_idx;
        token pt = *parent;
        pt.vlen = static_cast<std::uint32_t>(p - vbase) - pt.vpos;
        pt.primitive_only = closed.primitive_only;
        pt.end_idx = end_idx;
        *parent = pt;
        ++parser->toks_cur;
        after_value();

        return FJ_OK;
    };

    auto commit_value = [&]() -> error_code {
        const char c = *p;
        switch ( c ) {
            case '"': return commit_string(false);
            case '{': return open_container(FJ_TYPE_OBJECT);
            case '[': return open_container(FJ_TYPE_ARRAY);
            case 't': return commit_span(FJ_TYPE_BOOL, "true", 4u);
            case 'f': return commit_span(FJ_TYPE_BOOL, "false", 5u);
            case 'n': return commit_span(FJ_TYPE_NULL, "null", 4u);
            case 'N': return allow_inf_nan
                ? commit_span(FJ_TYPE_NUMBER, "NaN", 3u)
                : fail_at(FJ_INVALID)
            ;
            case 'I': return allow_inf_nan
                ? commit_span(FJ_TYPE_NUMBER, "Infinity", 8u)
                : fail_at(FJ_INVALID)
            ;
            case '-': {
                if ( allow_inf_nan && (p + 1 >= limit || p[1] == 'I') ) {
                    if ( p + 1 >= limit ) {
                        return fail_at(FJ_SV_CROSS_CHUNK);
                    }

                    return commit_span(FJ_TYPE_NUMBER, "-Infinity", 9u);
                }

                return commit_number();
            }
            default: {
                if ( c >= '0' && c <= '9' ) {
                    return commit_number();
                }

                return fail_at(FJ_INVALID);
            }
        }
    };

    auto skip_gap = [&]() -> error_code {
        if ( !gap_comments ) {
            if ( !despaced ) {
                while ( p < limit && fj_is_whitespace_char_map[static_cast<std::uint8_t>(*p)] ) {
                    ++p;
                }
            }

            return FJ_OK;
        }
        for ( ;; ) {
            if ( rs.lex == FJ_LEX_COMMENT_SLASH ) {
                if ( p >= limit ) {
                    return FJ_INCOMPLETE;
                }
                if ( *p != '/' ) {
                    rs.lex = FJ_LEX_NONE;
                    if ( ctx->state == ST_DONE ) {
                        return FJ_OK;
                    }

                    return FJ_INVALID;
                }
                ++p;
                rs.lex = FJ_LEX_LINE_COMMENT;
            }
            if ( rs.lex == FJ_LEX_LINE_COMMENT ) {
                while ( p < limit && *p != '\n' ) {
                    ++p;
                }
                if ( p >= limit ) {
                    if ( ctx->state == ST_DONE ) {
                        rs.lex = FJ_LEX_NONE;

                        return FJ_OK;
                    }

                    return FJ_INCOMPLETE;
                }
                ++p;
                rs.lex = FJ_LEX_NONE;
                continue;
            }
            if ( !despaced ) {
                while ( p < limit && fj_is_whitespace_char_map[static_cast<std::uint8_t>(*p)] ) {
                    ++p;
                }
            }
            if ( !allow_cmt || p >= limit || *p != '/' ) {
                return FJ_OK;
            }
            if ( p + 1 >= limit ) {
                ++p;
                rs.lex = FJ_LEX_COMMENT_SLASH;

                return FJ_INCOMPLETE;
            }
            if ( p[1] != '/' ) {
                return FJ_OK;
            }
            p += 2;
            rs.lex = FJ_LEX_LINE_COMMENT;
        }
    };

    if ( rs.lex == FJ_LEX_TOP_NUMBER && p < limit ) {
        const auto c = static_cast<std::uint8_t>(*p);
        if ( parse_number_is_digit_char(c) || c == '.' || (c | 0x20u) == 'e' ) {
            return fail_at(FJ_SV_CROSS_CHUNK);
        }
        rs.lex = FJ_LEX_NONE;
    }

    while ( p < limit || rs.lex == FJ_LEX_COMMENT_SLASH || rs.lex == FJ_LEX_LINE_COMMENT ) {
        if ( !gap_comments ) {
            p = run_parse_doc_fast(parser, p, limit);
            ctx = &rs.stack[rs.depth - 1u];
        }
        const auto gap = skip_gap();
        if ( gap == FJ_INCOMPLETE ) {
            parser->resume.json_cur = p;

            return fail_at(FJ_INCOMPLETE);
        }
        if ( gap == FJ_INVALID ) {
            return fail_at(FJ_INVALID);
        }
        if ( p >= limit ) {
            break;
        }

        const auto state = ctx->state;
        const char c = *p;
        error_code ec = FJ_OK;
        switch ( state ) {
            case ST_DONE: return fail_at(FJ_INVALID);

            case ST_COLON:
                if ( c != ':' ) {
                    return fail_at(FJ_INVALID);
                }
                ++p;
                ctx->state = ST_VALUE;
                break;

            case ST_COMMA_OR_OBJ_END:
                if ( c == ',' ) {
                    ++p;
                    ctx->state = ST_KEY;
                    break;
                }
                if ( c == '}' ) {
                    ec = close_container(FJ_TYPE_OBJECT_END);
                    if ( ec != FJ_OK ) {
                        return ec;
                    }
                    break;
                }
                return fail_at(FJ_INVALID);

            case ST_COMMA_OR_ARR_END:
                if ( c == ',' ) {
                    ++p;
                    ctx->state = ST_VALUE;
                    break;
                }
                if ( c == ']' ) {
                    ec = close_container(FJ_TYPE_ARRAY_END);
                    if ( ec != FJ_OK ) {
                        return ec;
                    }
                    break;
                }
                return fail_at(FJ_INVALID);

            case ST_KEY_OR_OBJ_END:
                if ( c == '}' ) {
                    ec = close_container(FJ_TYPE_OBJECT_END);
                    if ( ec != FJ_OK ) {
                        return ec;
                    }
                    break;
                }
                if ( c != '"' ) {
                    return fail_at(FJ_INVALID);
                }
                ec = commit_string(true);
                if ( ec != FJ_OK ) {
                    return ec;
                }
                break;

            case ST_KEY:
                if ( c != '"' ) {
                    return fail_at(FJ_INVALID);
                }
                ec = commit_string(true);
                if ( ec != FJ_OK ) {
                    return ec;
                }
                break;

            case ST_VALUE_OR_ARR_END:
                if ( c == ']' ) {
                    ec = close_container(FJ_TYPE_ARRAY_END);
                    if ( ec != FJ_OK ) {
                        return ec;
                    }
                    break;
                }
                ec = commit_value();
                if ( ec != FJ_OK ) {
                    return ec;
                }
                break;

            case ST_VALUE:
                ec = commit_value();
                if ( ec != FJ_OK ) {
                    return ec;
                }
                break;

            default: return fail_at(FJ_INVALID);
        }
    }

    parser->resume.json_cur = p;
    if ( ctx->parent_type != FJ_TYPE_INVALID || ctx->state != ST_DONE || rs.lex == FJ_LEX_TOP_NUMBER ) {
        __FJ__INIT_ERROR_INFO(parser, FJ_INCOMPLETE, prev_func);
        parser->error.func = here;

        return FJ_INCOMPLETE;
    }

    parser->error.code = FJ_OK;

    return FJ_OK;
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

inline void init_parser(
     parser *parser
    ,token *toks_beg
    ,token *toks_end
    ,chunk *chunks_beg
    ,chunk *chunks_end
    ,alloc_fnptr alloc_fn
    ,free_fnptr free_fn
    ,bool dyn_parser
    ,bool dyn_tokens
    ,bool despaced) noexcept
{
    // root token
    if ( toks_beg ) {
        toks_beg->type = FJ_TYPE_INVALID;
        toks_beg->vpos = 0u;
        toks_beg->vlen = 0u;
        toks_beg->parent_idx = 0u;
        toks_beg->end_idx = 0u;
        toks_beg->primitive_only = 0u;
    }

    auto &cl = parser->chunks;
    cl.first = chunk{};
    cl.beg = chunks_beg;
    cl.num = 0u;
    cl.cap = static_cast<std::uint32_t>(chunks_end - chunks_beg);
    cl.dyn = false;

    parser->toks_beg = toks_beg;
    parser->toks_cur = toks_beg;
    parser->toks_end = toks_end;

    parser->alloc_fn = alloc_fn;
    parser->free_fn = free_fn;

    parser->error.code = FJ_INVALID;

    parser->dyn_alloc = dyn_tokens;
    parser->dyn_parser = dyn_parser;
    parser->despaced = despaced;
    parser->ref_counter = 0u;
    std::memset(&parser->resume, 0, sizeof(parser->resume));
}

inline std::size_t estimate_tokens(const char *json_beg, const char *json_end) noexcept {
    const auto json_size = json_beg && json_end > json_beg
        ? static_cast<std::size_t>(json_end - json_beg)
        : std::size_t{0}
    ;

    return json_size / 4u + FJ_TOKEN_INIT_CAP;
}

inline error_code bind_json(parser *parser, const char *json_beg, const char *json_end) noexcept {
    auto *alloc_fn = parser->alloc_fn;
    auto *free_fn = parser->free_fn;
    const auto cap = static_cast<std::size_t>(parser->toks_end - parser->toks_beg);
    const auto want = estimate_tokens(json_beg, json_end);
    error_code ec = FJ_OK;
    if ( parser->dyn_alloc && alloc_fn && free_fn && cap < want ) {
        if ( parser->toks_beg ) {
            free_fn(parser->toks_beg);
        }
        auto *mem = static_cast<token *>(alloc_fn(nullptr, want * sizeof(token)));
        parser->toks_beg = mem;
        parser->toks_end = mem ? mem + want : nullptr;
        if ( !mem ) {
            ec = FJ_CANT_ALLOCATE;
        }
    }

    auto &rs = parser->resume;
    parser->chunks.num = 0u;
    rs.chunk_beg = nullptr;
    rs.chunk_off = 0u;
    rs.json_end = nullptr;
    if ( json_beg && ec == FJ_OK ) {
        const auto size = json_end > json_beg
            ? static_cast<std::size_t>(json_end - json_beg)
            : std::size_t{0};
        ec = add_chunk(parser, json_beg, size);
    }

    reset_progress(parser);
    rs.streaming = false;
    rs.eos = false;
    parser->error.code = FJ_INVALID;

    return ec;
}

} // ns details

/*************************************************************************************************/
// zero-alloc routines
// A parser has no document until parse() or feed() gives it one.

// toks_beg*   - required
// toks_end*   - required
// chunks_beg* - optional: a chunk table for feed(), together with chunks_end
// chunks_end* - optional
// mem_fns     - optional: grow the chunk table when none is given; free_parser() releases it
// despaced    - optional

// without a chunk table and mem_fns the document is one chunk: one parse() or feed() call.

template<typename ...Args>
parser make_parser(Args && ...fnargs) noexcept {
    __FJ__ACCEPT_ARGS(
         Args
        ,args.toks_beg
        ,args.toks_end
        ,args.chunks_beg
        ,args.chunks_end
        ,args.mem_fns
        ,args.despaced
    );
    __FJ__ARGS_PAIRED(
         Args
        ,args.chunks_beg
        ,args.chunks_end
        ,"chunks_beg and chunks_end are passed together"
    );
    token      *toks_beg = details::get(args.toks_beg, __FJ__FWD(Args, fnargs));
    token      *toks_end = details::get(args.toks_end, __FJ__FWD(Args, fnargs));
    chunk    *chunks_beg = details::get(args.chunks_beg, args.chunks_beg = nullptr ,__FJ__FWD(Args, fnargs));
    chunk    *chunks_end = details::get(args.chunks_end, args.chunks_end = nullptr ,__FJ__FWD(Args, fnargs));
    const auto   mem_fns = details::get(args.mem_fns, args.mem_fns = {nullptr, nullptr} ,__FJ__FWD(Args, fnargs));
    const bool  despaced = details::get(args.despaced, args.despaced = false, __FJ__FWD(Args, fnargs));

    parser parser;
    details::init_parser(
         &parser
        ,toks_beg
        ,toks_end
        ,chunks_beg
        ,chunks_end
        ,mem_fns.alloc
        ,mem_fns.free
        ,false
        ,false
        ,despaced
    );

    return parser;
}

// init an empty parser
// it is implied that this function will be used for unpack a previously packed JSON-state.

// parser*    - required
// mem_fns    - optional

template<typename ...Args>
inline void init_parser(Args && ...fnargs) noexcept {
    __FJ__ACCEPT_ARGS(Args, args.parser, args.mem_fns);
    auto *parser = details::get(args.parser, __FJ__FWD(Args, fnargs));
    auto mem_fns = details::get(args.mem_fns, args.mem_fns = {&realloc, &free} , __FJ__FWD(Args, fnargs));

    details::init_parser(
         parser
        ,nullptr
        ,nullptr
        ,nullptr
        ,nullptr
        ,mem_fns.alloc
        ,mem_fns.free
        ,false
        ,true
        ,false
    );
}

/*************************************************************************************************/
// dyn-alloc routines

// toks_beg* - required
// toks_end* - required
// mem_fns   - optional
// despaced  - optional

// dyn-allocated parser and stack-allocated tokens
template<typename ...Args>
__FJ__FUNCTION_ENABLE(Args, args.toks_beg, args.toks_end)
(parser*) alloc_parser(Args && ...fnargs) noexcept {
    __FJ__ACCEPT_ARGS(Args, args.toks_beg, args.toks_end, args.mem_fns, args.despaced);
    auto *toks_beg = details::get(args.toks_beg, __FJ__FWD(Args, fnargs));
    auto *toks_end = details::get(args.toks_end, __FJ__FWD(Args, fnargs));
    auto   mem_fns = details::get(args.mem_fns, args.mem_fns = {&realloc, &free} ,__FJ__FWD(Args, fnargs));
    bool  despaced = details::get(args.despaced, args.despaced = false ,__FJ__FWD(Args, fnargs));

    auto *parser = static_cast<struct parser *>(mem_fns.alloc(nullptr, sizeof(struct parser)));
    if ( !parser ) {
        return nullptr;
    }

    details::init_parser(
         parser
        ,toks_beg
        ,toks_end
        ,nullptr
        ,nullptr
        ,mem_fns.alloc
        ,mem_fns.free
        ,true
        ,false
        ,despaced
    );

    return parser;
}

// init_size   - optional
// mem_fns     - optional
// despaced    - optional
// error_info* - optional
// error_code* - optional

// dyn-allocated parser and dyn-allocated tokens.
// tokens start at init_size (FJ_TOKEN_INIT_CAP without it) and grow during parse.
// parse() of a document grows them to the estimate for its size first.

template<typename ...Args>
__FJ__FUNCTION_DISABLE(Args, args.toks_beg, args.toks_end)
(parser*) alloc_parser(Args && ...fnargs) noexcept {
    __FJ__ACCEPT_ARGS(
         Args
        ,args.init_size
        ,args.mem_fns
        ,args.despaced
        ,args.error_info
        ,args.error_code
    );
    auto   mem_fns = details::get(args.mem_fns, args.mem_fns = {&realloc, &free} ,__FJ__FWD(Args, fnargs));
    bool  despaced = details::get(args.despaced, args.despaced = false ,__FJ__FWD(Args, fnargs));
    auto *error_info = details::get(args.error_info, args.error_info = nullptr ,__FJ__FWD(Args, fnargs));
    auto *error_code = details::get(args.error_code, args.error_code = nullptr ,__FJ__FWD(Args, fnargs));
    auto init_n = details::get(args.init_size, args.init_size = std::size_t{0} ,__FJ__FWD(Args, fnargs));
    if ( init_n == 0u ) {
        init_n = FJ_TOKEN_INIT_CAP;
    }

    auto *out = static_cast<struct parser *>(mem_fns.alloc(nullptr, sizeof(struct parser)));
    auto *toks_beg = out
        ? static_cast<struct token *>(mem_fns.alloc(nullptr, init_n * sizeof(struct token)))
        : nullptr
    ;
    if ( !toks_beg ) {
        if ( out ) {
            mem_fns.free(out);
        }
        if ( error_code ) {
            *error_code = FJ_CANT_ALLOCATE;
        }
        if ( error_info ) {
            *error_info = flatjson::error_info{};
            error_info->code = FJ_CANT_ALLOCATE;
            error_info->func = __func__;
            error_info->line = __LINE__;
        }

        return nullptr;
    }

    details::init_parser(
         out
        ,toks_beg
        ,toks_beg + init_n
        ,nullptr
        ,nullptr
        ,mem_fns.alloc
        ,mem_fns.free
        ,true
        ,true
        ,despaced
    );

    return out;
}

// free parser

inline void free_parser(parser *parser) noexcept {
    auto *free_fn = parser->free_fn;
    if ( parser->dyn_alloc ) {
        free_fn(parser->toks_beg);
        parser->toks_beg = nullptr;
        parser->toks_cur = nullptr;
        parser->toks_end = nullptr;
        parser->dyn_alloc = false;
    }

    auto &cl = parser->chunks;
    if ( cl.dyn ) {
        free_fn(cl.beg);
        cl = details::chunk_list{};
    }

    if ( parser->dyn_parser ) {
        free_fn(parser);
    }
}

inline void reset(parser *parser) noexcept {
    auto &rs = parser->resume;
    parser->chunks.num = 0u;
    rs.chunk_beg = nullptr;
    rs.chunk_off = 0u;
    rs.json_end = nullptr;
    rs.eos = false;
    details::reset_progress(parser);
    parser->error.code = FJ_INVALID;
}

/*************************************************************************************************/
// parser state

inline bool is_valid(const parser *parser) noexcept
{ return parser->error.code == FJ_OK; }

inline error_code get_error(const parser *parser) noexcept
{ return parser->error.code; }

inline const char* get_error_message(const parser *parser) noexcept
{ return error_message(parser->error); }

inline std::size_t num_tokens(const parser *parser) noexcept
{ return static_cast<std::size_t>(parser->toks_cur - parser->toks_beg); }

inline bool is_empty(const parser *parser) noexcept
{ return parser->toks_cur == parser->toks_beg; }

inline bool is_array(const parser *parser) noexcept
{ return parser->toks_beg->type == FJ_TYPE_ARRAY; }

inline bool is_object(const parser *parser) noexcept
{ return parser->toks_beg->type == FJ_TYPE_OBJECT; }

inline bool is_null(const parser *parser) noexcept
{ return parser->toks_beg->type == FJ_TYPE_NULL; }

inline bool is_bool(const parser *parser) noexcept
{ return parser->toks_beg->type == FJ_TYPE_BOOL; }

inline bool is_number(const parser *parser) noexcept
{ return parser->toks_beg->type == FJ_TYPE_NUMBER; }

inline bool is_string(const parser *parser) noexcept
{ return parser->toks_beg->type == FJ_TYPE_STRING; }

inline bool is_simple_type(const parser *parser) noexcept
{ return fj_is_simple_type_macro(parser->toks_beg->type); }

/*************************************************************************************************/
// parsing

// for previously allocated/inited parser

// parser*     - required
// json_beg    - optional: a new document for the parser, the previous tokens are dropped
// json_end    - optional, together with json_beg
// error_code* - optional
// error_info* - optional
// despaced    - optional
// flags       - optional (FJ_PARSE_*)

template<typename ...Args>
inline std::size_t parse(parser *parser, Args && ...fnargs) noexcept {
    __FJ__ACCEPT_ARGS(
         Args
        ,args.json_beg
        ,args.json_end
        ,args.error_code
        ,args.error_info
        ,args.despaced
        ,args.flags
    );
    __FJ__JSON_ARGS_PAIRED(Args);
    auto *error_info = details::get(args.error_info, args.error_info = nullptr, __FJ__FWD(Args, fnargs));
    auto *error_code = details::get(args.error_code, args.error_code = nullptr, __FJ__FWD(Args, fnargs));
    const auto flag_arg = details::get(args.flags, args.flags = std::uint8_t{0xff}, __FJ__FWD(Args, fnargs));
    if ( flag_arg != 0xffu ) {
        parser->resume.flags = flag_arg;
    }
    if ( details::has_arg<decltype(args.despaced), Args...>::value ) {
        parser->despaced = details::get(args.despaced, args.despaced = false ,__FJ__FWD(Args, fnargs));
    }

    auto bind_ec = FJ_OK;
    if ( details::has_arg<decltype(args.json_beg), Args...>::value ) {
        bind_ec = details::bind_json(
             parser
            ,details::get(args.json_beg, args.json_beg = nullptr, __FJ__FWD(Args, fnargs))
            ,details::get(args.json_end, args.json_end = nullptr, __FJ__FWD(Args, fnargs))
        );
    }
    if ( bind_ec == FJ_OK && parser->chunks.num == 0u ) {
        bind_ec = FJ_INVALID;
    }
    if ( bind_ec != FJ_OK ) {
        __FJ__INIT_ERROR_INFO(parser, bind_ec, __func__);
        if ( error_code ) {
            *error_code = bind_ec;
        }
        if ( error_info ) {
            *error_info = parser->error;
        }

        return 0;
    }

    const auto prev = parser->error.code;
    if ( prev == FJ_INVALID || prev == FJ_NO_FREE_TOKENS || prev == FJ_CANT_ALLOCATE
        || prev == FJ_EXTRA_DATA || prev == FJ_NO_FREE_CHUNKS )
    {
        if ( parser->chunks.num != 1u ) {
            if ( error_code ) {
                *error_code = prev;
            }
            if ( error_info ) {
                *error_info = parser->error;
            }

            return 0;
        }
        details::reset_progress(parser);
    }

    if ( !parser->resume.streaming && details::content_end(parser) == parser->resume.chunk_beg ) {
        __FJ__INIT_ERROR_INFO(parser, FJ_INVALID, __func__);

        if ( error_code ) {
            *error_code = FJ_INVALID;
        }
        if ( error_info ) {
            *error_info = parser->error;
        }

        return 0;
    }

    auto ec = details::parse_doc(parser, __func__);
    if ( ec != FJ_OK ) {
        if ( error_code ) {
            *error_code = parser->error.code;
        }
        if ( error_info ) {
            *error_info = parser->error;
        }

        return 0;
    }
    parser->error.code = ec;

    return static_cast<std::size_t>(parser->toks_cur - parser->toks_beg);
}

// for user-provided preallocated tokens

// json_beg*   - required
// json_end*   - required
// toks_beg*   - required
// toks_end*   - required
// error_code* - optional
// error_info* - optional
// despaced    - optional
// flags       - optional (FJ_PARSE_*)

template<typename ...Args>
__FJ__FUNCTION_ENABLE(Args, args.toks_beg, args.toks_end)
(std::size_t) parse(Args && ...fnargs) noexcept {
    __FJ__ACCEPT_ARGS(
         Args
        ,args.json_beg
        ,args.json_end
        ,args.toks_beg
        ,args.toks_end
        ,args.error_code
        ,args.error_info
        ,args.despaced
        ,args.flags
    );
    const char *json_beg = details::get(args.json_beg, __FJ__FWD(Args, fnargs));
    const char *json_end = details::get(args.json_end, __FJ__FWD(Args, fnargs));
    token      *toks_beg = details::get(args.toks_beg, __FJ__FWD(Args, fnargs));
    token      *toks_end = details::get(args.toks_end, __FJ__FWD(Args, fnargs));
    auto     *error_info = details::get(args.error_info, args.error_info = nullptr, __FJ__FWD(Args, fnargs));
    auto     *error_code = details::get(args.error_code, args.error_code = nullptr, __FJ__FWD(Args, fnargs));
    bool        despaced = details::get(args.despaced, args.despaced = false, __FJ__FWD(Args, fnargs));
    const auto flag_arg = details::get(args.flags, args.flags = std::uint8_t{0xff}, __FJ__FWD(Args, fnargs));

    auto parser = make_parser(
         args.toks_beg = toks_beg
        ,args.toks_end = toks_end
        ,args.despaced = despaced
    );

    return parse(
         &parser
        ,args.json_beg = json_beg
        ,args.json_end = json_end
        ,args.error_code = error_code
        ,args.error_info = error_info
        ,args.flags = flag_arg
    );
}

// for user-provided preallocated tokens

// toks_beg*   - required
// toks_end*   - required
// error_code* - optional
// error_info* - optional
// despaced    - optional
// flags       - optional (FJ_PARSE_*)

template<std::size_t N, typename ...Args>
__FJ__FUNCTION_ENABLE(Args, args.toks_beg, args.toks_end)
(std::size_t) parse(const char (&json)[N], Args && ...fnargs) noexcept {
    __FJ__ACCEPT_ARGS(
         Args
        ,args.toks_beg
        ,args.toks_end
        ,args.error_code
        ,args.error_info
        ,args.despaced
        ,args.flags
    );

    return parse(
         args.json_beg = std::begin(json)
        ,args.json_end = std::end(json)
        ,std::forward<Args>(fnargs)...
    );
}

// will allocate the parser and the tokens

// json_beg*   - required
// json_end*   - required
// error_code* - optional
// error_info* - optional
// despaced    - optional
// flags       - optional (FJ_PARSE_*)
// init_size   - optional
// mem_fns     - optional

template<typename ...Args>
__FJ__FUNCTION_DISABLE(Args, args.toks_beg, args.toks_end)
(parser*) parse(Args && ...fnargs) noexcept {
    __FJ__ACCEPT_ARGS(
         Args
        ,args.json_beg
        ,args.json_end
        ,args.error_code
        ,args.error_info
        ,args.despaced
        ,args.flags
        ,args.init_size
        ,args.mem_fns
    );
    const char *json_beg = details::get(args.json_beg, __FJ__FWD(Args, fnargs));
    const char *json_end = details::get(args.json_end, __FJ__FWD(Args, fnargs));
    auto     *error_info = details::get(args.error_info, args.error_info = nullptr, __FJ__FWD(Args, fnargs));
    auto     *error_code = details::get(args.error_code, args.error_code = nullptr, __FJ__FWD(Args, fnargs));
    bool        despaced = details::get(args.despaced, args.despaced = false, __FJ__FWD(Args, fnargs));
    const auto  flag_arg = details::get(args.flags, args.flags = std::uint8_t{0xff}, __FJ__FWD(Args, fnargs));
    auto         mem_fns = details::get(args.mem_fns, args.mem_fns = {&realloc, &free},__FJ__FWD(Args, fnargs));
    const auto    init_n = details::get(args.init_size, args.init_size = std::size_t{0},__FJ__FWD(Args, fnargs));

    auto *parser = alloc_parser(
         args.init_size = init_n ? init_n : details::estimate_tokens(json_beg, json_end)
        ,args.mem_fns = mem_fns
        ,args.error_code = error_code
        ,args.error_info = error_info
        ,args.despaced = despaced
    );
    if ( !parser ) {
        return nullptr;
    }

    parse(
         parser
        ,args.json_beg = json_beg
        ,args.json_end = json_end
        ,args.error_code = error_code
        ,args.error_info = error_info
        ,args.flags = flag_arg
    );

    return parser;
}

// will allocate the parser and the tokens

// error_code* - optional
// error_info* - optional
// despaced    - optional
// flags       - optional (FJ_PARSE_*)
// init_size   - optional
// mem_fns     - optional

template<std::size_t N, typename ...Args>
__FJ__FUNCTION_DISABLE(Args, args.toks_beg, args.toks_end)
(parser*) parse(const char (&json)[N], Args && ...fnargs) noexcept {
    __FJ__ACCEPT_ARGS(
         Args
        ,args.error_code
        ,args.error_info
        ,args.despaced
        ,args.flags
        ,args.init_size
        ,args.mem_fns
    );

    return parse(
         args.json_beg = std::begin(json)
        ,args.json_end = std::end(json)
        ,std::forward<Args>(fnargs)...
    );
}

/*************************************************************************************************/

namespace details {

inline cursor current_cursor(const parser *parser) noexcept {
    cursor out{};
    out.code = parser->error.code;
    out.next = parser->resume.json_cur;
    const char *end = details::content_end(parser);
    if ( parser->resume.json_cur && end > parser->resume.json_cur ) {
        out.remaining = static_cast<std::size_t>(end - parser->resume.json_cur);
    }

    return out;
}

/*************************************************************************************************/

} // ns details

/*************************************************************************************************/

inline cursor feed(parser *parser, const char *data, std::size_t size) noexcept {
    cursor out{};
    if ( !parser ) {
        out.code = FJ_INVALID;

        return out;
    }
    if ( !data && size != 0u ) {
        __FJ__INIT_ERROR_INFO(parser, FJ_INVALID, __func__);

        return details::current_cursor(parser);
    }

    auto &rs = parser->resume;
    const auto code = parser->error.code;
    const bool started = parser->chunks.num != 0u;
    if ( started && code != FJ_OK && code != FJ_INCOMPLETE ) {
        return details::current_cursor(parser);
    }

    if ( rs.eos || (!data && !started) ) {
        __FJ__INIT_ERROR_INFO(parser, FJ_INVALID, __func__);

        return details::current_cursor(parser);
    }

    if ( !data ) {
        rs.eos = true;
        if ( rs.lex == details::FJ_LEX_TOP_NUMBER ) {
            rs.lex = details::FJ_LEX_NONE;
            parser->error.code = FJ_OK;
        } else if ( rs.lex == details::FJ_LEX_COMMENT_SLASH ) {
            __FJ__INIT_ERROR_INFO(parser, FJ_INVALID, __func__);
        }

        return details::current_cursor(parser);
    }
    if ( size == 0u ) {
        return details::current_cursor(parser);
    }

    if ( !started ) {
        details::reset_progress(parser);
    }

    const auto add_ec = details::add_chunk(parser, data, size);
    if ( add_ec != FJ_OK ) {
        __FJ__INIT_ERROR_INFO(parser, add_ec, __func__);
        out.code = add_ec;
        out.next = data;
        out.remaining = size;

        return out;
    }
    rs.streaming = true;
    details::parse_doc(parser, __func__);

    return details::current_cursor(parser);
}

/*************************************************************************************************/

} // ns flatjson

#endif // __FLATJSON__PARSE_HPP
