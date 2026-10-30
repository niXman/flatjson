
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__CONV_HPP
#define __FLATJSON__CONV_HPP

#include "stringview.hpp"
#include "types.hpp"

#include <limits>
#include <string>
#include <type_traits>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace flatjson {
namespace details {

/*************************************************************************************************/

template<typename To>
typename std::enable_if<
    (std::is_integral<To>::value && std::is_unsigned<To>::value) &&
    !std::is_same<To, bool>::value, To
>::type
conv_to(const char *ptr, std::size_t len, To) noexcept {
    const auto *str = reinterpret_cast<const std::uint8_t *>(ptr);
    std::uint64_t res = 0;
#define __FJ__PER_CHAR_EXPR(n) ((res << 1) + (res << 3) + (str[len - n] - '0'))
    switch ( len ) {
        case 20: res = __FJ__PER_CHAR_EXPR(20); __FJ__FALLTHROUGH;
        case 19: res = __FJ__PER_CHAR_EXPR(19); __FJ__FALLTHROUGH;
        case 18: res = __FJ__PER_CHAR_EXPR(18); __FJ__FALLTHROUGH;
        case 17: res = __FJ__PER_CHAR_EXPR(17); __FJ__FALLTHROUGH;
        case 16: res = __FJ__PER_CHAR_EXPR(16); __FJ__FALLTHROUGH;
        case 15: res = __FJ__PER_CHAR_EXPR(15); __FJ__FALLTHROUGH;
        case 14: res = __FJ__PER_CHAR_EXPR(14); __FJ__FALLTHROUGH;
        case 13: res = __FJ__PER_CHAR_EXPR(13); __FJ__FALLTHROUGH;
        case 12: res = __FJ__PER_CHAR_EXPR(12); __FJ__FALLTHROUGH;
        case 11: res = __FJ__PER_CHAR_EXPR(11); __FJ__FALLTHROUGH;
        case 10: res = __FJ__PER_CHAR_EXPR(10); __FJ__FALLTHROUGH;
        case 9 : res = __FJ__PER_CHAR_EXPR(9 ); __FJ__FALLTHROUGH;
        case 8 : res = __FJ__PER_CHAR_EXPR(8 ); __FJ__FALLTHROUGH;
        case 7 : res = __FJ__PER_CHAR_EXPR(7 ); __FJ__FALLTHROUGH;
        case 6 : res = __FJ__PER_CHAR_EXPR(6 ); __FJ__FALLTHROUGH;
        case 5 : res = __FJ__PER_CHAR_EXPR(5 ); __FJ__FALLTHROUGH;
        case 4 : res = __FJ__PER_CHAR_EXPR(4 ); __FJ__FALLTHROUGH;
        case 3 : res = __FJ__PER_CHAR_EXPR(3 ); __FJ__FALLTHROUGH;
        case 2 : res = __FJ__PER_CHAR_EXPR(2 ); __FJ__FALLTHROUGH;
        case 1 : res = __FJ__PER_CHAR_EXPR(1 );
            break;
        default:
            assert(!"unreachable!");
    }
#undef __FJ__PER_CHAR_EXPR

    return static_cast<To>(res);
}

template<typename To>
typename std::enable_if<
    (std::is_integral<To>::value && std::is_signed<To>::value) &&
    !std::is_same<To, bool>::value, To
>::type
conv_to(const char *ptr, std::size_t len, To) noexcept {
    using UnsignedTo = typename std::make_unsigned<To>::type;
    if ( len == 0 ) {
        assert(!"empty input for signed conversion");
        return To{};
    }

    const UnsignedTo max_pos = static_cast<UnsignedTo>(std::numeric_limits<To>::max());
    const UnsignedTo min_abs = max_pos + 1u;

    if ( *ptr == '-' ) {
        if ( len == 1 ) {
            assert(!"invalid signed integer literal");
            return To{};
        }
        ++ptr;
        auto res = conv_to(ptr, len - 1, UnsignedTo{});
        if ( res > min_abs ) {
            assert(!"overflow detected!");
            return std::numeric_limits<To>::min();
        }
        if ( res == min_abs ) {
            return std::numeric_limits<To>::min();
        }

        return -static_cast<To>(res);
    }

    auto res = conv_to(ptr, len, UnsignedTo{});
    if ( res > max_pos ) {
        assert(!"overflow detected!");
        return std::numeric_limits<To>::max();
    }

    return static_cast<To>(res);
}

template<typename To>
typename std::enable_if<std::is_same<To, bool>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) noexcept {
    return *ptr == 't' && len == 4;
}

template<typename To>
typename std::enable_if<std::is_same<To, double>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) noexcept {
    char buf[std::numeric_limits<To>::max_exponent10 + 20];
    std::memcpy(buf, ptr, len);
    buf[len] = 0;

    return std::strtod(buf, nullptr);
}

template<typename To>
typename std::enable_if<std::is_same<To, float>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) noexcept {
    char buf[std::numeric_limits<To>::max_exponent10 + 20];
    std::memcpy(buf, ptr, len);
    buf[len] = 0;

    return std::strtof(buf, nullptr);
}

template<typename To>
typename std::enable_if<std::is_same<To, std::string>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) { return {ptr, len}; }

template<typename To>
typename std::enable_if<std::is_same<To, flatjson::string_view>::value, To>::type
conv_to(const char *ptr, std::size_t len, To) { return {ptr, len}; }


/*************************************************************************************************/

} // ns details
} // ns flatjson

#endif // __FLATJSON__CONV_HPP
