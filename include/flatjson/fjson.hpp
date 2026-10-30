
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__FJSON_HPP
#define __FLATJSON__FJSON_HPP

#include "stringview.hpp"
#include "error.hpp"
#include "types.hpp"
#include "args.hpp"
#include "iterator.hpp"
#include "parse.hpp"

#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace flatjson {

/*************************************************************************************************/

struct fjson {
    struct const_iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = const iterator;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using reference = value_type &;
        using const_reference = const value_type &;

        explicit const_iterator(iterator it) noexcept
            :m_it{std::move(it)}
        {}

        const_pointer operator->() const noexcept { return &m_it; }
        const_iterator& operator++() noexcept { m_it = next(m_it); return *this; }
        const_iterator operator++(int) noexcept { const_iterator tmp{*this}; ++(*this); return tmp; }
        reference operator* () noexcept { return m_it; }
        const_reference operator* () const noexcept { return m_it; }
        friend bool operator== (const const_iterator &l, const const_iterator &r) noexcept
        { return equal(l.m_it, r.m_it); }
        friend bool operator!= (const const_iterator &l, const const_iterator &r) noexcept
        { return !operator==(l, r); }

        iterator m_it;
    };

private:
    struct intrusive_ptr {
        using deleter_fn_ptr = void(*)(parser *);

        intrusive_ptr(bool manage, parser *parser, deleter_fn_ptr free_fn) noexcept
            :m_manage{manage}
            ,m_parser{parser}
            ,m_free_fn{free_fn}
        {
            if ( m_manage && m_parser ) {
                ++m_parser->ref_counter;
            }
        }
        virtual ~intrusive_ptr() noexcept { release(); }
        intrusive_ptr(const intrusive_ptr &other) noexcept
            :m_manage{other.m_manage}
            ,m_parser{other.m_parser}
            ,m_free_fn{other.m_free_fn}
        {
            if ( m_manage && m_parser ) {
                ++m_parser->ref_counter;
            }
        }
        intrusive_ptr(intrusive_ptr &&other) noexcept
            :m_manage{other.m_manage}
            ,m_parser{other.m_parser}
            ,m_free_fn{other.m_free_fn}
        {
            other.m_manage  = false;
            other.m_parser  = nullptr;
            other.m_free_fn = nullptr;
        }
        intrusive_ptr& operator= (const intrusive_ptr &r) noexcept {
            if ( this == &r ) {
                return *this;
            }
            // A non-owning alias does not keep the parser alive. Dropping the last
            // owning reference frees it, so the alias must not be adopted afterwards.
            if ( m_parser && m_parser == r.m_parser && m_manage && !r.m_manage ) {
                const auto refcnt = --m_parser->ref_counter;
                if ( !refcnt ) {
                    if ( m_free_fn ) {
                        m_free_fn(m_parser);
                    }
                    clear();

                    return *this;
                }
                m_manage = false;
                m_free_fn = r.m_free_fn;

                return *this;
            }
            if ( r.m_manage && r.m_parser ) {
                ++r.m_parser->ref_counter;
            }
            release();
            m_manage = r.m_manage;
            m_parser = r.m_parser;
            m_free_fn = r.m_free_fn;

            return *this;
        }
        intrusive_ptr& operator= (intrusive_ptr &&r) noexcept {
            if ( this == &r ) {
                return *this;
            }
            if ( m_parser && m_parser == r.m_parser ) {
                if ( m_manage && r.m_manage && --m_parser->ref_counter == 0u ) {
                    if ( m_free_fn ) {
                        m_free_fn(m_parser);
                    }
                    clear();
                    r.clear();

                    return *this;
                }
                if ( m_manage && !r.m_manage ) {
                    const auto refcnt = --m_parser->ref_counter;
                    if ( !refcnt ) {
                        if ( m_free_fn ) {
                            m_free_fn(m_parser);
                        }
                        clear();
                        r.clear();

                        return *this;
                    }
                }
                m_manage = r.m_manage;
                m_free_fn = r.m_free_fn;
                r.clear();

                return *this;
            }
            release();
            m_manage = r.m_manage;
            m_parser = r.m_parser;
            m_free_fn = r.m_free_fn;
            r.clear();

            return *this;
        }

        explicit operator bool()    const noexcept { return m_parser != nullptr; }
        const parser* get()         const noexcept { return m_parser; }
        parser*       get()               noexcept { return m_parser; }
        const parser* operator-> () const noexcept { return m_parser; }
        parser*       operator-> ()       noexcept { return m_parser; }
        const parser& operator*  () const noexcept { return *m_parser; }
        parser&       operator*  ()       noexcept { return *m_parser; }

    private:
        void release() noexcept {
            if ( m_manage && m_parser ) {
                const auto refcnt = --m_parser->ref_counter;
                if ( !refcnt && m_free_fn ) {
                    m_free_fn(m_parser);
                }
            }
            clear();
        }
        void clear() noexcept {
            m_manage = false;
            m_parser = nullptr;
            m_free_fn = nullptr;
        }

        bool m_manage;
        parser *m_parser;
        deleter_fn_ptr m_free_fn;
    };

    static void keep_parser(parser *) noexcept {}
    static intrusive_ptr bind_parser(parser *held) noexcept {
        const bool shared = held->ref_counter != 0u;

        return intrusive_ptr{shared, held, shared ? &free_parser : &keep_parser};
    }
    // ref_counter == 0: this parser is not inside intrusive_ptr. Drop buffers it still
    // owns before the caller writes a new document over the object.
    static void release_detached_storage(parser *user) noexcept {
        auto *free_fn = user->free_fn;
        if ( user->dyn_alloc && free_fn ) {
            free_fn(user->toks_beg);
            user->toks_beg = nullptr;
            user->toks_cur = nullptr;
            user->toks_end = nullptr;
            user->dyn_alloc = false;
        }
        auto &cl = user->chunks;
        if ( cl.dyn && free_fn ) {
            free_fn(cl.beg);
            cl = details::chunk_list{};
        }
    }
    void adopt_existing(parser *user) noexcept {
        m_parser = intrusive_ptr{true, user, &free_parser};
        if ( flatjson::is_valid(user) ) {
            m_beg = flatjson::begin(user);
            m_end = flatjson::end(user);
        }
    }

public:
    const_iterator begin()  const noexcept { return const_iterator{m_beg}; }
    const_iterator end()    const noexcept { return const_iterator{m_end}; }
    const_iterator cbegin() const noexcept { return const_iterator{m_beg}; }
    const_iterator cend()   const noexcept { return const_iterator{m_end}; }

    fjson(const fjson &) noexcept = default;
    fjson& operator= (const fjson &other) noexcept {
        if ( this == &other ) {
            return *this;
        }
        m_parser = other.m_parser;
        if ( !m_parser ) {
            m_beg = iterator{};
            m_end = iterator{};
        } else {
            m_beg = other.m_beg;
            m_end = other.m_end;
        }

        return *this;
    }
    fjson(fjson &&other) noexcept
        :m_parser{std::move(other.m_parser)}
        ,m_beg{std::move(other.m_beg)}
        ,m_end{std::move(other.m_end)}
    {
        other.m_beg = iterator{};
        other.m_end = iterator{};
    }
    fjson& operator= (fjson &&other) noexcept {
        if ( this == &other ) {
            return *this;
        }
        m_parser = std::move(other.m_parser);
        if ( !m_parser ) {
            m_beg = iterator{};
            m_end = iterator{};
        } else {
            m_beg = std::move(other.m_beg);
            m_end = std::move(other.m_end);
        }
        other.m_beg = iterator{};
        other.m_end = iterator{};

        return *this;
    }

    fjson() noexcept
        :m_parser{false, nullptr, nullptr}
        ,m_beg{}
        ,m_end{}
    {}

    // construct using user-provided already initialized parser.
    // ref_counter == 0 leaves this a non-owning view: the caller frees the parser.
    // ref_counter > 0 joins the owners, so this view keeps the parser alive.
    fjson(parser *p) noexcept
        :m_parser{bind_parser(p)}
        ,m_beg{flatjson::begin(p)}
        ,m_end{flatjson::end(p)}
    {
        assert(flatjson::is_valid(p));
    }

    // json_beg and json_end are required.
    // parser, toks_beg and toks_end, mem_fns, despaced and flags are optional.
    // The parser argument is initialized (value-initialized or init_parser()).
    // without parser the parser is allocated.
    // without tokens the tokens are allocated.
    // ref_counter == 0: fill this parser from the arguments. Tokens allocated here are
    // freed with this object. The parser object itself is not freed.
    // ref_counter > 0: the parser already has owners. Join them and keep its document.
    template<
         typename ...Args
        ,typename std::enable_if<(sizeof...(Args) > 1), int>::type = 0
    >
    fjson(Args &&...fnargs) noexcept
        :m_parser{false, nullptr, nullptr}
        ,m_beg{}
        ,m_end{}
    {
        __FJ__ACCEPT_ARGS(
             Args
            ,args.json_beg
            ,args.json_end
            ,args.parser
            ,args.toks_beg
            ,args.toks_end
            ,args.mem_fns
            ,args.despaced
            ,args.flags
        );
        __FJ__JSON_ARGS_PAIRED(Args);
        __FJ__ARGS_PAIRED(
             Args
            ,args.toks_beg
            ,args.toks_end
            ,"toks_beg and toks_end are passed together"
        );
        const char *json_beg = details::get(args.json_beg, __FJ__FWD(Args, fnargs));
        const char *json_end = details::get(args.json_end, __FJ__FWD(Args, fnargs));
        parser *user = details::get(args.parser, args.parser = nullptr, __FJ__FWD(Args, fnargs));
        token *toks_beg = details::get(args.toks_beg, args.toks_beg = nullptr ,__FJ__FWD(Args, fnargs));
        token *toks_end = details::get(args.toks_end, args.toks_end = nullptr ,__FJ__FWD(Args, fnargs));
        auto mem_fns = details::get(args.mem_fns, args.mem_fns = {&realloc, &free} ,__FJ__FWD(Args, fnargs));
        const bool despaced = details::get(args.despaced, args.despaced = false ,__FJ__FWD(Args, fnargs));
        const auto flag_arg = details::get(args.flags, args.flags = std::uint8_t{0xff} ,__FJ__FWD(Args, fnargs));
        const bool has_parser = details::has_arg<decltype(args.parser), Args...>::value;
        const bool has_toks = details::has_arg<decltype(args.toks_beg), Args...>::value;
        const bool has_mem = details::has_arg<decltype(args.mem_fns), Args...>::value;

        auto parsed = [&](parser *held) {
            if ( !held ) {
                return;
            }
            if ( flag_arg != 0xffu ) {
                parse(
                     held
                    ,args.json_beg = json_beg
                    ,args.json_end = json_end
                    ,args.flags = flag_arg
                );
            } else {
                parse(held, args.json_beg = json_beg, args.json_end = json_end);
            }
            if ( flatjson::is_valid(held) ) {
                m_beg = flatjson::begin(held);
                m_end = flatjson::end(held);
            }
        };

        if ( has_parser && has_toks ) {
            if ( user->ref_counter != 0u ) {
                adopt_existing(user);
            } else {
                release_detached_storage(user);
                if ( has_mem ) {
                    *user = make_parser(
                         args.toks_beg = toks_beg
                        ,args.toks_end = toks_end
                        ,args.despaced = despaced
                        ,args.mem_fns = mem_fns
                    );
                } else {
                    *user = make_parser(
                         args.toks_beg = toks_beg
                        ,args.toks_end = toks_end
                        ,args.despaced = despaced
                    );
                }
                m_parser = intrusive_ptr{false, user, &keep_parser};
                parsed(user);
            }
        } else if ( has_parser ) {
            if ( user->ref_counter != 0u ) {
                adopt_existing(user);
            } else {
                release_detached_storage(user);
                parser *tmp = alloc_parser(
                     args.init_size = details::estimate_tokens(json_beg, json_end)
                    ,args.mem_fns = mem_fns
                    ,args.despaced = despaced
                );
                if ( tmp ) {
                    *user = *tmp;
                    user->dyn_parser = false;
                    user->ref_counter = 0u;
                    tmp->dyn_alloc = false;
                    mem_fns.free(tmp);
                } else {
                    details::init_parser(
                         user
                        ,nullptr
                        ,nullptr
                        ,nullptr
                        ,nullptr
                        ,mem_fns.alloc
                        ,mem_fns.free
                        ,false
                        ,false
                        ,despaced
                    );
                    user->error = error_info{};
                    user->error.code = FJ_CANT_ALLOCATE;
                    user->error.func = __func__;
                    user->error.line = __LINE__;
                }
                m_parser = intrusive_ptr{true, user, &free_parser};
                if ( tmp ) {
                    parsed(user);
                }
            }
        } else if ( has_toks ) {
            m_parser = intrusive_ptr{true, alloc_parser(
                 args.toks_beg = toks_beg
                ,args.toks_end = toks_end
                ,args.mem_fns = mem_fns
                ,args.despaced = despaced
            ), free_parser};
            parsed(m_parser.get());
        } else {
            m_parser = intrusive_ptr{true, alloc_parser(
                 args.init_size = details::estimate_tokens(json_beg, json_end)
                ,args.mem_fns = mem_fns
                ,args.despaced = despaced
            ), free_parser};
            parsed(m_parser.get());
        }
    }

    virtual ~fjson() = default;

private:
    fjson(intrusive_ptr p, iterator beg, iterator end) noexcept
        :m_parser{std::move(p)}
        ,m_beg{std::move(beg)}
        ,m_end{std::move(end)}
    {}

public:
    bool is_valid() const noexcept { return m_parser && flatjson::is_valid(m_parser.get()); }
    int error() const noexcept { return m_parser->error.code; }
    const char* error_string() const noexcept { return flatjson::error_message(m_parser->error); }

    // number of direct childs for OBJECT/ARRAY, or 1 for a valid SIMPLE type
    std::size_t size() const noexcept { return flatjson::members(m_beg); }
    std::size_t members() const noexcept { return flatjson::members(m_beg); }
    bool is_empty() const noexcept { return size() == 0; }

    std::size_t tokens() const noexcept
    { return static_cast<std::size_t>(m_parser->toks_cur - m_parser->toks_beg); }

    token_type type() const noexcept { return m_beg.type(); }
    const char* type_name() const noexcept { return m_beg.type_name(); }

    bool is_array() const noexcept { return m_beg.is_array(); }
    bool is_object() const noexcept { return m_beg.is_object(); }
    bool is_null() const noexcept { return m_beg.is_null(); }
    bool is_bool() const noexcept { return m_beg.is_bool(); }
    bool is_number() const noexcept { return m_beg.is_number(); }
    bool is_string() const noexcept { return m_beg.is_string(); }
    bool is_simple_type() const noexcept { return m_beg.is_simple_type(); }

    string_view to_string_view() const noexcept { return m_beg.to_string_view(); }
    std::string to_string() const noexcept { return m_beg.to_string(); }
    template<typename T>
    T to() const noexcept { return m_beg.template to<T>(); }
    bool to_bool() const noexcept { return m_beg.to_bool(); }
    std::uint32_t to_uint() const noexcept { return m_beg.to_uint(); }
    std::int32_t to_int() const noexcept { return m_beg.to_int(); }
    std::uint64_t to_uint64() const noexcept { return m_beg.to_uint64(); }
    std::int64_t to_int64() const noexcept { return m_beg.to_int64(); }
    double to_double() const noexcept { return m_beg.to_double(); }
    float to_float() const noexcept { return m_beg.to_float(); }

    template<std::size_t N>
    bool contains(const char (&key)[N]) const noexcept { return contains(key, N-1); }
    template<typename T, typename = typename details::enable_if_const_char_ptr<T>::type>
    bool contains(T key) const noexcept { return contains(key, std::strlen(key)); }
    bool contains(const char *key, std::size_t len) const noexcept
    { auto it = flatjson::at(m_beg, key, len); return not_equal(it, m_end); }

    // for objects
    template<std::size_t N>
    fjson at(const char (&key)[N]) const noexcept { return at(key, N-1); }
    template<typename T, typename = typename details::enable_if_const_char_ptr<T>::type>
    fjson at(T key) const noexcept { return at(key, std::strlen(key)); }
    fjson at(const char *key, std::size_t len) const noexcept {
        auto it = flatjson::at(m_beg, key, len);
        if ( equal(it, m_end) ) {
            return {m_parser, m_end, m_end};
        }

        return {m_parser, it, flatjson::end(it)};
    }
    // for arrays
    fjson at(std::size_t idx) const noexcept {
        auto it = flatjson::at(m_beg, idx);
        if ( equal(it, m_end) ) {
            return {m_parser, m_end, m_end};
        }

        return {m_parser, it, flatjson::end(it)};
    }

    // get a fjson object at iterator position
    fjson at(const const_iterator &it) const noexcept
    { return {m_parser, flatjson::begin(it.m_it), flatjson::end(it.m_it)}; }

    // for arrays
    fjson operator[](std::size_t idx) const noexcept { return at(idx); }

    // for objects
    template<std::size_t N>
    fjson operator[](const char (&key)[N]) const noexcept { return at(key, N-1); }
    template<typename T, typename = typename details::enable_if_const_char_ptr<T>::type>
    fjson operator[](T key) const noexcept { return at(key, std::strlen(key)); }

    // for top level object/array only
    std::size_t keys_num() const noexcept
    {
        if ( !m_beg.is_object() ) {
            return 0;
        }
        return flatjson::members(m_beg);
    }
    std::vector<string_view>
    keys() const noexcept
    {
        std::vector<string_view> out;
        if ( !m_beg.is_object() ) {
            return out;
        }

        out.reserve(flatjson::members(m_beg));
        for ( auto it = next(m_beg), it_end = m_end; not_equal(it, it_end); it = next(it) ) {
            out.push_back(it.key());
        }
        return out;
    }

private:
    intrusive_ptr m_parser;
    iterator   m_beg;
    iterator   m_end;
};

inline std::size_t distance(const fjson::const_iterator &from, const fjson::const_iterator &to) noexcept {
    return flatjson::distance(from.m_it, to.m_it);
}

/*************************************************************************************************/
// json_beg and json_end are required.
// the optional arguments are those of fjson's constructor.

template<typename ...Args>
inline fjson pparse(Args &&...fnargs) noexcept {
    static_assert(sizeof...(Args) > 1, "pparse() takes json_beg and json_end");

    return fjson{std::forward<Args>(fnargs)...};
}

/*************************************************************************************************/

} // ns flatjson

#endif // __FLATJSON__FJSON_HPP
