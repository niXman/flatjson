// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__ITERATOR_HPP
#define __FLATJSON__ITERATOR_HPP

#include "chunks.hpp"
#include "conv.hpp"

#include <string>

#include <cassert>
#include <cstring>

namespace flatjson {

/*************************************************************************************************/

inline const char* type_name(std::uint8_t t) noexcept {
    static const char *strs[] = {
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

/*************************************************************************************************/

struct iterator {
    const token *toks_beg;
    const token *toks_end;
    const details::chunk_list *chunks;
    token *beg;
    token *cur;
    token *end;

    string_view key() const noexcept {
        if ( cur > toks_beg ) {
            auto *parent_tok = toks_beg + cur->parent_idx;
            if ( parent_tok->type == FJ_TYPE_OBJECT ) {
                auto *prev = cur - 1;
                if ( prev >= toks_beg
                     && prev->type == FJ_TYPE_STRING
                     && prev->parent_idx == cur->parent_idx )
                {
                    return string_view{details::text_at(*chunks, prev->vpos), prev->vlen};
                }
            }
        }
        return {};
    }
    string_view value() const noexcept {
        return fj_is_simple_type_macro(cur->type)
            ? string_view{details::text_at(*chunks, cur->vpos), cur->vlen}
            : string_view{}
        ;
    }

    const token* parent() const noexcept { return toks_beg + cur->parent_idx; }

    token_type type() const noexcept { return (token_type)cur->type; }
    const char* type_name() const noexcept { return flatjson::type_name(type()); }
    bool is_valid() const noexcept { return cur && type() != FJ_TYPE_INVALID; }
    bool is_array() const noexcept { return type() == FJ_TYPE_ARRAY; }
    bool is_object() const noexcept { return type() == FJ_TYPE_OBJECT; }
    bool is_null() const noexcept { return type() == FJ_TYPE_NULL; }
    bool is_bool() const noexcept { return type() == FJ_TYPE_BOOL; }
    bool is_number() const noexcept { return type() == FJ_TYPE_NUMBER; }
    bool is_string() const noexcept { return type() == FJ_TYPE_STRING; }
    bool is_simple_type() const noexcept { return fj_is_simple_type_macro(type()); }

    string_view to_string_view() const noexcept { return value(); }
    std::string to_string() const noexcept
    { auto s = to_string_view(); return {s.data(), s.size()}; }

    template<typename T>
    T to() const noexcept {
        auto s = to_string_view();

        return details::conv_to(s.data(), s.size(), T{});
    }
    bool to_bool() const noexcept { return to<bool>(); }
    std::uint32_t to_uint() const noexcept { return to<std::uint32_t>(); }
    std::int32_t to_int() const noexcept { return to<std::int32_t>(); }
    std::uint64_t to_uint64() const noexcept { return to<std::uint64_t>(); }
    std::int64_t to_int64() const noexcept { return to<std::int64_t>(); }
    double to_double() const noexcept { return to<double>(); }
    float to_float() const noexcept { return to<float>(); }
};

/*************************************************************************************************/

inline iterator begin(const parser *parser) noexcept {
    auto *beg_tok = parser->toks_beg;
    auto *end_tok = parser->toks_cur;
    auto *range_end = (beg_tok && beg_tok < parser->toks_cur && !fj_is_simple_type_macro(beg_tok->type))
        ? beg_tok + beg_tok->end_idx
        : end_tok
    ;
    return {
         beg_tok
        ,end_tok
        ,&parser->chunks
        ,beg_tok
        ,beg_tok
        ,range_end
    };
}

inline iterator end(const parser *parser) noexcept {
    auto *beg_tok = parser->toks_beg;
    auto *end_tok = parser->toks_cur;
    auto *range_end = (beg_tok && beg_tok < parser->toks_cur && !fj_is_simple_type_macro(beg_tok->type))
        ? beg_tok + beg_tok->end_idx
        : end_tok
    ;
    return {
         beg_tok
        ,end_tok
        ,&parser->chunks
        ,range_end
        ,range_end
        ,range_end
    };
}

inline iterator begin(const iterator &it) noexcept {
    if ( it.is_simple_type() ) {
        auto *parent = it.toks_beg + it.cur->parent_idx;
        auto *end = it.toks_beg + parent->end_idx;
        return {it.toks_beg, it.toks_end, it.chunks, it.cur, it.cur, (token *)end};
    }
    auto *end = it.toks_beg + it.cur->end_idx;

    return {it.toks_beg, it.toks_end, it.chunks, it.cur, it.cur, (token *)end};
}

inline iterator end(const iterator &it) noexcept {
    if ( it.is_simple_type() ) {
        return {it.toks_beg, it.toks_end, it.chunks, it.end, it.end, it.end};
    }
    auto *end_tok = it.toks_beg + it.cur->end_idx;

    return {it.toks_beg, it.toks_end, it.chunks, (token *)end_tok, (token *)end_tok, (token *)end_tok};
}

inline bool equal(const iterator &l, const iterator &r) noexcept
{ return l.cur == r.cur; }

inline bool not_equal(const iterator &l, const iterator &r) noexcept
{ return !equal(l, r); }

inline iterator next(const iterator &it) noexcept {
    token *nxt;

    if ( it.cur == it.beg ) {
        nxt = it.cur + 1;
    } else if ( !it.is_simple_type() ) {
        nxt = const_cast<token *>(it.toks_beg) + it.cur->end_idx + 1;
    } else {
        nxt = it.cur + 1;
    }

    if ( nxt >= it.toks_end ) {
        return {it.toks_beg, it.toks_end, it.chunks, it.beg, const_cast<token *>(it.toks_end), it.end};
    }

    if ( nxt->type == FJ_TYPE_STRING ) {
        auto *parent_tok = it.toks_beg + nxt->parent_idx;
        if ( parent_tok->type == FJ_TYPE_OBJECT ) {
            ++nxt;
        }
    }

    return {it.toks_beg, it.toks_end, it.chunks, it.beg, nxt, it.end};
}

inline std::size_t distance(const iterator &from, const iterator &to) noexcept {
    assert(from.cur->parent_idx == to.cur->parent_idx);

    auto *parent = from.toks_beg + from.cur->parent_idx;
    if ( parent->primitive_only == 1 ) {
        if ( parent->type == FJ_TYPE_ARRAY ) {
            return to.cur - from.cur;
        } else {
            return (to.cur - from.cur) / 2;
        }
    }

    std::size_t cnt = 0;
    iterator it{from};
    for ( ; not_equal(it, to); it = next(it), ++cnt )
    {}

    return cnt;
}

// returns the number members for ARRAYs and OBJECTs.
// for ARRAYS returns the number of a direct children.
// for OBJECTS returns the number of keys.
inline std::size_t members(const iterator &it) noexcept {
    assert(it.beg->type == FJ_TYPE_ARRAY
           || it.beg->type == FJ_TYPE_OBJECT
    );

    std::size_t cnt = 0;
    iterator e = end(it);
    for ( auto tmp = next(it); not_equal(tmp, e); tmp = next(tmp), ++cnt )
    {}

    return cnt;
}

/*************************************************************************************************/

namespace details {

// find by key name
inline iterator find(
     const iterator &beg
    ,const iterator &end
    ,const char *key
    ,std::size_t klen) noexcept
{
    iterator it{beg};
    while ( not_equal(it, end) ) {
        if ( it.type() == FJ_TYPE_OBJECT_END ) {
            return end;
        }
        const auto sv = it.key();
        if ( sv.size() == klen && sv == string_view{key, klen} ) {
            return it;
        }

        it = next(it);
    }

    return end;
}

} // ns details

/*************************************************************************************************/

// at by key, from the parser
inline iterator at(const parser *parser, const char *key, std::size_t klen) noexcept {
    auto beg = begin(parser);
    auto end_it = end(parser);
    auto first = next(beg);
    auto found = details::find(first, end_it, key, klen);

    return not_equal(found, end_it) ? begin(found) : end_it;
}

template<typename T, typename = typename details::enable_if_const_char_ptr<T>::type>
iterator at(const parser *parser, T key) noexcept
{ return at(parser, key, std::strlen(key)); }

template<std::size_t N>
iterator at(const parser *parser, const char (&key)[N]) noexcept
{ return at(parser, key, N-1); }

// at by key, from the iterators
inline iterator at(const iterator &it, const char *key, std::size_t klen) noexcept {
    auto beg = begin(it);
    auto end_it = end(it);
    auto first = next(beg);
    auto found = details::find(first, end_it, key, klen);

    return not_equal(found, end_it) ? begin(found) : end_it;
}

template<typename T, typename = typename details::enable_if_const_char_ptr<T>::type>
iterator at(const iterator &it, T key) noexcept
{ return at(it, key, std::strlen(key)); }

template<std::size_t N>
iterator at(const iterator &it, const char (&key)[N]) noexcept
{ return at(it, key, N-1); }

/*************************************************************************************************/

namespace details {

// find by index for ARRAY type only
inline iterator find(const iterator &beg, const iterator &end, std::size_t idx) noexcept {
    assert(beg.beg->type == FJ_TYPE_ARRAY);

    if ( beg.beg->primitive_only ) {
        const auto *e = beg.toks_beg + beg.beg->end_idx;
        const auto  n = e - beg.cur;
        assert(n >= 0);
        if ( idx < static_cast<std::size_t>(n) ) {
            return {
                 beg.toks_beg
                ,beg.toks_end
                ,beg.chunks
                ,beg.beg
                ,beg.cur + idx
                ,beg.end
            };
        }

        return end;
    } else {
        auto it{beg};
        for ( ; not_equal(it, end) && idx ; --idx ) {
            if ( it.type() == FJ_TYPE_ARRAY_END ) {
                return end;
            }

            it = it.is_simple_type()
                ? iterator{it.toks_beg, it.toks_end, it.chunks, it.beg, it.cur + 1, it.end}
                : iterator{
                     it.toks_beg
                    ,it.toks_end
                    ,it.chunks
                    ,it.beg
                    ,const_cast<token *>(it.toks_beg) + it.cur->end_idx + 1
                    ,it.end
                }
            ;
        }

        if ( not_equal(it, end) ) {
            return it;
        }

        return end;
    }
}

} // ns details

/*************************************************************************************************/

// at by index, from a parser
inline iterator at(const parser *p, std::size_t idx) noexcept {
    bool is_simple = fj_is_simple_type_macro(p->toks_beg->type);
    const iterator beg = {
         p->toks_beg
        ,p->toks_cur
        ,&p->chunks
        ,p->toks_beg
        ,p->toks_beg + static_cast<std::size_t>(!is_simple)
        ,p->toks_cur
    };
    const auto end = ::flatjson::end(p);
    auto found = details::find(beg, end, idx);

    return not_equal(found, end) ? begin(found) : end;
}

// at by index, from a iterator
inline iterator at(const iterator &it, std::size_t idx) noexcept {
    const iterator beg = {
         it.toks_beg
        ,it.toks_end
        ,it.chunks
        ,it.beg
        ,it.cur + static_cast<std::size_t>(!it.is_simple_type())
        ,it.end
    };
    const auto end = flatjson::end(it);
    auto found = details::find(beg, end, idx);

    return not_equal(found, end) ? begin(found) : end;
}

/*************************************************************************************************/

} // ns flatjson

#endif // __FLATJSON__ITERATOR_HPP
