
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__STRINGVIEW_HPP
#define __FLATJSON__STRINGVIEW_HPP

#include <algorithm>
#include <ostream>
#include <string>

#include <cassert>
#include <cstddef>
#include <cstring>

#if __cplusplus >= 201703L
#   include <string_view>
#endif // __cplusplus >= 201703L

namespace flatjson {
namespace details {

/*************************************************************************************************/

template<typename T>
struct enable_if_const_char_ptr;

template<>
struct enable_if_const_char_ptr<const char *> {
    using type = void;
};

} // ns details

#if __cplusplus >= 201703L

using string_view = std::string_view;

#else

/*************************************************************************************************/

struct string_view {
    string_view() = default;
    explicit string_view(const char *s) noexcept
        :m_ptr{s}
        ,m_len{std::strlen(m_ptr)}
    {}
    template<std::size_t N>
    explicit string_view(const char (&str)[N]) noexcept
        :m_ptr{str}
        ,m_len{N-1}
    {}
    string_view(const char *ptr, std::size_t len) noexcept
        :m_ptr{ptr}
        ,m_len{len}
    {}
    string_view(const char *beg, const char *end) noexcept
        :m_ptr{beg}
        ,m_len{static_cast<std::size_t>(end - beg)}
    {}

    std::size_t size() const noexcept { return m_len; }
    std::size_t length() const noexcept { return m_len; }
    bool empty() const noexcept { return size() == 0; }
    const char* data() const noexcept { return m_ptr; }

    string_view substr(std::size_t pos = 0, std::size_t n = std::string::npos) const noexcept {
        assert(pos <= size());
        return string_view( data() + pos, (std::min)( n, size() - pos ) );
    }

    int compare(string_view other) const noexcept {
        const std::size_t n = (std::min)(size(), other.size());
        const int res = n ? std::memcmp(data(), other.data(), n) : 0;

        return res ? res : size() == other.size() ? 0 : size() < other.size() ? -1 : 1;
    }
    int compare(std::size_t pos1, std::size_t n1, string_view other) const noexcept
    { return substr(pos1, n1).compare(other); }
    int compare(std::size_t pos1, std::size_t n1, string_view other, std::size_t pos2, std::size_t n2) const noexcept
    { return substr(pos1, n1).compare(other.substr(pos2, n2)); }
    template<std::size_t N>
    int compare(const char (&r)[N]) const noexcept { return compare(string_view{r, N-1}); }
    int compare(const char *s) const noexcept { return compare(string_view(s)); }
    template<std::size_t N>
    int compare(std::size_t pos1, std::size_t n1, const char (&s)[N]) const noexcept
    { return substr(pos1, n1).compare(string_view(s, N-1)); }
    int compare(std::size_t pos1, std::size_t n1, const char *s) const noexcept
    { return substr(pos1, n1).compare(string_view(s)); }
    int compare( std::size_t pos1, std::size_t n1, const char *s, std::size_t n2 ) const noexcept
    { return substr(pos1, n1).compare(string_view(s, n2)); }

    template<typename T, typename = typename details::enable_if_const_char_ptr<T>::type>
    friend bool operator==(const string_view &l, T r) noexcept { return l.compare(r) == 0; }
    template<std::size_t N>
    friend bool operator==(const string_view &l, const char (&r)[N]) noexcept
    { return l.compare(0, N-1, r, N-1) == 0; }
    friend bool operator==(const string_view &l, const string_view &r) noexcept
    { return l.compare(r) == 0; }

    template<typename T, typename = typename details::enable_if_const_char_ptr<T>::type>
    friend bool operator!=(const string_view &l, T r) noexcept { return !(l == r); }
    template<std::size_t N>
    friend bool operator!=(const string_view &l, const char (&r)[N]) noexcept { return !(l == r); }
    friend bool operator!=(const string_view &l, const string_view &r) noexcept { return !(l == r); }

    friend std::ostream& operator<< (std::ostream &os, const string_view &s) noexcept {
        os.write(s.m_ptr, s.m_len);

        return os;
    }

private:
    const char *m_ptr;
    std::size_t m_len;
};

#endif // __cplusplus >= 201703L

/*************************************************************************************************/

} // ns flatjson

#endif // __FLATJSON__STRINGVIEW_HPP
