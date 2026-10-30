
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__TYPES_HPP
#define __FLATJSON__TYPES_HPP

#include "error.hpp"

#include <cstddef>
#include <cstdint>

/*************************************************************************************************/
// likely/unlikely + attributes

#if defined(_MSC_VER)
#   define __FJ__LIKELY(...) __VA_ARGS__
#   define __FJ__UNLIKELY(...) __VA_ARGS__
#   define __FJ__NOINLINE
#   define __FJ__ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#   define __FJ__LIKELY(...) __builtin_expect(static_cast<bool>(__VA_ARGS__), 1)
#   define __FJ__UNLIKELY(...) __builtin_expect(static_cast<bool>(__VA_ARGS__), 0)
#   define __FJ__NOINLINE __attribute__((noinline))
#   define __FJ__ALWAYS_INLINE inline __attribute__((always_inline))
#else
#   error "unknown compiler!"
#endif // defined(_MSC_VER)

// Initial token capacity
#ifndef FJ_TOKEN_INIT_CAP
#   define FJ_TOKEN_INIT_CAP 16u
#endif // FJ_TOKEN_INIT_CAP

/*************************************************************************************************/
// fallthrough/constexpr if()

#if __cplusplus >= 201703L
#   define __FJ__FALLTHROUGH [[fallthrough]]
#else
#   if defined(__clang__)
#       define __FJ__FALLTHROUGH [[clang::fallthrough]]
#   elif defined(__GNUC__)
#       define __FJ__FALLTHROUGH __attribute__ ((fallthrough))
#   elif defined(_MSC_VER)
#       define __FJ__FALLTHROUGH
#   else
#       error "Unknown compiler"
#   endif // defined(__clang__)
#endif // __cplusplus >= 201703L

/*************************************************************************************************/

namespace flatjson {

// realloc's contract: nullptr allocates a block, a non-null grows that block.
using alloc_fnptr = void*(*)(void *, std::size_t);
using free_fnptr = void(*)(void *);

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

/*************************************************************************************************/
// 16 bytes for now

#pragma pack(push, 1)
struct token {
    std::uint32_t vpos:32;
    std::uint32_t vlen:30;
    std::uint32_t parent_idx:30;
    std::uint32_t end_idx:30;
    std::uint32_t type:4;
    std::uint32_t primitive_only:1; // array or object consists of only simple types
    std::uint32_t unused:1;
};
#pragma pack(pop)

#if defined(_MSC_VER)
static_assert(sizeof(token) == 20, "");
#else
static_assert(sizeof(token) == 16, "");
#endif

// simple type
#define fj_is_simple_type_macro(v) \
    ("011110000"[v] == '1')

/*************************************************************************************************/

struct chunk {
    const char *beg;
    std::uint32_t size;
    std::uint32_t offset;
};

namespace details {

struct chunk_list {
    chunk first;
    chunk *beg;
    std::uint32_t num;
    std::uint32_t cap;
    bool dyn;
};

struct parse_frame {
    std::uint32_t parent_idx;
    std::uint16_t parent_type;
    std::uint16_t primitive_only;
    std::uint8_t state;
};

static constexpr std::uint8_t k_parse_stack_max = 128u;

struct parse_resume {
    parse_frame stack[k_parse_stack_max];
    std::uint8_t depth;
    std::uint8_t lex;
    std::uint8_t flags;
    bool streaming;
    bool eos;
    std::uint32_t chunk_off;
    const char *chunk_beg;
    const char *json_cur;
    const char *json_end;
};

} // ns details

/*************************************************************************************************/
// parser

struct parser {
    details::chunk_list chunks;

    token *toks_beg;
    token *toks_cur;
    token *toks_end;

    alloc_fnptr alloc_fn;
    free_fnptr free_fn;
    bool dyn_alloc;
    bool dyn_parser;
    bool despaced;
    error_info  error;

    std::uint32_t ref_counter;

    details::parse_resume resume;
};

} // ns flatjson

/*************************************************************************************************/

#endif // __FLATJSON__TYPES_HPP
