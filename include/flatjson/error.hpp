
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__ERROR_HPP
#define __FLATJSON__ERROR_HPP

#include <cstdint>

#include <cctype>
#include <cstdio>
#include <cstring>

namespace flatjson {

/*************************************************************************************************/

enum error_code: std::int8_t {
     FJ_OK = 0
    ,FJ_INVALID = -1
    ,FJ_INCOMPLETE = -2
    ,FJ_EXTRA_DATA = -3
    ,FJ_NO_FREE_TOKENS = -4
    ,FJ_CANT_ALLOCATE = -5
    ,FJ_SV_CROSS_CHUNK = -6
    ,FJ_NO_FREE_CHUNKS = -7
};

struct error_info {
    error_code  code;
    char        js_str[16];
    unsigned    js_pos;
    unsigned    in_dump_pos;
    unsigned    line;
    const char *func;
    const char *prev_func;
};

inline const char* error_string(error_code e) noexcept {
    static const char* strs[] = {
         "OK"
        ,"INVALID"
        ,"INCOMPLETE"
        ,"EXTRA_DATA"
        ,"NO_FREE_TOKENS"
        ,"CANT_ALLOCATE"
        ,"SV_CROSS_CHUNK"
        ,"NO_FREE_CHUNKS"
    };
    auto idx = static_cast<std::int8_t>(e);
    idx = -idx;

    return strs[idx];
}

inline const char* error_message(const error_info &e) noexcept {
    static char buf[512];
    const char *p = e.js_str;
    auto f = [](const char *at) -> char {
        const auto byte = static_cast<unsigned char>(*at);
        return std::isprint(byte) ? static_cast<char>(byte) : '.';
    };
    auto c = [](const char *p) -> std::uint8_t { return std::uint8_t(*p); };
    char prevfunc[128]{};
    if ( e.prev_func ) {
        std::snprintf(prevfunc, sizeof(prevfunc), ", prev function \"%s()\"", e.prev_func);
    }
    auto len = std::snprintf(buf, sizeof(buf)
        ,"%s, in function \"%s()\"(%u)%s, at json position %u, content:\n"
         "c|%c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c  %c |\n"
         "x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|%02x|\n"
         "%*s^\n"
        ,error_string(e.code), e.func, e.line, (e.prev_func ? prevfunc : ""), e.js_pos
        ,f(p+0), f(p+1), f(p+2), f(p+3), f(p+4), f(p+5), f(p+6), f(p+7)
            ,f(p+8), f(p+9), f(p+10), f(p+11), f(p+12), f(p+13), f(p+14), f(p+15)
        ,c(p+0), c(p+1), c(p+2), c(p+3), c(p+4), c(p+5), c(p+6), c(p+7)
            ,c(p+8), c(p+9), c(p+10), c(p+11), c(p+12), c(p+13), c(p+14), c(p+15)
        ,2+(3 * e.in_dump_pos), " "
    );
    if ( len < 0 ) {
        buf[0] = 0;
    } else if ( static_cast<std::size_t>(len) >= sizeof(buf) ) {
        buf[sizeof(buf) - 1] = 0;
    } else {
        buf[len] = 0;
    }

    return buf;
}

#define __FJ__INIT_ERROR_INFO(p, pcode, pprev_func) { \
    (p)->error.code = pcode; \
    std::memset((p)->error.js_str, 0, sizeof((p)->error.js_str)); \
    (p)->error.js_pos = 0; \
    (p)->error.in_dump_pos = 0; \
    if ( (p)->resume.json_cur && (p)->resume.chunk_beg ) { \
        const char *js_cur = (p)->resume.json_cur; \
        (p)->error.js_pos = (p)->resume.chunk_off \
            + static_cast<unsigned>(js_cur - (p)->resume.chunk_beg); \
        unsigned from_back = (js_cur - (p)->resume.chunk_beg) > 10 \
            ? 10 \
            : (js_cur - (p)->resume.chunk_beg) \
        ; \
        (p)->error.in_dump_pos = from_back; \
        unsigned to_copy = \
            static_cast<unsigned>((p)->resume.json_end - (js_cur - from_back)) < sizeof((p)->error.js_str) \
            ? ((p)->resume.json_end - (js_cur - from_back)) \
            : sizeof((p)->error.js_str) \
        ; \
        /* std::printf("from_back=%d, to_copy=%u\n", from_back, to_copy);*/ \
        to_copy ? std::memcpy((p)->error.js_str, (js_cur - from_back), to_copy) : nullptr; \
    } \
    (p)->error.func = __func__; \
    (p)->error.line = __LINE__; \
    (p)->error.prev_func = pprev_func; \
}

/*************************************************************************************************/

} // ns flatjson

#endif // __FLATJSON__ERROR_HPP
