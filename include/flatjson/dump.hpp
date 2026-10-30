
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__DUMP_HPP
#define __FLATJSON__DUMP_HPP

#include "chunks.hpp"
#include "iterator.hpp"

#include <cstdio>

namespace flatjson {
namespace details {

/*************************************************************************************************/

// for debug purposes

inline void dump_tokens_impl(
     std::FILE *stream
    ,const chunk_list &chunks
    ,const token *beg
    ,const token *cur
    ,const token *end
    ,std::size_t indent) noexcept
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
    for ( auto *it = beg; it != end; ++it ) {
        if ( it->type == FJ_TYPE_ARRAY_END || it->type == FJ_TYPE_OBJECT_END ) {
            assert(indent > 0);
            local_indent -= static_cast<int>(indent);
        }

        const unsigned ty = it->type;
        const char *tname = ty < sizeof(tnames) / sizeof(tnames[0]) ? tnames[ty] : "???";
        const char *val = it->vlen != 0u && ty < sizeof(tnames) / sizeof(tnames[0])
            ? span_at(chunks, it->vpos, it->vlen)
            : nullptr;
        const bool has_val = val != nullptr;
        std::fprintf(stream, "%3d:%c type=%.*s%3s, addr=%p, end=%p, parent=%p, val=\"%.*s\"\n"
            ,(int)(it - beg) // tok index
            ,(it == cur ? '>' : ' ') // current position arrow
            ,local_indent, spaces // indent
            ,tname // type name
            ,it // own addr
            ,beg + it->end_idx // end
            ,beg + it->parent_idx // parent
            ,(int)(has_val ? it->vlen : 5), (has_val ? val : "(nil)") // value
        );
        std::fflush(stream);

        if ( it->type == FJ_TYPE_ARRAY || it->type == FJ_TYPE_OBJECT ) {
            local_indent += static_cast<int>(indent);
        }
    }
}

} // ns details

// dump for parser
inline void dump_tokens(
     std::FILE *stream
    ,const char *caption
    ,const parser *parser
    ,std::size_t indent = 3) noexcept
{
    std::fprintf(stream, "%s:\n", caption);
    const auto *beg = parser->toks_beg;
    const auto *cur = parser->toks_cur;
    return details::dump_tokens_impl(stream, parser->chunks, beg, cur, cur, indent);
}

// dump for iterator
inline void dump_tokens(
     const parser *parser
    ,std::FILE *stream
    ,const char *caption
    ,const iterator &it
    ,std::size_t indent = 3) noexcept
{
    std::fprintf(stream, "%s:\n", caption);
    return details::dump_tokens_impl(
        stream, parser->chunks, it.beg, it.cur, it.end, indent);
}

/*************************************************************************************************/

} // ns flatjson

#endif // __FLATJSON__DUMP_HPP
