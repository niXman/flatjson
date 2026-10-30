
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__STATE_HPP
#define __FLATJSON__STATE_HPP

#include "error.hpp"
#include "types.hpp"
#include "chunks.hpp"

#include <limits>

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable: 4127)
#endif

namespace flatjson {

/*************************************************************************************************/
// pack/unpack the internal representation for pass to another node/process

// layout, little-endian: [u32 magic][u32 json_len][u32 toks_num][u32 events_size][json][events]
// "FJS0": the minified text.
// event: [u8 code:3, len:5][varint len - 31]?
//         len - for strings and numbers only, the separators between values are implied.
// a close event ends a container. parent_idx, end_idx, primitive_only, the container lengths
// and the end tokens are rebuilt on unpacking.

namespace details {

constexpr std::uint32_t k_packed_magic = 0x30534A46u; // "FJS0"
constexpr std::size_t k_packed_header_size = sizeof(std::uint32_t) * 4u;
constexpr std::uint32_t k_packed_inline_max = 31u;
constexpr std::uint32_t k_packed_field_max = std::uint32_t{1} << 30;
constexpr std::size_t k_packed_event_max = 6u;

enum pack_event: std::uint8_t {
     FJ_EV_STRING
    ,FJ_EV_NUMBER
    ,FJ_EV_TRUE
    ,FJ_EV_FALSE
    ,FJ_EV_NULL
    ,FJ_EV_OBJECT
    ,FJ_EV_ARRAY
    ,FJ_EV_CLOSE
};

inline std::uint32_t event_len(std::uint32_t ev) noexcept {
    static const std::uint8_t lens[8] = {0u, 0u, 4u, 5u, 4u, 1u, 1u, 1u};

    return lens[ev];
}

inline std::uint32_t event_type(std::uint32_t ev) noexcept {
    static const std::uint8_t types[8] = {
         FJ_TYPE_STRING
        ,FJ_TYPE_NUMBER
        ,FJ_TYPE_BOOL
        ,FJ_TYPE_BOOL
        ,FJ_TYPE_NULL
        ,FJ_TYPE_OBJECT
        ,FJ_TYPE_ARRAY
        ,FJ_TYPE_INVALID
    };

    return types[ev];
}

template<bool Write>
struct pack_sink {
    char *ptr;
    std::size_t size;

    void byte(std::uint32_t c) noexcept {
        if ( Write ) {
            *ptr++ = static_cast<char>(c);
        }
        ++size;
    }
    void bytes(const char *src, std::size_t n) noexcept {
        if ( Write && n != 0u ) {
            std::memcpy(ptr, src, n);
            ptr += n;
        }
        size += n;
    }
    void varint(std::uint32_t v) noexcept {
        while ( v >= 0x80u ) {
            byte((v & 0x7fu) | 0x80u);
            v >>= 7u;
        }
        byte(v);
    }
    void head(std::uint32_t ev, std::uint32_t v) noexcept {
        if ( v < k_packed_inline_max ) {
            byte(ev | (v << 3u));
        } else {
            byte(ev | (k_packed_inline_max << 3u));
            varint(v - k_packed_inline_max);
        }
    }
};

inline bool get_varint(
     const std::uint8_t *&src
    ,const std::uint8_t *end
    ,std::uint32_t &out) noexcept
{
    std::uint32_t v = 0u;
    for ( std::uint32_t shift = 0u; shift < 35u && src < end; shift += 7u ) {
        const std::uint32_t b = *src++;
        v |= (b & 0x7fu) << shift;
        if ( b < 0x80u ) {
            out = v;

            return true;
        }
    }

    return false;
}

inline bool get_head_value(
     const std::uint8_t *&src
    ,const std::uint8_t *end
    ,std::uint32_t head
    ,std::uint32_t &out) noexcept
{
    out = head >> 3u;
    if ( out != k_packed_inline_max ) {
        return true;
    }
    std::uint32_t more{};
    if ( !get_varint(src, end, more) || more >= k_packed_field_max ) {
        return false;
    }
    out += more;

    return true;
}

inline void set_token(
     token &t
    ,std::uint32_t type
    ,std::uint64_t vpos
    ,std::uint32_t vlen
    ,std::uint32_t parent) noexcept
{
    t = token{};
    t.type = type;
    t.vpos = static_cast<std::uint32_t>(vpos);
    t.vlen = vlen;
    t.parent_idx = parent;
    t.end_idx = 0u;
    t.primitive_only = 0u;
    t.unused = 0u;
}

inline std::uint32_t document_size(const chunk_list &cl) noexcept {
    const chunk &last = cl.num > 1u ? cl.beg[cl.num - 1u] : cl.first;

    return cl.num ? last.offset + last.size : 0u;
}

template<bool Write>
std::size_t encode(
     char *json_dst
    ,char *events_dst
    ,std::uint32_t &json_len
    ,const token *toks
    ,std::size_t num
    ,const chunk_list &cl) noexcept
{
    pack_sink<Write> text{json_dst, 0u};
    pack_sink<Write> out{events_dst, 0u};

    bool is_object[k_parse_stack_max];
    std::uint32_t children[k_parse_stack_max];
    std::uint32_t depth = 0u;
    for ( std::size_t i = 0; i < num; ++i ) {
        const token &t = toks[i];
        if ( t.type == FJ_TYPE_OBJECT_END || t.type == FJ_TYPE_ARRAY_END ) {
            --depth;
            text.byte(t.type == FJ_TYPE_OBJECT_END ? '}' : ']');
            out.byte(FJ_EV_CLOSE);
            continue;
        }
        if ( depth ) {
            auto &n = children[depth - 1u];
            if ( n ) {
                text.byte(is_object[depth - 1u] && (n & 1u) ? ':' : ',');
            }
            ++n;
        }
        const char *src = Write && t.vlen ? text_at(cl, t.vpos) : nullptr;
        switch ( t.type ) {
            case FJ_TYPE_STRING: {
                text.byte('"');
                text.bytes(src, t.vlen);
                text.byte('"');
                out.head(FJ_EV_STRING, t.vlen);
                break;
            }
            case FJ_TYPE_NUMBER: {
                text.bytes(src, t.vlen);
                out.head(FJ_EV_NUMBER, t.vlen);
                break;
            }
            case FJ_TYPE_BOOL: {
                text.bytes(src, t.vlen);
                out.byte(t.vlen == 4u ? FJ_EV_TRUE : FJ_EV_FALSE);
                break;
            }
            case FJ_TYPE_NULL: {
                text.bytes(src, t.vlen);
                out.byte(FJ_EV_NULL);
                break;
            }
            default: {
                const bool obj = t.type == FJ_TYPE_OBJECT;
                text.byte(obj ? '{' : '[');
                out.byte(obj ? FJ_EV_OBJECT : FJ_EV_ARRAY);
                is_object[depth] = obj;
                children[depth] = 0u;
                ++depth;
            }
        }
    }
    json_len = static_cast<std::uint32_t>(text.size);

    return out.size;
}

struct unpack_frame {
    std::uint32_t idx;
    std::uint32_t children;
    bool primitive_only;
    bool is_object;
};

inline std::uint32_t parent_of(const unpack_frame *stack, std::uint32_t depth) noexcept
{ return depth ? stack[depth - 1u].idx : 0u; }

inline void close_container(
     token *toks
    ,std::uint32_t idx
    ,const unpack_frame &f
    ,std::uint64_t close_pos) noexcept
{
    token &open = toks[f.idx];
    open.end_idx = idx;
    open.vlen = static_cast<std::uint32_t>(close_pos + 1u - open.vpos);
    open.primitive_only = f.primitive_only;
    const std::uint32_t type = open.type == FJ_TYPE_OBJECT ? FJ_TYPE_OBJECT_END : FJ_TYPE_ARRAY_END;
    set_token(toks[idx], type, 0u, 0u, f.idx);
}

inline bool decode(
     token *toks
    ,std::uint32_t num
    ,const std::uint8_t *src
    ,const std::uint8_t *end
    ,std::uint32_t json_len) noexcept
{
    unpack_frame stack[k_parse_stack_max];
    std::uint32_t depth = 0u;
    std::uint32_t i = 0u;
    std::uint64_t cur = 0u;
    for ( ; src < end; ++i ) {
        if ( i == num || (depth == 0u && i != 0u) ) {
            return false;
        }
        const std::uint32_t head = *src++;
        const std::uint32_t ev = head & 7u;
        if ( ev != FJ_EV_CLOSE && depth ) {
            unpack_frame &top = stack[depth - 1u];
            if ( top.is_object && (top.children & 1u) == 0u && ev != FJ_EV_STRING ) {
                return false;
            }
            cur += top.children != 0u;
            ++top.children;
        }
        if ( ev <= FJ_EV_NUMBER ) {
            std::uint32_t len{};
            if ( !get_head_value(src, end, head, len) ) {
                return false;
            }
            const std::uint64_t pos = cur + (ev == FJ_EV_STRING);
            set_token(toks[i], event_type(ev), pos, len, parent_of(stack, depth));
            cur = pos + len + (ev == FJ_EV_STRING);
        } else if ( (head >> 3u) != 0u ) {
            return false;
        } else if ( ev <= FJ_EV_NULL ) {
            set_token(toks[i], event_type(ev), cur, event_len(ev), parent_of(stack, depth));
            cur += event_len(ev);
        } else if ( ev != FJ_EV_CLOSE ) {
            if ( depth == k_parse_stack_max ) {
                return false;
            }
            set_token(toks[i], event_type(ev), cur, 0u, parent_of(stack, depth));
            if ( depth ) {
                stack[depth - 1u].primitive_only = false;
            }
            stack[depth++] = unpack_frame{i, 0u, true, ev == FJ_EV_OBJECT};
            cur += 1u;
        } else {
            if ( depth == 0u ) {
                return false;
            }
            const unpack_frame &f = stack[--depth];
            if ( f.is_object && (f.children & 1u) != 0u ) {
                return false;
            }
            close_container(toks, i, f, cur);
            cur += 1u;
        }
        if ( cur > json_len ) {
            return false;
        }
    }

    return i == num && depth == 0u && cur == json_len;
}

inline bool packed_layout(
     const parser *parser
    ,std::uint32_t &json_len
    ,std::size_t &events_size) noexcept
{
    if ( !parser || parser->error.code != FJ_OK || parser->toks_cur == parser->toks_beg ) {
        return false;
    }
    const auto *toks = parser->toks_beg;
    const auto num = static_cast<std::size_t>(parser->toks_cur - parser->toks_beg);
    events_size = encode<false>(
         nullptr
        ,nullptr
        ,json_len
        ,toks
        ,num
        ,parser->chunks
    );

    return true;
}

} // ns details

/*************************************************************************************************/

inline std::size_t packed_state_size(const parser *parser) noexcept {
    std::uint32_t json_len{};
    std::size_t events_size{};
    if ( !details::packed_layout(parser, json_len, events_size) ) {
        return 0u;
    }

    return details::k_packed_header_size + json_len + events_size;
}

inline std::size_t packed_state_size_max(const parser *parser) noexcept {
    if ( !parser || parser->error.code != FJ_OK || parser->toks_cur == parser->toks_beg ) {
        return 0u;
    }
    const auto ntoks = static_cast<std::size_t>(parser->toks_cur - parser->toks_beg);
    const std::uint64_t bound = details::k_packed_header_size
        + std::uint64_t{details::document_size(parser->chunks)}
        + details::k_packed_event_max * ntoks
    ;

    return bound > std::numeric_limits<std::size_t>::max()
        ? std::numeric_limits<std::size_t>::max()
        : static_cast<std::size_t>(bound)
    ;
}

/*************************************************************************************************/

inline std::size_t pack_state(void *dst, std::size_t size, const parser *parser) noexcept {
    const std::size_t bound = packed_state_size_max(parser);
    if ( !dst || bound == 0u ) {
        return 0u;
    }
    const auto *toks = parser->toks_beg;
    const auto num = static_cast<std::size_t>(parser->toks_cur - parser->toks_beg);
    const auto &cl = parser->chunks;
    const std::uint32_t doc_size = details::document_size(cl);

    std::uint32_t json_len = doc_size;
    std::size_t events_size{};
    const bool one_pass = size >= bound;
    if ( !one_pass ) {
        details::packed_layout(parser, json_len, events_size);
        if ( size < details::k_packed_header_size + json_len + events_size ) {
            return 0u;
        }
    }

    char *json = static_cast<char *>(dst) + details::k_packed_header_size;
    char *events = json + (one_pass ? doc_size : json_len);
    events_size = details::encode<true>(json, events, json_len, toks, num, cl);
    if ( one_pass ) {
        std::memmove(json + json_len, events, events_size);
    }

    const std::uint32_t header[4] = {
         details::k_packed_magic
        ,json_len
        ,static_cast<std::uint32_t>(num)
        ,static_cast<std::uint32_t>(events_size)
    };
    std::memcpy(dst, header, sizeof(header));

    return details::k_packed_header_size + json_len + events_size;
}

/*************************************************************************************************/

inline bool unpack_state(parser *parser, char *ptr, std::size_t size) noexcept {
    if ( !parser || !ptr || size < details::k_packed_header_size ) {
        return false;
    }

    std::uint32_t header[4]{};
    std::memcpy(header, ptr, sizeof(header));
    const std::uint32_t magic = header[0];
    const std::uint32_t json_len = header[1];
    const std::uint32_t toks_num = header[2];
    const std::uint32_t events_size = header[3];
    if ( magic != details::k_packed_magic ) {
        return false;
    }
    if ( json_len >= details::k_packed_field_max || toks_num >= details::k_packed_field_max
        || toks_num == 0u
        || details::k_packed_header_size + std::uint64_t{json_len} + events_size != size )
    {
        return false;
    }

    auto *toks = parser->toks_beg;
    const auto toks_cap = toks
        ? static_cast<std::size_t>(parser->toks_end - toks)
        : std::size_t{0}
    ;
    if ( toks_num > toks_cap ) {
        auto *alloc_fn = parser->alloc_fn;
        if ( !parser->dyn_alloc || !alloc_fn ) {
            return false;
        }
        auto *mem = static_cast<token *>(alloc_fn(toks, toks_num * sizeof(token)));
        if ( !mem ) {
            return false;
        }
        toks = mem;
        parser->toks_beg = mem;
        parser->toks_end = mem + toks_num;
    }

    const char *json_beg = ptr + details::k_packed_header_size;
    const auto *events = reinterpret_cast<const std::uint8_t *>(json_beg + json_len);
    const bool ok = details::decode(
         toks
        ,toks_num
        ,events
        ,events + events_size
        ,json_len
    );
    if ( !ok ) {
        parser->toks_cur = toks;
        parser->error.code = FJ_INVALID;

        return false;
    }

    auto &cl = parser->chunks;
    cl.first = chunk{json_beg, json_len, 0u};
    cl.num = 1u;
    parser->resume.chunk_beg = json_beg;
    parser->resume.chunk_off = 0u;
    parser->resume.json_cur = json_beg + json_len;
    parser->resume.json_end = json_beg + json_len;
    parser->toks_cur = toks + toks_num;
    parser->error.code = FJ_OK;

    return true;
}

/*************************************************************************************************/

} // ns flatjson

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif // __FLATJSON__STATE_HPP
