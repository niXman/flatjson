
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__ARGS_HPP
#define __FLATJSON__ARGS_HPP

#include "types.hpp"

#include <type_traits>
#include <utility>

namespace flatjson {
namespace details {

/*************************************************************************************************/
// named functions argument processing: https://github.com/niXman/named-args

template<typename ...Types>
struct types_list;

// position

template<typename T, int I, typename ...Args>
struct position_impl;

template<typename T, int I>
struct position_impl<T, I>
    :std::integral_constant<int, -1>
{};

template<typename T, int I, typename Arg0, typename ...Args>
struct position_impl<T, I, Arg0, Args...>
    :std::integral_constant<
         int
        ,std::is_same<T, Arg0>::value
            ? I
            : position_impl<T, I+1, Args...>::value
    >
{};

template<typename T, typename ...Args>
struct position
    :position_impl<typename std::decay<T>::type, 0, Args...>
{};

// multi-contains

template<bool OK, typename T0, typename ...Args>
struct multi_contains_impl;

template<typename T0, typename ...Types, typename ...Args>
struct multi_contains_impl<false, types_list<T0, Types...>, Args...>
    :multi_contains_impl<
         position<T0, Args...>::value != -1
        ,types_list<Types...>
        ,Args...
    >::type
{};

template<typename ...Args>
struct multi_contains_impl<false, types_list<>, Args...>
    :std::false_type
{};

template<typename ...Types, typename ...Args>
struct multi_contains_impl<true, types_list<Types...>, Args...>
    :std::true_type
{};

template<typename T, typename ...Args>
struct multi_contains;

template<typename T0, typename ...Types, typename ...Args>
struct multi_contains<types_list<T0, Types...>, Args...>
    :multi_contains_impl<
         position<T0, Args...>::value != -1
        ,types_list<T0, Types...>
        ,Args...
    >::type
{};

// get() for variadic

template<bool ok>
struct get_arg_impl_variadic;

template<>
struct get_arg_impl_variadic<true> {
    template<
         typename K
        ,typename Def
        ,typename Arg0
        ,typename ...Args
    >
    static typename std::enable_if<!std::is_same<K, Arg0>::value, typename K::type>::type
    get(const K &unused0, Def &&unused1, Arg0 &&/*arg0*/, Args && ...args) {
        return get(unused0, std::forward<Def>(unused1), std::forward<Args>(args)...);
    }
    template<
         typename K
        ,typename Def
        ,typename Arg0
        ,typename ...Args
    >
    static typename std::enable_if<std::is_same<K, Arg0>::value, typename K::type>::type
    get(const K &/*unused0*/, Def &&/*unused1*/, Arg0 &&arg0, Args && .../*args*/) {
        return std::forward<Arg0>(arg0).v;
    }
};

template<>
struct get_arg_impl_variadic<false> {
    template<
         typename K
        ,typename Def
        ,typename ...Args
    >
    static typename Def::type get(const K &/*k*/, Def &&def, Args &&.../*args*/) {
        return std::forward<Def>(def).v;
    }
};

template<typename K, typename ...Args>
typename K::type get(const K &k, Args && ...args) {
    constexpr int idx = position<K, Args...>::value;
    static_assert(idx != -1, "looks like that argument is required");
    return get_arg_impl_variadic<idx != -1>::template get(
        k, K{}, std::forward<Args>(args)...);
}

template<typename K, typename Def, typename ...Args>
typename K::type get(const K &k, Def &&def, Args && ...args) {
    constexpr int idx = position<K, Args...>::value;
    return get_arg_impl_variadic<idx != -1>::template get(
        k, std::forward<Def>(def), std::forward<Args>(args)...);
}

template<typename K, typename ...Args>
struct has_arg
    :std::integral_constant<
         bool
        ,position<K, typename std::decay<Args>::type...>::value != -1
    >
{};

template<typename Known, typename ...Args>
struct all_args_known;

template<typename ...Known>
struct all_args_known<types_list<Known...>>
    :std::true_type
{};

template<typename ...Known, typename Arg0, typename ...Args>
struct all_args_known<types_list<Known...>, Arg0, Args...>
    :std::integral_constant<
         bool
        ,position<Arg0, Known...>::value != -1
            && all_args_known<types_list<Known...>, Args...>::value
    >
{};

template<typename ...Args, typename ...Known>
constexpr bool args_known(const Known &...) noexcept
{ return all_args_known<types_list<Known...>, Args...>::value; }

// helpers

#define __FJ__ACCEPT_ARGS(VARIADIC, ...) \
    static_assert( \
         ::flatjson::details::args_known<VARIADIC...>(__VA_ARGS__) \
        ,"the function does not take one of the passed arguments" \
    )

#define __FJ__PARENTHESIS_MUST_BE_PLACED_AROUND_THE_RETURN_TYPE(...) __VA_ARGS__>::type
#define __FJ__FUNCTION_ENABLE(VARIADIC, ...) \
    typename std::enable_if< \
        ::flatjson::details::multi_contains<::flatjson::details::types_list<decltype(__VA_ARGS__)> \
            ,VARIADIC...>::value == true \
                ,__FJ__PARENTHESIS_MUST_BE_PLACED_AROUND_THE_RETURN_TYPE

#define __FJ__FUNCTION_DISABLE(VARIADIC, ...) \
    typename std::enable_if< \
    ::flatjson::details::multi_contains<::flatjson::details::types_list<decltype(__VA_ARGS__)> \
            ,VARIADIC...>::value == false \
                ,__FJ__PARENTHESIS_MUST_BE_PLACED_AROUND_THE_RETURN_TYPE

#define __FJ__FWD(VAR, PACK) std::forward<VAR>(PACK)...

} // ns details

/*************************************************************************************************/
// arguments placeholders

struct {
    struct json_beg { using type = const char *; type v;
        json_beg operator= (type v) const noexcept { return {v}; }
    } const json_beg{};
    struct json_end { using type = const char *; type v;
        json_end operator= (type v) const noexcept { return {v}; }
    }const json_end{};
    struct tokens_beg { using type = token *; type v;
        tokens_beg operator= (type v) const noexcept { return {v}; }
    } const toks_beg{};
    struct tokens_end { using type = token *; type v;
        tokens_end operator= (type v) const noexcept { return {v}; }
    } const toks_end{};
    struct parser { using type = struct flatjson::parser *; type v;
        parser operator= (type v) const noexcept { return {v}; }
    } const parser{};
    struct mem_fns { struct type { alloc_fnptr alloc; free_fnptr free; } v;
        mem_fns operator= (type v) const noexcept { return {v}; }
    } const mem_fns{};
    struct chunks_beg { using type = chunk *; type v;
        chunks_beg operator= (type v) const noexcept { return {v}; }
    } const chunks_beg{};
    struct chunks_end { using type = chunk *; type v;
        chunks_end operator= (type v) const noexcept { return {v}; }
    } const chunks_end{};
    struct init_size { using type = std::size_t; type v;
        init_size operator= (type v) const noexcept { return {v}; }
    } const init_size{};
    struct despaced { using type = bool; type v;
        despaced operator= (type v) const noexcept { return {v}; }
    } const despaced{};
    struct flags { using type = std::uint8_t; type v;
        flags operator= (type v) const noexcept { return {v}; }
    } const flags{};
    struct error_code { using type = flatjson::error_code *; type v;
        error_code operator= (type v) const noexcept { return {v}; }
    } const error_code{};
    struct error_info { using type = flatjson::error_info *; type v;
        error_info operator= (type v) const noexcept { return {v}; }
    } const error_info{};
} static constexpr args;


} // ns flatjson

#define __FJ__ARGS_PAIRED(VARIADIC, FIRST, SECOND, MSG) \
    static_assert( \
         ::flatjson::details::has_arg<decltype(FIRST), VARIADIC...>::value \
            == ::flatjson::details::has_arg<decltype(SECOND), VARIADIC...>::value \
        ,MSG \
    )

#define __FJ__JSON_ARGS_PAIRED(VARIADIC) \
    __FJ__ARGS_PAIRED( \
         VARIADIC \
        ,::flatjson::args.json_beg \
        ,::flatjson::args.json_end \
        ,"json_beg and json_end are passed together" \
    )

/*************************************************************************************************/

#endif // __FLATJSON__ARGS_HPP
