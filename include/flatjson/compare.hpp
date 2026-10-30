// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__COMPARE_HPP
#define __FLATJSON__COMPARE_HPP

#include "stringview.hpp"
#include "types.hpp"
#include "chunks.hpp"
#include "iterator.hpp"

#include <cstddef>
#include <cstring>

namespace flatjson {

/*************************************************************************************************/

enum class compare_mode {
     markup_only // structure and key names
    ,length_only // markup_only, plus the length of each value
    ,full        // length_only, plus the bytes of each value
};

enum class compare_result {
     equal
    ,type    // differs in token type.
    ,key     // differs in key. (for objects only)
    ,no_key  // a key of the left document is absent from the right one
    ,length  // differs in length.
    ,value   // differs in values.
    ,longer  // the right document has more children here
    ,shorter // the right document has fewer children here
};

inline const char* compare_result_string(compare_result result) noexcept {
    switch ( result ) {
        case compare_result::equal: return "equal";
        case compare_result::type: return "tokens types do not match";
        case compare_result::key: return "values of keys do not match";
        case compare_result::no_key: return "no required key";
        case compare_result::length: return "length of values do not match";
        case compare_result::value: return "value do not match";
        case compare_result::longer: return "the right-side JSON is longer";
        case compare_result::shorter: return "the right-side JSON is shorter";
    }

    return nullptr;
}

/*************************************************************************************************/

namespace details {

inline bool same_text(
     const chunk_list &left_chunks
    ,const token &left
    ,const chunk_list &right_chunks
    ,const token &right) noexcept
{
    if ( left.vlen != right.vlen ) {
        return false;
    }
    if ( left.vlen == 0u ) {
        return true;
    }
    const char *a = span_at(left_chunks, left.vpos, left.vlen);
    const char *b = span_at(right_chunks, right.vpos, right.vlen);
    if ( !a || !b ) {
        return false;
    }

    return ::memcmp(a, b, left.vlen) == 0;
}

inline std::size_t direct_children(const iterator &container) noexcept {
    const token *const open = container.cur;
    const token *const close = container.toks_beg + open->end_idx;
    if ( open->primitive_only ) {
        const auto between = static_cast<std::size_t>(close - open - 1);

        return open->type == FJ_TYPE_OBJECT ? between / 2u : between;
    }
    const bool object = open->type == FJ_TYPE_OBJECT;
    std::size_t count = 0u;
    for ( const token *cursor = open + 1; cursor < close; ) {
        ++count;
        const token *value = object ? cursor + 1 : cursor;
        cursor = value->type == FJ_TYPE_ARRAY || value->type == FJ_TYPE_OBJECT
            ? container.toks_beg + value->end_idx + 1
            : value + 1;
    }

    return count;
}

inline iterator at_token(const iterator &range, token *tok) noexcept {
    return {
         range.toks_beg
        ,range.toks_end
        ,range.chunks
        ,range.beg
        ,tok
        ,range.end
    };
}

inline void assign_diff(
     iterator *left_diff
    ,iterator *right_diff
    ,const iterator &left
    ,const iterator &right) noexcept
{
    if ( left_diff ) {
        *left_diff = left;
    }
    if ( right_diff ) {
        *right_diff = right;
    }
}

inline compare_result compare_container(
     iterator *left_diff
    ,iterator *right_diff
    ,const iterator &left
    ,const iterator &right
    ,compare_mode mode) noexcept;

inline compare_result compare_leaf(
     iterator *left_diff
    ,iterator *right_diff
    ,const iterator &left
    ,const iterator &right
    ,compare_mode mode) noexcept
{
    if ( left.type() != right.type() ) {
        assign_diff(left_diff, right_diff, left, right);

        return compare_result::type;
    }
    if ( mode == compare_mode::markup_only ) {
        return compare_result::equal;
    }
    const auto left_len = left.cur->vlen;
    const auto right_len = right.cur->vlen;
    if ( mode == compare_mode::length_only ) {
        if ( left_len == right_len ) {
            return compare_result::equal;
        }
        assign_diff(left_diff, right_diff, left, right);

        return compare_result::length;
    }
    if ( left_len == right_len
        && same_text(*left.chunks, *left.cur, *right.chunks, *right.cur) )
    {
        return compare_result::equal;
    }
    assign_diff(left_diff, right_diff, left, right);

    return compare_result::value;
}

inline compare_result compare_value(
     iterator *left_diff
    ,iterator *right_diff
    ,const iterator &left
    ,const iterator &right
    ,compare_mode mode) noexcept
{
    const bool left_simple = left.is_simple_type();
    if ( left_simple != right.is_simple_type() || left.type() != right.type() ) {
        assign_diff(left_diff, right_diff, left, right);

        return compare_result::type;
    }
    if ( left_simple ) {
        return compare_leaf(left_diff, right_diff, left, right, mode);
    }

    return compare_container(
         left_diff
        ,right_diff
        ,begin(left)
        ,begin(right)
        ,mode
    );
}

inline compare_result compare_primitive(
     iterator *left_diff
    ,iterator *right_diff
    ,const iterator &left
    ,const iterator &right
    ,compare_mode mode) noexcept
{
    const bool object = left.type() == FJ_TYPE_OBJECT;
    token *left_tok = left.cur + 1;
    token *right_tok = right.cur + 1;
    token *const left_close = const_cast<token *>(left.toks_beg) + left.cur->end_idx;
    token *const right_close = const_cast<token *>(right.toks_beg) + right.cur->end_idx;
    const std::size_t step = object ? 2u : 1u;
    bool same_order = true;
    for ( ; left_tok < left_close; left_tok += step, right_tok += step ) {
        if ( object && !same_text(*left.chunks, *left_tok, *right.chunks, *right_tok) ) {
            same_order = false;
            break;
        }
        token *const left_value = object ? left_tok + 1 : left_tok;
        token *const right_value = object ? right_tok + 1 : right_tok;
        if ( left_value->type != right_value->type ) {
            assign_diff(
                 left_diff
                ,right_diff
                ,at_token(left, left_value)
                ,at_token(right, right_value)
            );

            return compare_result::type;
        }
        if ( mode == compare_mode::markup_only ) {
            continue;
        }
        if ( mode == compare_mode::length_only ) {
            if ( left_value->vlen == right_value->vlen ) {
                continue;
            }
            assign_diff(
                 left_diff
                ,right_diff
                ,at_token(left, left_value)
                ,at_token(right, right_value)
            );

            return compare_result::length;
        }
        if ( same_text(*left.chunks, *left_value, *right.chunks, *right_value) ) {
            continue;
        }
        assign_diff(
             left_diff
            ,right_diff
            ,at_token(left, left_value)
            ,at_token(right, right_value)
        );

        return compare_result::value;
    }
    if ( same_order ) {
        return compare_result::equal;
    }

    const iterator right_end = end(right);
    for ( token *key = left.cur + 1; key < left_close; key += 2 ) {
        token *found = nullptr;
        for ( token *other = right.cur + 1; other < right_close; other += 2 ) {
            if ( same_text(*left.chunks, *key, *right.chunks, *other) ) {
                found = other + 1;
                break;
            }
        }
        const iterator left_value = at_token(left, key + 1);
        if ( !found ) {
            assign_diff(left_diff, right_diff, left_value, right_end);

            return compare_result::no_key;
        }
        const auto result = compare_leaf(
             left_diff
            ,right_diff
            ,left_value
            ,at_token(right, found)
            ,mode
        );
        if ( result != compare_result::equal ) {
            return result;
        }
    }

    return compare_result::equal;
}

inline bool same_key(const iterator &left, const iterator &right) noexcept {
    const string_view a = left.key();
    const string_view b = right.key();
    if ( a.size() != b.size() ) {
        return false;
    }

    return a.size() == 0u || ::memcmp(a.data(), b.data(), a.size()) == 0;
}

inline compare_result compare_container(
     iterator *left_diff
    ,iterator *right_diff
    ,const iterator &left
    ,const iterator &right
    ,compare_mode mode) noexcept
{
    const auto left_count = direct_children(left);
    const auto right_count = direct_children(right);
    if ( left_count != right_count ) {
        assign_diff(left_diff, right_diff, left, right);

        return left_count < right_count ? compare_result::longer : compare_result::shorter;
    }
    if ( left_count == 0u ) {
        return compare_result::equal;
    }
    if ( left.cur->primitive_only && right.cur->primitive_only ) {
        return compare_primitive(left_diff, right_diff, left, right, mode);
    }

    const iterator left_end = end(left);
    const iterator right_end = end(right);
    iterator left_it = next(left);
    iterator right_it = next(right);
    bool same_order = true;
    for ( ; not_equal(left_it, left_end); left_it = next(left_it), right_it = next(right_it) ) {
        if ( left.is_object() && !same_key(left_it, right_it) ) {
            same_order = false;
            break;
        }
        const auto result = compare_value(left_diff, right_diff, left_it, right_it, mode);
        if ( result != compare_result::equal ) {
            return result;
        }
    }
    if ( same_order || !left.is_object() ) {
        return compare_result::equal;
    }

    for ( left_it = next(left); not_equal(left_it, left_end); left_it = next(left_it) ) {
        const string_view key = left_it.key();
        const iterator found = at(right, key.data(), key.size());
        if ( equal(found, right_end) ) {
            assign_diff(left_diff, right_diff, left_it, right_end);

            return compare_result::no_key;
        }
        const auto result = compare_value(left_diff, right_diff, left_it, found, mode);
        if ( result != compare_result::equal ) {
            return result;
        }
    }

    return compare_result::equal;
}

} // ns details

/*************************************************************************************************/

inline compare_result compare(
     iterator *left_diff
    ,iterator *right_diff
    ,const parser *left_parser
    ,const parser *right_parser
    ,compare_mode mode = compare_mode::markup_only) noexcept
{
    const iterator left = begin(left_parser);
    const iterator right = begin(right_parser);
    if ( left.type() != right.type() ) {
        details::assign_diff(left_diff, right_diff, left, right);

        return compare_result::type;
    }
    if ( left.is_simple_type() ) {
        return details::compare_leaf(left_diff, right_diff, left, right, mode);
    }

    return details::compare_container(left_diff, right_diff, left, right, mode);
}

/*************************************************************************************************/

} // ns flatjson

#endif // __FLATJSON__COMPARE_HPP
