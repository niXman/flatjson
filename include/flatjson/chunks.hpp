// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__CHUNKS_HPP
#define __FLATJSON__CHUNKS_HPP

#include "types.hpp"

namespace flatjson {
namespace details {

/*************************************************************************************************/

inline const chunk& chunk_of(const chunk_list &cl, std::uint32_t pos) noexcept {
    if ( cl.num <= 1u ) {
        return cl.first;
    }

    std::uint32_t lo = 0u;
    std::uint32_t hi = cl.num;
    while ( hi - lo > 1u ) {
        const auto mid = lo + (hi - lo) / 2u;
        if ( cl.beg[mid].offset <= pos ) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    return cl.beg[lo];
}

inline const char* text_at(const chunk_list &cl, std::uint32_t pos) noexcept {
    if ( cl.num <= 1u ) {
        return cl.first.beg + pos;
    }

    const chunk &c = chunk_of(cl, pos);

    return c.beg + (pos - c.offset);
}

inline const char* span_at(const chunk_list &cl, std::uint32_t pos, std::uint32_t len) noexcept {
    const chunk &c = chunk_of(cl, pos);
    if ( !c.beg || pos + len > c.offset + c.size ) {
        return nullptr;
    }

    return c.beg + (pos - c.offset);
}

/*************************************************************************************************/

} // ns details
} // ns flatjson

#endif // __FLATJSON__CHUNKS_HPP
