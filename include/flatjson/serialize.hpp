
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__SERIALIZE_HPP
#define __FLATJSON__SERIALIZE_HPP

#include "chunks.hpp"
#include "iterator.hpp"

#include <algorithm>
#include <string>
#include <type_traits>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace flatjson {

struct fjson;

/*************************************************************************************************/

namespace details {

static constexpr std::size_t k_indent_step_len = 3u;

template<typename T>
struct is_serializable: std::integral_constant<
     bool
    ,std::is_same<T, iterator>::value
        || std::is_same<T, fjson>::value
        || std::is_same<T, parser *>::value
        || std::is_same<T, const parser *>::value
> {};

struct tree {
    const token *first;
    const token *last;
    const chunk_list *chunks;
};

inline tree make_tree(
     const token *toks_beg
    ,const token *root
    ,const token *toks_end
    ,const chunk_list *chunks) noexcept
{
    if ( !root || root < toks_beg || root >= toks_end ) {
        return {nullptr, nullptr, nullptr};
    }
    switch ( root->type ) {
        case FJ_TYPE_STRING:
        case FJ_TYPE_NUMBER:
        case FJ_TYPE_BOOL:
        case FJ_TYPE_NULL: return {root, root, chunks};

        case FJ_TYPE_OBJECT:
        case FJ_TYPE_ARRAY: {
            const token *last = toks_beg + root->end_idx;
            if ( last <= root || last >= toks_end ) {
                return {nullptr, nullptr, nullptr};
            }

            return {root, last, chunks};
        }

        default: return {nullptr, nullptr, nullptr};
    }
}

template<typename It>
typename std::enable_if<std::is_same<It, iterator>::value, tree>::type
tree_of(const It &it) noexcept
{ return make_tree(it.toks_beg, it.cur, it.toks_end, it.chunks); }

inline tree tree_of(const parser *p) noexcept {
    if ( !p || p->error.code != FJ_OK ) {
        return {nullptr, nullptr, nullptr};
    }

    return make_tree(
         p->toks_beg
        ,p->toks_beg
        ,p->toks_cur
        ,&p->chunks
    );
}

template<typename J>
typename std::enable_if<std::is_same<J, fjson>::value, tree>::type
tree_of(const J &j) noexcept {
    if ( !j.is_valid() ) {
        return {nullptr, nullptr, nullptr};
    }

    return tree_of(*j.begin());
}

inline bool is_inf_nan(const char *text, std::size_t len) noexcept
{ return text[0] == 'N' || text[0] == 'I' || (len > 1u && text[0] == '-' && text[1] == 'I'); }

struct counting_sink {
    std::size_t size;

    void put(char) noexcept { ++size; }
    void put(const char *, std::size_t len) noexcept { size += len; }
    void fill(char, std::size_t count) noexcept { size += count; }
    void atom() noexcept {}
    bool good() const noexcept { return true; }
};

struct buffer_sink {
    char *ptr;

    void put(char c) noexcept { *ptr++ = c; }
    void put(const char *data, std::size_t len) noexcept {
        ::memcpy(ptr, data, len);
        ptr += len;
    }
    void fill(char c, std::size_t count) noexcept {
        ::memset(ptr, c, count);
        ptr += count;
    }
    void atom() noexcept {}
    bool good() const noexcept { return true; }
};

using flush_fnptr = bool(*)(void *ctx, const void *data, std::size_t size);

struct chunked_sink {
    char *buf;
    std::size_t capacity;
    std::size_t pos;
    std::size_t mark;
    flush_fnptr flush_fn;
    void *ctx;
    bool ok;

    void put(char c) {
        if ( pos == capacity && !make_room() ) {
            return;
        }
        buf[pos++] = c;
    }
    void put(const char *data, std::size_t len) {
        while ( len != 0u && (pos != capacity || make_room()) ) {
            const auto n = std::min(len, capacity - pos);
            std::memcpy(buf + pos, data, n);
            pos += n;
            data += n;
            len -= n;
        }
    }
    void fill(char c, std::size_t count) {
        while ( count != 0u && (pos != capacity || make_room()) ) {
            const auto n = std::min(count, capacity - pos);
            std::memset(buf + pos, c, n);
            pos += n;
            count -= n;
        }
    }
    void atom() noexcept { mark = pos; }
    bool good() const noexcept { return ok; }

    bool make_room() {
        if ( !ok ) {
            return false;
        }
        if ( mark == 0u ) {
            ok = flush_fn(ctx, buf, pos);
            pos = 0u;
        } else {
            ok = flush_fn(ctx, buf, mark);
            pos -= mark;
            std::memmove(buf, buf + mark, pos);
            mark = 0u;
        }

        return ok;
    }
    bool finish() {
        if ( ok && pos != 0u ) {
            ok = flush_fn(ctx, buf, pos);
            pos = 0u;
        }

        return ok;
    }
};

template<typename Sink>
void write_leaf(Sink &s, const token *tok, const chunk_list &chunks) {
    const char *text = text_at(chunks, tok->vpos);
    const std::size_t len = tok->vlen;
    if ( tok->type == FJ_TYPE_STRING ) {
        s.put('"');
        s.put(text, len);
        s.put('"');
    } else if ( tok->type == FJ_TYPE_NUMBER && is_inf_nan(text, len) ) {
        s.put("null", 4u);
    } else {
        s.put(text, len);
    }
}

template<bool Pretty, typename Sink>
bool write_tree_as(Sink &s, const tree &t) {
    struct level {
        bool object;
        std::uint32_t items;
    };
    level stack[k_parse_stack_max];

    std::size_t depth = 0u;
    for ( const token *tok = t.first; tok <= t.last && s.good(); ++tok ) {
        const auto type = tok->type;
        if ( type == FJ_TYPE_OBJECT_END || type == FJ_TYPE_ARRAY_END ) {
            if ( depth == 0u ) {
                return false;
            }
            --depth;
            if ( Pretty && stack[depth].items != 0u ) {
                s.put('\n');
                s.fill(' ', depth * k_indent_step_len);
            }
            s.put(type == FJ_TYPE_OBJECT_END ? '}' : ']');
            continue;
        }
        if ( depth != 0u ) {
            level &lv = stack[depth - 1u];
            if ( lv.object && (lv.items & 1u) != 0u ) {
                s.put(':');
                if ( Pretty ) {
                    s.put(' ');
                }
            } else {
                if ( lv.items != 0u ) {
                    s.put(',');
                }
                if ( Pretty ) {
                    s.put('\n');
                    s.fill(' ', depth * k_indent_step_len);
                }
            }
            ++lv.items;
        }
        if ( type == FJ_TYPE_OBJECT || type == FJ_TYPE_ARRAY ) {
            if ( depth == k_parse_stack_max ) {
                return false;
            }
            stack[depth++] = level{type == FJ_TYPE_OBJECT, 0u};
            s.put(type == FJ_TYPE_OBJECT ? '{' : '[');
        } else {
            s.atom();
            write_leaf(s, tok, *t.chunks);
        }
    }

    return s.good() && depth == 0u;
}

template<typename Sink>
bool write_tree(Sink &s, const tree &t, bool pretty) {
    if ( !t.first ) {
        return false;
    }

    return pretty ? write_tree_as<true>(s, t) : write_tree_as<false>(s, t);
}

} // ns details

/*************************************************************************************************/

template<typename T>
std::size_t required_bytes(const T &obj, bool pretty = false) noexcept {
    static_assert(details::is_serializable<T>::value, "T must be iterator, fjson or parser*");
    details::counting_sink s{0u};

    return details::write_tree(s, details::tree_of(obj), pretty) ? s.size : 0u;
}

template<typename T>
char* to_buffer(char *ptr, const T &obj, bool pretty = false) noexcept {
    static_assert(details::is_serializable<T>::value, "T must be iterator, fjson or parser*");
    details::buffer_sink s{ptr};

    return details::write_tree(s, details::tree_of(obj), pretty) ? s.ptr : ptr;
}

template<typename T>
std::string to_string(const T &obj, bool pretty = false) {
    static_assert(details::is_serializable<T>::value, "T must be iterator, fjson or parser*");

    std::string result(required_bytes(obj, pretty), '\0');
    if ( !result.empty() ) {
        const char *end = to_buffer(&result[0], obj, pretty);
        result.resize(static_cast<std::size_t>(end - result.data()));
    }

    return result;
}

template<
     typename T
    ,typename Fn
    ,typename AllocFn = alloc_fnptr
    ,typename FreeFn = free_fnptr
>
bool to_chunked_buffer(
     char *buf
    ,std::size_t buf_size
    ,const T &obj
    ,Fn &&fn
    ,bool pretty = false
    ,AllocFn alloc_fn = &std::realloc
    ,FreeFn free_fn = &std::free
    ,bool owned = false)
{
    static_assert(details::is_serializable<T>::value, "T must be iterator, fjson or parser*");
    assert(!buf == owned && "buf == nullptr requires owned == true, and the other way round");

    const auto t = details::tree_of(obj);
    if ( !t.first || buf_size == 0u ) {
        return false;
    }
    if ( owned ) {
        buf = static_cast<char *>(alloc_fn(nullptr, buf_size));
        if ( !buf ) {
            return false;
        }
    }
    struct buffer_guard {
        FreeFn free_fn;
        char *ptr;

        ~buffer_guard() {
            if ( ptr ) {
                free_fn(ptr);
            }
        }
    } guard{free_fn, owned ? buf : nullptr};

    using fn_type = typename std::remove_reference<Fn>::type;
    details::chunked_sink s{
         buf
        ,buf_size
        ,0u
        ,0u
        ,[](void *ctx, const void *data, std::size_t size) -> bool
        { return (*static_cast<fn_type *>(ctx))(data, size); }
        ,const_cast<void *>(static_cast<const void *>(&fn))
        ,true
    };

    return details::write_tree(s, t, pretty) && s.finish();
}

/*************************************************************************************************/

} // ns flatjson

#endif // __FLATJSON__SERIALIZE_HPP
