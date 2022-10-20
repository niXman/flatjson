
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2022 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#include <flatjson/flatjson.hpp>
#include <flatjson/io.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <functional>

#include <cassert>

/*************************************************************************************************/

std::size_t token_childs(const flatjson::token *t)
{ return t->childs; }

flatjson::string_view token_key(const flatjson::token *t)
{ return {t->key, t->klen}; }

flatjson::string_view token_value(const flatjson::token *t)
{ return {t->val, t->vlen}; }

bool token_to_bool(const flatjson::token *t)
{ auto sv = token_value(t); return flatjson::details::conv_to(sv.data(), sv.size(), bool{}); }

int token_to_int(const flatjson::token *t)
{ auto sv = token_value(t); return flatjson::details::conv_to(sv.data(), sv.size(), int{}); }

flatjson::string_view token_to_string_view(const flatjson::token *t)
{ return token_value(t); }

std::string token_to_string(const flatjson::token *t)
{ auto sv = token_value(t); return {sv.data(), sv.size()}; }

flatjson::token* token_parent(const flatjson::token *t)
{ return t->parent; }

#ifdef WIN32
static const TCHAR dir_separator = '\\';
#else
static const char dir_separator = '/';
#endif // WIN32

#define FJ_TEST(...) \
    []{ \
        const char *fileline = __FILE__ "(" FJ_STRINGIZE(__LINE__) ")"; \
        const char *ptr = std::strrchr(fileline, dir_separator); \
        ptr = ptr ? ptr+1 : ptr; \
        static char buf[256]{}; \
        int len = std::snprintf(buf, sizeof(buf), "%2d: %s", __COUNTER__, ptr); \
        static const char notes[] = FJ_STRINGIZE(__VA_ARGS__); \
        if ( sizeof(notes)-1 != 0 ) { \
            std::snprintf(buf+len, sizeof(buf)-len, "(%s)", notes); \
        } \
        return buf; \
    }() + []

#define FJ_STRINGIZE_IMPL(x) #x
#define FJ_STRINGIZE(x) FJ_STRINGIZE_IMPL(x)

/*************************************************************************************************/

template<typename F>
std::pair<std::string, std::function<void()>>
operator+ (const char *s, F &&f) {
    return {s, std::forward<F>(f)};
}

struct test_t {
    test_t& operator+= (std::pair<std::string, std::function<void()>> func) {
        auto res = m_funcs.insert(std::move(func));
        assert(res.second);

        return *this;
    }
    void run() {
        for ( const auto &it: m_funcs ) {
            it.second();
            std::cout << "unit \"" << it.first << "\" passed!" << std::endl;
        }
    }

private:
    std::map<std::string, std::function<void()>> m_funcs;
};

/*************************************************************************************************/

struct {
    void* alloc(std::size_t size) {
        void *ptr = std::malloc(size);
        m_map.emplace(ptr, stat_t{ptr, size});

        return ptr;
    }
    void free(const void *ptr) {
        auto it = m_map.find(ptr);
        assert(it != m_map.end());

        std::free(it->second.ptr);
        m_map.erase(it);
    }
    bool valid() const { return m_map.empty(); }

    std::size_t allocations() const { return m_map.size(); }
    std::size_t total_alloc() const {
        std::size_t res{};
        for ( const auto &it: m_map ) {
            res += it.second.size;
        }

        return res;
    }

    void reset() {
        for ( auto &it: m_map ) {
            void *ptr = it.second.ptr;
            std::free(ptr);
        }

        m_map.clear();
    }

    struct stat_t {
        void *ptr;
        std::size_t size;
    };

    void dump(std::ostream &os) const {
        for ( const auto &it: m_map) {
            os << it.second.ptr << ": " << it.second.size << std::endl;
        }
    }

private:

    std::map<const void *, stat_t> m_map;
} myallocator;

static void* my_alloc(std::size_t size) { return myallocator.alloc(size); }
static void my_free(void *ptr) { return myallocator.free(ptr); }

/*************************************************************************************************/

std::string read_file(const char *fname) {
    int ec{};
    flatjson::file_handle fh{};
    const char *ptr = static_cast<const char *>(flatjson::mmap_for_read(&fh, fname, &ec));
    assert(ec == 0);

    auto fsize = flatjson::file_size(fname, &ec);
    assert(ec == 0);

    std::string str{ptr, ptr + fsize};

    flatjson::munmap_file_fd(ptr, fh, &ec);
    assert(ec == 0);

    return str;
}

/*************************************************************************************************/

struct test_result {
    int ec;
    const char *name;
};

int parse_file(const char *path, const char *fname) {
    std::string p{path};
    p += fname;

    int lec{};
    auto fsize = flatjson::file_size(p.c_str(), &lec);
    if ( lec ) {
        return flatjson::FJ_EC_INVALID;
    }

    flatjson::file_handle fd{};
    const char *addr = static_cast<const char *>(flatjson::mmap_for_read(&fd, p.c_str(), &lec));
    if ( lec ) {
        return flatjson::FJ_EC_INVALID;
    }

    flatjson::error_code ec{};
    flatjson::num_tokens(&ec, addr, addr + fsize);

    flatjson::munmap_file_fd(addr, fd, &lec);
    if ( lec ) {
        std::cout << "parse_file(): munmap_file: err=" << lec << std::endl;
    }

    return ec;
}

test_result test_conformance() {
    static const char *part0_path = "jsonchecker/";
    static const char *part0[] = {
         "0:pass01.json","0:pass02.json","0:pass03.json","1:fail02.json"
        ,"1:fail03.json","1:fail04.json","1:fail05.json","1:fail06.json"
        ,"1:fail07.json","1:fail08.json","1:fail09.json","1:fail10.json"
        ,"1:fail11.json","1:fail12.json","1:fail13.json","1:fail14.json"
        ,"1:fail15.json","1:fail16.json","1:fail17.json","1:fail19.json"
        ,"1:fail20.json","1:fail21.json","1:fail22.json","1:fail23.json"
        ,"1:fail24.json","1:fail25.json","1:fail26.json","1:fail27.json"
        ,"1:fail28.json","1:fail29.json","1:fail30.json","1:fail31.json"
        ,"1:fail32.json","1:fail33.json"
        ,nullptr
    };
    for ( const char **it = part0; *it; ++it ) {
        int exp = *it[0] - '0';
        int ec = parse_file(part0_path, *it+2);
        if ( exp && 0 == ec ) {
            return {ec, *it+2};
        }
    }

    static const char *part1_path = "roundtrip/";
    static const char *part1[] = {
         "0:roundtrip01.json","0:roundtrip04.json","0:roundtrip07.json","0:roundtrip10.json"
        ,"0:roundtrip13.json","0:roundtrip16.json","0:roundtrip19.json","0:roundtrip22.json"
        ,"0:roundtrip25.json","0:roundtrip02.json","0:roundtrip05.json","0:roundtrip08.json"
        ,"0:roundtrip11.json","0:roundtrip14.json","0:roundtrip17.json","0:roundtrip20.json"
        ,"0:roundtrip23.json","0:roundtrip26.json","0:roundtrip03.json","0:roundtrip06.json"
        ,"0:roundtrip09.json","0:roundtrip12.json","0:roundtrip15.json","0:roundtrip18.json"
        ,"0:roundtrip21.json","0:roundtrip24.json","0:roundtrip27.json"
        ,nullptr
    };
    for ( const char **it = part1; *it; ++it ) {
        int exp = *it[0] - '0';
        int ec = parse_file(part1_path, *it+2);
        if ( exp && 0 == ec ) {
            return {ec, *it+2};
        }
    }

    return {};
}

/*************************************************************************************************/

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

int main() {
    std::cout << "sizeof(fj_token) = " << sizeof(flatjson::token) << std::endl;
    std::cout << "version str=" << FJ_VERSION_STRING << std::endl;

    auto res = test_conformance();
    if ( res.ec ) {
        std::cout << "test \"" << res.name << "\" FAILED!" << std::endl;
    } else {
        std::cout << "conformance tests PASSED!" << std::endl;
    }

    test_t test;

    /*************************************************************************************************/

    test += FJ_TEST(test for fj_is_simple_type()) {
        assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_INVALID) == false);
        assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_STRING) == true);
        assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_NUMBER) == true);
        assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_BOOL) == true);
        assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_NULL) == true);
        assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_OBJECT) == false);
        assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_OBJECT_END) == false);
        assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_ARRAY) == false);
        assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_ARRAY_END) == false);
    };

    test += FJ_TEST(test for empty JSON) {
        using namespace flatjson;

        static const char str[] = R"()";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(!is_valid(parser));
        assert(toknum == 0);
        assert(parser->toks_end == parser->toks_beg + toknum);

        free_parser(parser);
    };

    test += FJ_TEST(test for empty JSON 2) {
        using namespace flatjson;

        static const char str[] = R"("")";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_valid());
        assert(beg.is_string());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        free_parser(parser);
    };

    test += FJ_TEST(test for NUMBER) {
        using namespace flatjson;

        static const char str[] = R"(3)";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_valid());
        assert(beg.is_number());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        free_parser(parser);
    };

    test += FJ_TEST(test for NUMBER which is a float) {
        using namespace flatjson;

        static const char str[] = R"(3.14)";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_valid());
        assert(beg.is_number());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        free_parser(parser);
    };

    test += FJ_TEST(test for STRING) {
        using namespace flatjson;

        static const char str[] = R"("string")";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_valid());
        assert(beg.is_string());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        free_parser(parser);
    };

    test += FJ_TEST(test for BOOL-false) {

        using namespace flatjson;

        static const char str[] = R"(false)";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_valid());
        assert(beg.is_bool());
        assert(beg.to_bool() == false);

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        free_parser(parser);
    };

    test += FJ_TEST(test for BOOL-true) {

        using namespace flatjson;

        static const char str[] = R"(true)";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_valid());
        assert(beg.is_bool());
        assert(beg.to_bool() == true);

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        free_parser(parser);
    };

    test += FJ_TEST(test for "NULL") {
        using namespace flatjson;

        static const char str[] = R"(null)";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_valid());
        assert(beg.is_null());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        free_parser(parser);
    };

    test += FJ_TEST(test for empty JSON OBJECT) {
        using namespace flatjson;

        static const char str[] = R"({})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 2);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_valid());
        assert(beg.is_object());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        free_parser(parser);
    };

    test += FJ_TEST(test for empty ARRAY) {
        using namespace flatjson;

        static const char str[] = R"([])";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 2);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_valid());
        assert(beg.is_array());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        free_parser(parser);
    };

    test += FJ_TEST(test for parse OBJECT) {
        using namespace flatjson;

        static const char str[] = R"({"bb":0, "b":1})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 4);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_object());
        assert(beg.beg == parser->toks_beg);

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        auto b = iter_at("b", beg);
        assert(b.cur == parser->toks_beg + 2);

        auto bb = iter_at("bb", beg);
        assert(bb.cur == parser->toks_beg + 1);

        auto c = iter_at("c", beg);
        assert(c.cur == parser->toks_end);

        free_parser(parser);
    };

    test += FJ_TEST(test for parse ARRAY) {
        using namespace flatjson;

        static const char str[] = R"([1,0])";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 4);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_array());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        auto i0 = iter_at(0, beg);
        assert(i0.is_valid());
        assert(i0.is_number());
        assert(i0.value() == "1");
        assert(i0.cur == parser->toks_beg + 1);

        auto i1 = iter_at(1, beg);
        assert(i1.is_valid());
        assert(i1.is_number());
        assert(i1.value() == "0");
        assert(i1.cur == parser->toks_beg + 2);

        free_parser(parser);
    };

    test += FJ_TEST(test for parse OBJECT with ARRAY as value) {
        using namespace flatjson;

        static const char str[] = R"({"a":[1,0]})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 6);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_object());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        auto i0 = iter_at("a", beg);
        assert(i0.is_valid());
        assert(i0.is_array());
        assert(i0.key() == "a");
        assert(i0.cur == parser->toks_beg + 1);

        auto i1 = iter_at(0, i0);
        assert(i1.is_valid());
        assert(i1.is_number());
        assert(i1.value() == "1");
        assert(i1.cur == parser->toks_beg + 2);

        auto i2 = iter_at(1, i0);
        assert(i2.is_valid());
        assert(i2.is_number());
        assert(i2.value() == "0");
        assert(i2.cur == parser->toks_beg + 3);

        free_parser(parser);
    };

    test += FJ_TEST(test for parse ARRAY with OBJECT as value) {
        using namespace flatjson;

        static const char str[] = R"([{"a":0, "b":1}])";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 6);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_array());

        auto end = iter_end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        auto obj = iter_next(beg);
        assert(obj.is_valid());
        assert(obj.is_object());
        assert(obj.cur == parser->toks_beg + 1);

        auto a = iter_at("a", obj);
        assert(a.is_valid());
        assert(a.is_number());
        assert(a.value() == "0");
        assert(a.cur == parser->toks_beg + 2);

        auto b = iter_at("b", obj);
        assert(b.is_valid());
        assert(b.is_number());
        assert(b.value() == "1");
        assert(b.cur == parser->toks_beg + 3);

        auto objend = iter_next(b);
        assert(objend.is_valid());
        assert(objend.type() == FJ_TYPE_OBJECT_END);
        assert(objend.cur == parser->toks_beg + 4);

        free_parser(parser);
    };

    // stack allocated parser and tokens
    test += FJ_TEST(test for stack-allocated parser and tokens) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        token tokens[10];
        auto parser = make_parser(std::begin(tokens), std::end(tokens), str);
        auto toknum = parse(&parser);

        assert(is_valid(&parser));
        assert(toknum == 7);
        assert(parser.toks_end == parser.toks_beg + toknum);

        assert(tokens[0].type == FJ_TYPE_OBJECT);
        assert(token_childs(&tokens[0]) == 6);
        assert(token_parent(&tokens[0]) == nullptr);

        assert(tokens[1].type == FJ_TYPE_BOOL);
        assert(token_key(&tokens[1]) == "a");
        assert(token_to_bool(&tokens[1]) == true);
        assert(token_value(&tokens[1]) == "true");
        assert(token_parent(&tokens[1]) == &tokens[0]);

        assert(tokens[2].type == FJ_TYPE_BOOL);
        assert(token_key(&tokens[2]) == "b");
        assert(token_to_bool(&tokens[2]) == false);
        assert(token_value(&tokens[2]) == "false");
        assert(token_parent(&tokens[2]) == &tokens[0]);

        assert(tokens[3].type == FJ_TYPE_NULL);
        assert(token_key(&tokens[3]) == "c");
        assert(token_value(&tokens[3]) == "null");
        assert(token_parent(&tokens[3]) == &tokens[0]);

        assert(tokens[4].type == FJ_TYPE_NUMBER);
        assert(token_key(&tokens[4]) == "d");
        assert(token_value(&tokens[4]) == "0");
        assert(token_to_int(&tokens[4]) == 0);
        assert(token_parent(&tokens[4]) == &tokens[0]);

        assert(tokens[5].type == FJ_TYPE_STRING);
        assert(token_key(&tokens[5]) == "e");
        assert(token_value(&tokens[5]) == "e");
        assert(token_to_string_view(&tokens[5]) == "e");
        assert(token_to_string(&tokens[5]) == "e");
        assert(token_parent(&tokens[5]) == &tokens[0]);

        assert(tokens[6].type == FJ_TYPE_OBJECT_END);
        assert(token_parent(&tokens[6]) == &tokens[0]);

        free_parser(&parser);
    };

    // stack allocated tokens and dyn allocated parser
    test += FJ_TEST(test for dyn-allocated parser and stack-allocated tokens) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        token tokens[10];
        auto *parser = alloc_parser(
             std::begin(tokens)
            ,std::end(tokens)
            ,str
            ,&my_alloc
            ,&my_free
        );

        auto toknum = parse(parser);

        assert(myallocator.allocations() == 1);
        auto total_allocated = myallocator.total_alloc();
        if ( sizeof(void *) == 4 ) {
            assert(total_allocated == 44);
        } else {
            assert(total_allocated == 80);
        }

        assert(is_valid(parser));
        assert(toknum == 7);
        assert(parser->toks_end == parser->toks_beg + toknum);

        assert(tokens[0].type == FJ_TYPE_OBJECT);
        assert(token_childs(&tokens[0]) == 6);
        assert(token_parent(&tokens[0]) == nullptr);

        assert(tokens[1].type == FJ_TYPE_BOOL);
        assert(token_key(&tokens[1]) == "a");
        assert(token_to_bool(&tokens[1]) == true);
        assert(token_value(&tokens[1]) == "true");
        assert(token_parent(&tokens[1]) == &tokens[0]);

        assert(tokens[2].type == FJ_TYPE_BOOL);
        assert(token_key(&tokens[2]) == "b");
        assert(token_to_bool(&tokens[2]) == false);
        assert(token_value(&tokens[2]) == "false");
        assert(token_parent(&tokens[2]) == &tokens[0]);

        assert(tokens[3].type == FJ_TYPE_NULL);
        assert(token_key(&tokens[3]) == "c");
        assert(token_value(&tokens[3]) == "null");
        assert(token_parent(&tokens[3]) == &tokens[0]);

        assert(tokens[4].type == FJ_TYPE_NUMBER);
        assert(token_key(&tokens[4]) == "d");
        assert(token_value(&tokens[4]) == "0");
        assert(token_to_int(&tokens[4]) == 0);
        assert(token_parent(&tokens[4]) == &tokens[0]);

        assert(tokens[5].type == FJ_TYPE_STRING);
        assert(token_key(&tokens[5]) == "e");
        assert(token_value(&tokens[5]) == "e");
        assert(token_to_string(&tokens[5]) == "e");
        assert(token_to_string_view(&tokens[5]) == "e");
        assert(token_parent(&tokens[5]) == &tokens[0]);

        assert(tokens[6].type == FJ_TYPE_OBJECT_END);
        assert(token_parent(&tokens[6]) == &tokens[0]);

        free_parser(parser);

        assert(myallocator.allocations() == 0);
        assert(myallocator.total_alloc() == 0);
    };

    // stack allocated parser and dyn allocated tokens
    test += FJ_TEST(test for stack-allocated parser and dyn-allocated tokens) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        auto parser = make_parser(
             str
            ,&my_alloc
            ,&my_free
        );

        auto toknum = parse(&parser);

        //myallocator.dump(std::cout);
        assert(myallocator.allocations() == 1);
        auto total_allocated = myallocator.total_alloc();
        if ( sizeof(void *) == 4 ) {
            assert(total_allocated == 168);
        } else {
            assert(total_allocated == 280);
        }

        assert(is_valid(&parser));
        assert(toknum == 7);
        assert(parser.toks_end == parser.toks_beg + toknum);

        auto *tokens = parser.toks_beg;
        assert(tokens[0].type == FJ_TYPE_OBJECT);
        assert(token_childs(&tokens[0]) == 6);
        assert(token_parent(&tokens[0]) == nullptr);

        assert(tokens[1].type == FJ_TYPE_BOOL);
        assert(token_key(&tokens[1]) == "a");
        assert(token_to_bool(&tokens[1]) == true);
        assert(token_value(&tokens[1]) == "true");
        assert(token_parent(&tokens[1]) == &tokens[0]);

        assert(tokens[2].type == FJ_TYPE_BOOL);
        assert(token_key(&tokens[2]) == "b");
        assert(token_to_bool(&tokens[2]) == false);
        assert(token_value(&tokens[2]) == "false");
        assert(token_parent(&tokens[2]) == &tokens[0]);

        assert(tokens[3].type == FJ_TYPE_NULL);
        assert(token_key(&tokens[3]) == "c");
        assert(token_value(&tokens[3]) == "null");
        assert(token_parent(&tokens[3]) == &tokens[0]);

        assert(tokens[4].type == FJ_TYPE_NUMBER);
        assert(token_key(&tokens[4]) == "d");
        assert(token_value(&tokens[4]) == "0");
        assert(token_to_int(&tokens[4]) == 0);
        assert(token_parent(&tokens[4]) == &tokens[0]);

        assert(tokens[5].type == FJ_TYPE_STRING);
        assert(token_key(&tokens[5]) == "e");
        assert(token_value(&tokens[5]) == "e");
        assert(token_to_string(&tokens[5]) == "e");
        assert(token_to_string_view(&tokens[5]) == "e");
        assert(token_parent(&tokens[4]) == &tokens[0]);

        assert(tokens[6].type == FJ_TYPE_OBJECT_END);
        assert(token_parent(&tokens[6]) == &tokens[0]);

        free_parser(&parser);

        assert(myallocator.allocations() == 0);
        assert(myallocator.total_alloc() == 0);
    };

    // dyn-allocated parser and dyn-allocated tokens
    test += FJ_TEST(test for dyn-allocated parser and dyn-allocated tokens) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        auto *parser = alloc_parser(
             str
            ,&my_alloc
            ,&my_free
        );

        auto toknum = parse(parser);

    //    myallocator.dump(std::cout);
    //    std::cout << "total: " << myallocator.total_alloc() << std::endl;
        assert(myallocator.allocations() == 2);
        auto total_allocated = myallocator.total_alloc();
        if ( sizeof(void *) == 4 ) {
            assert(total_allocated == 212);
        } else {
            assert(total_allocated == 360);
        }

        assert(is_valid(parser));
        assert(toknum == 7);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto *tokens = parser->toks_beg;
        assert(tokens[0].type == FJ_TYPE_OBJECT);
        assert(token_childs(&tokens[0]) == 6);
        assert(token_parent(&tokens[0]) == nullptr);

        assert(tokens[1].type == FJ_TYPE_BOOL);
        assert(token_key(&tokens[1]) == "a");
        assert(token_to_bool(&tokens[1]) == true);
        assert(token_value(&tokens[1]) == "true");
        assert(token_parent(&tokens[1]) == &tokens[0]);

        assert(tokens[2].type == FJ_TYPE_BOOL);
        assert(token_key(&tokens[2]) == "b");
        assert(token_to_bool(&tokens[2]) == false);
        assert(token_value(&tokens[2]) == "false");
        assert(token_parent(&tokens[2]) == &tokens[0]);

        assert(tokens[3].type == FJ_TYPE_NULL);
        assert(token_key(&tokens[3]) == "c");
        assert(token_value(&tokens[3]) == "null");
        assert(token_parent(&tokens[3]) == &tokens[0]);

        assert(tokens[4].type == FJ_TYPE_NUMBER);
        assert(token_key(&tokens[4]) == "d");
        assert(token_value(&tokens[4]) == "0");
        assert(token_to_int(&tokens[4]) == 0);
        assert(token_parent(&tokens[4]) == &tokens[0]);

        assert(tokens[5].type == FJ_TYPE_STRING);
        assert(token_key(&tokens[5]) == "e");
        assert(token_value(&tokens[5]) == "e");
        assert(token_to_string(&tokens[5]) == "e");
        assert(token_to_string_view(&tokens[5]) == "e");
        assert(token_parent(&tokens[4]) == &tokens[0]);

        assert(tokens[6].type == FJ_TYPE_OBJECT_END);
        assert(token_parent(&tokens[6]) == &tokens[0]);

        free_parser(parser);

        assert(myallocator.allocations() == 0);
        assert(myallocator.total_alloc() == 0);
    };

    test += FJ_TEST(test for case when lack of stack-allocated tokens) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        token tokens[4]{};
        auto parser = alloc_parser(std::begin(tokens), std::end(tokens), str);
        auto toknum = parse(parser);

        assert(!is_valid(parser));
        assert(toknum == 4);
        assert(num_tokens(parser) == 4);
        assert(get_error(parser) == flatjson::FJ_EC_NO_FREE_TOKENS);

        free_parser(parser);
    };

    test += FJ_TEST(test for end iterator pointing to the latest token which is ARRAY_END) {
        using namespace flatjson;

        static const char str[] = R"([{"c":3, "f":5}])";
        auto parser = make_parser(str);
        auto toknum = parse(&parser);

        assert(is_valid(&parser));
        assert(toknum == 6);
        assert(parser.toks_end == parser.toks_beg + toknum);

        auto beg = iter_begin(&parser);
        assert(beg.is_array());

        auto end = iter_end(&parser);
        assert(end.type() == FJ_TYPE_ARRAY_END);

        auto next = iter_next(beg);
        assert(iter_not_equal(next, end));
        assert(next.is_object());

        free_parser(&parser);
    };

    test += FJ_TEST(test for end iterator pointing to the latest token which is OBJECT_END) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":false, "c":null})";
        token tokens[10];
        auto parser = make_parser(std::begin(tokens), std::end(tokens), str);
        auto toknum = parse(&parser);

        assert(is_valid(&parser));
        assert(toknum == 5);
        assert(parser.toks_end == parser.toks_beg + toknum);

        auto beg = iter_begin(&parser);
        assert(beg.is_object());

        auto end = iter_end(&parser);
        assert(end.type() == FJ_TYPE_OBJECT_END);

        auto members = iter_members(beg);
        assert(members == 3);
    };

    test += FJ_TEST(test for iteration through the OBJECT using iterators) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        token tokens[10];
        auto parser = make_parser(std::begin(tokens), std::end(tokens), str);
        auto toknum = parse(&parser);

        assert(is_valid(&parser));
        assert(toknum == 7);
        assert(parser.toks_end == parser.toks_beg + toknum);

        auto beg = iter_begin(&parser);
        assert(beg.is_object());

        auto end = iter_end(&parser);
        assert(end.type() == FJ_TYPE_OBJECT_END);

        auto dist = iter_members(beg);
        assert(dist == 5);

        std::size_t idx = 0u;
        for ( auto it = iter_next(beg); iter_not_equal(it, end); it = iter_next(it) ) {
            idx = iter_distance(iter_next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.key() == "a");
                    assert(it.is_bool());
                    assert(it.value() == "true");
                    break;
                }
                case 1: {
                    assert(it.key() == "b");
                    assert(it.is_bool());
                    assert(it.value() == "false");
                    break;
                }
                case 2: {
                    assert(it.key() == "c");
                    assert(it.is_null());
                    assert(it.value() == "null");
                    break;
                }
                case 3: {
                    assert(it.key() == "d");
                    assert(it.is_number());
                    assert(it.value() == "0");
                    break;
                }
                case 4: {
                    assert(it.key() == "e");
                    assert(it.is_string());
                    assert(it.value() == "e");
                    break;
                }
                default: assert(!"unreachable!");
            }
        }
        assert(idx == 4);
    };

    test += FJ_TEST(test for iteration through the ARRAY using iterators) {
        using namespace flatjson;

        static const char str[] = R"([4,3,2,1])";
        token tokens[10];
        auto parser = make_parser(std::begin(tokens), std::end(tokens), str);
        auto toknum = parse(&parser);

        assert(is_valid(&parser));
        assert(toknum == 6);
        assert(parser.toks_end == parser.toks_beg + toknum);

        auto beg = iter_begin(&parser);
        assert(beg.is_array());

        auto end = iter_end(&parser);
        assert(end.type() == flatjson::FJ_TYPE_ARRAY_END);

        auto dist = iter_members(beg);
        assert(dist == 4);

        bool case_0 = false;
        bool case_1 = false;
        bool case_2 = false;
        bool case_3 = false;
        std::size_t idx = 0u;
        for ( auto it = iter_next(beg); iter_not_equal(it, end); it = iter_next(it) ) {
            idx = iter_distance(iter_next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.is_number());
                    case_0 = it.cur->type == FJ_TYPE_NUMBER && *(it.cur->val) == '4';
                    break;
                }
                case 1: {
                    assert(it.is_number());
                    case_1 = it.cur->type == FJ_TYPE_NUMBER && *(it.cur->val) == '3';
                    break;
                }
                case 2: {
                    assert(it.is_number());
                    case_2 = it.cur->type == FJ_TYPE_NUMBER && *(it.cur->val) == '2';
                    break;
                }
                case 3: {
                    assert(it.is_number());
                    case_3 = it.cur->type == FJ_TYPE_NUMBER && *(it.cur->val) == '1';
                    break;
                }
                default: assert(!"unreachable!");
            }
        }

        assert(idx == 3);
        assert(case_0);
        assert(case_1);
        assert(case_2);
        assert(case_3);

        free_parser(&parser);
    };

    test += FJ_TEST(test for iteration through the ARRAY of ARRAYS using iterators) {
        using namespace flatjson;

        static const char str[] = R"([[4],[3],[2],[1]])";
        token tokens[14];
        auto parser = make_parser(std::begin(tokens), std::end(tokens), str);
        auto toknum = parse(&parser);

        assert(is_valid(&parser));
        assert(toknum == 14);
        assert(parser.toks_end == parser.toks_beg + toknum);

        auto beg = iter_begin(&parser);
        assert(beg.is_array());

        auto end = iter_end(&parser);
        assert(end.type() == flatjson::FJ_TYPE_ARRAY_END);

        auto dist = iter_members(beg);
        assert(dist == 4);

        bool case_0 = false;
        bool case_1 = false;
        bool case_2 = false;
        bool case_3 = false;
        std::size_t idx = 0u;
        for ( auto it = iter_next(beg); iter_not_equal(it, end); it = iter_next(it) ) {
            idx = iter_distance(iter_next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.is_array());
                    case_0 = (it.cur+1)->type == FJ_TYPE_NUMBER && *((it.cur+1)->val) == '4';
                    break;
                }
                case 1: {
                    assert(it.is_array());
                    case_1 = (it.cur+1)->type == FJ_TYPE_NUMBER && *((it.cur+1)->val) == '3';
                    break;
                }
                case 2: {
                    assert(it.is_array());
                    case_2 = (it.cur+1)->type == FJ_TYPE_NUMBER && *((it.cur+1)->val) == '2';
                    break;
                }
                case 3: {
                    assert(it.is_array());
                    case_3 = (it.cur+1)->type == FJ_TYPE_NUMBER && *((it.cur+1)->val) == '1';
                    break;
                }
                default: assert(!"unreachable!");
            }
        }

        assert(idx == 3);
        assert(case_0);
        assert(case_1);
        assert(case_2);
        assert(case_3);

        free_parser(&parser);
    };

    test += FJ_TEST(test for access to the ARRAY values using search by index) {
        using namespace flatjson;

        static const char str[] = R"([0, "1", 3.14, -314])";
        token tokens[10]{};
        auto parser = make_parser(std::begin(tokens), std::end(tokens), str);
        auto toknum = parse(&parser);

        assert(is_valid(&parser));
        assert(toknum == 6);
        assert(parser.toks_end == parser.toks_beg + toknum);

        assert(num_tokens(&parser) == 6);
        assert(num_childs(&parser) == 4);
        assert(!is_empty(&parser));
        assert(!is_object(&parser));
        assert(is_array(&parser));
        assert(!is_null(&parser));
        assert(!is_string(&parser));
        assert(!is_bool(&parser));
        assert(!is_number(&parser));

        auto pbeg = iter_begin(&parser);
        assert(pbeg.is_array());
        auto dist = iter_members(pbeg);
        assert(dist == 4);

        auto it0 = iter_at(0, &parser);
        assert(it0.is_number());
        assert(it0.to_int() == 0);

        auto it1 = iter_at(1, &parser);
        assert(it1.is_string());
        assert(it1.to_string_view() == "1");

        auto it2 = iter_at(2, &parser);
        assert(it2.is_number());
        assert(it2.to_double() == 3.14);

        auto it3 = iter_at(3, &parser);
        assert(it3.is_number());
        assert(it3.to_int() == -314);
    };

    test += FJ_TEST(test for access to the OBJECT values using search by key) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":"false", "c":null})";
        token tokens[10];
        auto parser = make_parser(std::begin(tokens), std::end(tokens), str);
        auto toknum = parse(&parser);

        assert(is_valid(&parser));
        assert(toknum == 5);
        assert(parser.toks_end == parser.toks_beg + toknum);

        assert(num_tokens(&parser) == 5);
        assert(num_childs(&parser) == 3);
        assert(!is_empty(&parser));
        assert(is_object(&parser));
        assert(!is_array(&parser));
        assert(!is_null(&parser));
        assert(!is_string(&parser));
        assert(!is_bool(&parser));
        assert(!is_number(&parser));

        auto it0 = iter_at("a", &parser);
        assert(it0.is_bool());
        assert(it0.to_bool() == true);

        auto it1 = iter_at("b", &parser);
        assert(it1.is_string());
        assert(it1.to_string_view() == "false");

        auto it2 = iter_at("c", &parser);
        assert(it2.is_null());
        assert(it2.to_string_view() == "null");
    };

    test += FJ_TEST(test for access to the nested OBJECT values using search by key) {
        using namespace flatjson;

        static const char str[] = R"({"a":{"b":true, "c":1234}})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 6);
        assert(parser->toks_end == parser->toks_beg + toknum);

        assert(num_childs(parser) == 1);
        assert(is_object(parser));
        assert(!is_array(parser));
        assert(!is_number(parser));
        assert(!is_string(parser));
        assert(!is_null(parser));
        assert(!is_bool(parser));

        auto j0 = iter_at("a", parser);
        assert(j0.is_valid());
        assert(j0.members() == 2);
        assert(j0.is_object());
        assert(!j0.is_array());
        assert(!j0.is_number());
        assert(!j0.is_string());
        assert(!j0.is_null());
        assert(!j0.is_bool());

        auto j1 = iter_at("b", j0);
        assert(j1.is_valid());
        assert(j1.members() == 1);
        assert(j1.is_bool());
        assert(j1.to_bool() == true);
        assert(!j1.is_array());
        assert(!j1.is_number());
        assert(!j1.is_string());
        assert(!j1.is_null());
        assert(!j1.is_object());

        auto j2 = iter_at("c", j0);
        assert(j2.is_valid());
        assert(j2.members() == 1);
        assert(j2.is_number());
        assert(j2.to_int() == 1234);
        assert(!j2.is_array());
        assert(!j2.is_string());
        assert(!j2.is_null());
        assert(!j2.is_object());
        assert(!j2.is_bool());

        auto j3 = iter_at("d", j0);
        assert(iter_equal(j3, iter_end(j0)));

        free_parser(parser);
    };

    test += FJ_TEST(test for iterate through ARRAY with nested OBJECTS) {
        using namespace flatjson;

        static const char str[] = R"({"a":[4,3,2,1], "b":[{"a":0,"b":1,"c":2},{"b":4,"a":3,"c":5},{"c":8,"b":7,"a":6}], "c":[0,1,2,3]})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 31);
        assert(parser->toks_end == parser->toks_beg + toknum);

//        details::fj_dump_tokens(stdout, "", parser);

        auto pbeg = iter_begin(parser);
        auto pend = iter_end(parser);
        assert(pend.type() == flatjson::FJ_TYPE_OBJECT_END);
        auto pdist = iter_members(pbeg);
        assert(pdist == 3);

        auto a = iter_at("a", parser);
        assert(a.is_valid());
        assert(a.members() == 4);
        assert(!a.is_object());
        assert(a.is_array());
        assert(!a.is_number());
        assert(!a.is_string());
        assert(!a.is_null());
        assert(!a.is_bool());

        auto abeg = iter_begin(a);
        auto aend = iter_end(a);
        assert(aend.type() == flatjson::FJ_TYPE_ARRAY_END);
        auto adist = iter_members(abeg);
        assert(adist == 4);

        auto b = iter_at("b", parser);
        assert(b.is_valid());
        assert(b.members() == 3);
        assert(!b.is_object());
        assert(b.is_array());
        assert(!b.is_number());
        assert(!b.is_string());
        assert(!b.is_null());
        assert(!b.is_bool());

        auto bbeg = iter_begin(b);
        auto bend = iter_end(b);
        auto bdist = iter_members(bbeg);
        assert(bdist == 3);

        bool dist_0 = false;
        bool dist_1 = false;
        bool dist_2 = false;
        std::size_t idx = 0u;
        for ( auto it = iter_next(bbeg); iter_not_equal(it, bend); it = iter_next(it) ) {
            idx = iter_distance(iter_next(bbeg), it);
            switch ( idx ) {
                case 0: {
                    auto a0 = iter_at("a", it);
                    assert(iter_not_equal(a0, it));
                    assert(a0.is_valid());
                    assert(a0.members() == 1);
                    assert(a0.is_number());
                    assert(a0.value() == "0");

                    auto b0 = iter_at("b", it);
                    assert(iter_not_equal(b0, it));
                    assert(b0.is_valid());
                    assert(b0.members() == 1);
                    assert(b0.is_number());
                    assert(b0.value() == "1");

                    auto c0 = iter_at("c", it);
                    assert(iter_not_equal(c0, it));
                    assert(c0.is_valid());
                    assert(c0.members() == 1);
                    assert(c0.is_number());
                    assert(c0.value() == "2");

                    dist_0 = true;
                    break;
                }
                case 1: {
                    auto a1 = iter_at("a", it);
                    assert(iter_not_equal(a1, it));
                    assert(a1.is_valid());
                    assert(a1.members() == 1);
                    assert(a1.is_number());
                    assert(a1.value() == "3");

                    auto b1 = iter_at("b", it);
                    assert(iter_not_equal(b1, it));
                    assert(b1.is_valid());
                    assert(b1.members() == 1);
                    assert(b1.is_number());
                    assert(b1.value() == "4");

                    auto c1 = iter_at("c", it);
                    assert(iter_not_equal(c1, it));
                    assert(c1.is_valid());
                    assert(c1.members() == 1);
                    assert(c1.is_number());
                    assert(c1.value() == "5");

                    dist_1 = true;
                    break;
                }
                case 2: {
                    auto a2 = iter_at("a", it);
                    assert(iter_not_equal(a2, it));
                    assert(a2.is_valid());
                    assert(a2.members() == 1);
                    assert(a2.is_number());
                    assert(a2.value() == "6");

                    auto b2 = iter_at("b", it);
                    assert(iter_not_equal(b2, it));
                    assert(b2.is_valid());
                    assert(b2.members() == 1);
                    assert(b2.is_number());
                    assert(b2.value() == "7");

                    auto c2 = iter_at("c", it);
                    assert(iter_not_equal(c2, it));
                    assert(c2.is_valid());
                    assert(c2.members() == 1);
                    assert(c2.is_number());
                    assert(c2.value() == "8");

                    dist_2 = true;
                    break;
                }
                default:
                    assert(!"unreachable!");
            }
        }

        assert(idx == 2);
        assert(dist_0);
        assert(dist_1);
        assert(dist_2);

        free_parser(parser);
    };

    test += FJ_TEST(test for iterate using index through ARRAY which is value of OBJECT) {
        using namespace flatjson;

        static const char str[] = R"({"a":[0,1,2]})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 7);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_object());
        auto end = iter_end(parser);
        assert(end.type() == FJ_TYPE_OBJECT_END);

        auto j0 = iter_at("a", beg);
        assert(iter_not_equal(j0, end));
        assert(j0.members() == 3);
        assert(j0.is_array());
        assert(!j0.is_object());
        assert(!j0.is_number());
        assert(!j0.is_string());
        assert(!j0.is_null());
        assert(!j0.is_bool());
        for ( auto idx = 0u; idx < j0.members(); ++idx ) {
            auto item = iter_at(idx, j0);
            assert(item.is_simple_type());
            assert(item.to_uint() == idx);
        }

        free_parser(parser);
    };

    test += FJ_TEST(test for iterate using iterator through ARRAY which is value of OBJECT) {
        using namespace flatjson;

        static const char str[] = R"({"a":[0,1,2]})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 7);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        assert(beg.is_object());
        auto end = iter_end(parser);
        assert(end.type() == FJ_TYPE_OBJECT_END);

        auto j0 = iter_at("a", beg);
        assert(iter_not_equal(j0, end));
        assert(j0.members() == 3);
        assert(j0.is_array());
        assert(!j0.is_object());
        assert(!j0.is_number());
        assert(!j0.is_string());
        assert(!j0.is_null());
        assert(!j0.is_bool());

        auto j0end = iter_end(j0);
        auto idx = 0u;
        for ( auto it = iter_next(j0); iter_not_equal(it, j0end); it = iter_next(it), ++idx ) {
            assert(it.is_simple_type());
            assert(it.to_uint() == idx);
        }

        free_parser(parser);
    };

    test += FJ_TEST(test for iterate through OBJECT with excluded nested OBJECTS) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 9);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        auto end = iter_end(parser);
        assert(beg.members() == 4);

        bool case_0 = false;
        bool case_1 = false;
        bool case_2 = false;
        bool case_3 = false;
        std::size_t idx = 0u;
        for ( auto it = iter_next(beg); iter_not_equal(it, end); it = iter_next(it) ) {
            idx = iter_distance(iter_next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.key() == "a");
                    assert(it.is_number());
                    assert(it.value() == "0");
                    case_0 = true;
                    break;
                }
                case 1: {
                    assert(it.key() == "b");
                    assert(it.is_number());
                    assert(it.value() == "1");
                    case_1 = true;
                    break;
                }
                case 2: {
                    assert(it.key() == "c");
                    assert(it.is_object());
                    assert(it.value() == "");
                    case_2 = true;
                    break;
                }
                case 3: {
                    assert(it.key() == "f");
                    assert(it.is_number());
                    assert(it.value() == "4");
                    case_3 = true;
                    break;
                }
                default: assert(!"unreachable!");
            }
        }

        assert(idx == 3);
        assert(case_0);
        assert(case_1);
        assert(case_2);
        assert(case_3);

        free_parser(parser);
    };

    test += FJ_TEST(test for iterate through OBJECT including nested OBJECT) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 9);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        auto end = iter_end(parser);
        assert(beg.members() == 4);

        bool case_0 = false;
        bool case_1 = false;
        bool case_2 = false;
        bool case_2_0 = false;
        bool case_2_1 = false;
        bool case_3 = false;
        std::size_t idx = 0u;
        for ( auto it = iter_next(beg); iter_not_equal(it, end); it = iter_next(it) ) {
            idx = iter_distance(iter_next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.key() == "a");
                    assert(it.value() == "0");
                    case_0 = true;
                    break;
                }
                case 1: {
                    assert(it.key() == "b");
                    assert(it.value() == "1");
                    case_1 = true;
                    break;
                }
                case 2: {
                    assert(it.key() == "c");
                    assert(it.is_object());
                    assert(it.childs() == 3);
                    auto cbeg = iter_begin(it);
                    auto cend = iter_end(it);
                    auto idx2 = 0u;
                    for ( auto cit = iter_next(cbeg); iter_not_equal(cit, cend); cit = iter_next(cit) ) {
                        idx2 = iter_distance(iter_next(cbeg), cit);
                        switch ( idx2 ) {
                            case 0: {
                                assert(cit.key() == "d");
                                assert(cit.value() == "2");
                                case_2_0 = true;
                                break;
                            }
                            case 1: {
                                assert(cit.key() == "e");
                                assert(cit.value() == "3");
                                case_2_1 = true;
                                break;
                            }
                            default: assert(!"unreachable");
                        }
                    }

                    assert(idx2 == 1);
                    case_2 = true;
                    break;
                }
                case 3: {
                    assert(it.key() == "f");
                    assert(it.value() == "4");
                    case_3 = true;
                    break;
                }
                default: assert(!"unreachable");
            }
        }

        assert(idx == 3);
        assert(case_0);
        assert(case_1);
        assert(case_2);
        assert(case_2_0);
        assert(case_2_1);
        assert(case_3);

        free_parser(parser);
    };

    test += FJ_TEST(test for serialization to std::ostringstream) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":{"c":{"d":1, "e":2}}, "c":[0,1,2,3]})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 15);
        assert(parser->toks_end == parser->toks_beg + toknum);

        static const char *expected =
R"({
    "a":true,
    "b":{
        "c":{
            "d":1,
            "e":2
        }
    },
    "c":[
        0,
        1,
        2,
        3
    ]
})";

        auto beg = iter_begin(parser);
        auto end = iter_end(parser);
        auto strlen = length_for_string(beg, end, 4);
        assert(strlen == 154);

        std::ostringstream os;
        auto strlen2 = serialize(os, beg, end, 4);
        assert(strlen2 == 154);

        auto ss = os.str();
        if ( ss != expected ) {
            std::cout
                << "expected: " << expected << std::endl
                << "got: " << ss
            << std::endl;
        }

        assert(os.str() == expected);

        free_parser(parser);
    };

    test += FJ_TEST(test for serialization to std::string) {
        using namespace flatjson;

        static const char str[] = R"({"a":true, "b":{"c":{"d":1, "e":2}}, "c":[0,1,2,3]})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 15);
        assert(parser->toks_end == parser->toks_beg + toknum);

        static const char *expected =
R"({
    "a":true,
    "b":{
        "c":{
            "d":1,
            "e":2
        }
    },
    "c":[
        0,
        1,
        2,
        3
    ]
})";

        auto beg = iter_begin(parser);
        auto end = iter_end(parser);
        auto strlen = length_for_string(beg, end, 4);
        assert(strlen == 154);

        auto sstr = to_string(beg, end, 4);
        assert(sstr.length() == 154);
        if ( expected != sstr ) {
            std::cout
                << "expected: " << expected << std::endl
                << "got: " << sstr
            << std::endl;
        }
        assert(sstr == expected);

        free_parser(parser);
    };

    test += FJ_TEST(test for serialization to std::FILE *) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 9);
        assert(parser->toks_end == parser->toks_beg + toknum);

        static const char *fname = "unit_27.json";

        std::FILE *stream = nullptr;
#ifdef _MSC_VER
        fopen_s(&stream, fname, "w");
#else
        stream = std::fopen(fname, "w");
#endif // _MSC_VER
        assert(stream);

        auto beg = iter_begin(parser);
        auto end = iter_end(parser);
        serialize(stream, beg, end, 4);

        std::fclose(stream);

        auto from_file = read_file(fname);
        assert(!from_file.empty());
        std::remove(fname);

        auto string = to_string(beg, end, 4);
        if ( from_file != string ) {
            std::cout
                << "expected: " << from_file << std::endl
                << "got: " << string
            << std::endl;
        }
        assert(from_file == string);

        free_parser(parser);
    };

#ifndef _MSC_VER
    test += FJ_TEST(test for serialization to file descriptor) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 9);
        assert(parser->toks_end == parser->toks_beg + toknum);

        static const char *fname = "unit_28.json";

        auto fd = ::open(fname, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
        assert(fd != -1);

        auto beg = iter_begin(parser);
        auto end = iter_end(parser);
        serialize(fd, beg, end, 4);

        ::close(fd);

        auto from_file = read_file(fname);
        assert(!from_file.empty());
        std::remove(fname);

        auto string = to_string(beg, end, 4);
        if ( from_file != string ) {
            std::cout
                << "expected: " << from_file << std::endl
                << "got: " << string
            << std::endl;
        }
        assert(from_file == string);

        free_parser(parser);
    };
#endif // _MSC_VER

    test += FJ_TEST(test for serialization to file_handle) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 9);
        assert(parser->toks_end == parser->toks_beg + toknum);

        static const char *fname = "unit_28.json";

        int ec{};
        auto fh = file_create(fname, &ec);
        assert(file_handle_valid(fh) && ec == 0);

        auto beg = iter_begin(parser);
        auto end = iter_end(parser);
        serialize(fh, beg, end, 4);

        assert(file_close(fh, &ec) && ec == 0);

        auto from_file = read_file(fname);
        assert(!from_file.empty());
        std::remove(fname);

        auto string = to_string(beg, end, 4);
        if ( from_file != string ) {
            std::cout
                << "expected: " << from_file << std::endl
                << "got: " << string
            << std::endl;
        }
        assert(from_file == string);

        free_parser(parser);
    };

    test += FJ_TEST(test for fj_walk_through_keys() for collecting keys) {
        using namespace flatjson;

        static const auto cb = [](void *userdata, const char *ptr, std::size_t len) {
            std::size_t &cnt = *static_cast<std::size_t *>(userdata);

            assert(len == 1);

            if ( cnt == 0 ) {
                assert(std::strncmp(ptr, "a", 1) == 0);
            } else if ( cnt == 1 ) {
                assert(std::strncmp(ptr, "b", 1) == 0);
            } else {
                assert(!"unreachable!");
            }

            ++cnt;
        };

        static const char str[] = R"({"a":0, "b":1})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 4);
        assert(parser->toks_end == parser->toks_beg + toknum);

        auto beg = iter_begin(parser);
        auto end = iter_end(parser);
        std::size_t calls_cnt{};
        std::size_t num = details::walk_through_keys(beg, end, &calls_cnt, cb);
        assert(num == 2);
        assert(calls_cnt == 2);

        bool case_0 = false;
        bool case_1 = false;
        auto keys = get_keys(beg, end);
        assert(num == keys.size());
        for ( auto it = keys.begin(); it != keys.end(); ++it ) {
            auto dist = std::distance(keys.begin(), it);
            switch ( dist ) {
                case 0: { assert(*it == "a"); case_0 = true; break; }
                case 1: { assert(*it == "b"); case_1 = true; break; }
                default:{ assert(!"unreachable!"); }
            }
        }

        assert(case_0);
        assert(case_1);

        free_parser(parser);
    };

    test += FJ_TEST(test for empty c++ fjson object constructor) {
        using namespace flatjson;

        fjson json;
        assert(!json.is_valid());
        assert(json.begin() == json.end());
    };

    test += FJ_TEST(test for empty ARRAY for c++ fjson object constructor) {
        using namespace flatjson;

        fjson json{"[]"};
        assert(json.is_valid());
        assert(json.tokens() == 2);
        auto beg = json.begin();
        auto end = json.end();
        assert(beg != end);
    };

    test += FJ_TEST(test for c++ fjson object constructor using a ready parser object) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser = alloc_parser(str);
        auto toknum = parse(parser);

        assert(is_valid(parser));
        assert(toknum == 9);
        assert(parser->toks_end == parser->toks_beg + toknum);

        fjson json{parser};
        assert(json.is_valid());
        assert(json.begin() != json.end());
        assert(json.tokens() == 9);

        free_parser(parser);
    };

    test += FJ_TEST(test for c++ fjson object constructor using pointers to the first and latest char) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        token toks[10];

        fjson json{std::begin(toks), std::end(toks), str};
        assert(json.is_valid());
        assert(json.begin() != json.end());
        assert(json.tokens() == 9);
    };

    test += FJ_TEST(test for c++ fjson object constructor using array of chars) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        fjson json{str};
        assert(json.is_valid());
        assert(json.begin() != json.end());
        assert(json.tokens() == 9);
        assert(json.members() == 4);
    };

    test += FJ_TEST(test for c++ fjson object used for access values) {
        using namespace flatjson;

        static const char str[] = R"({"a":[4,3,2,1], "b":[{"a":0,"b":1,"c":2},{"b":4,"a":3,"c":5},{"c":8,"b":7,"a":6}], "c":[0,1,2,3]})";
        fjson json{str};

        assert(json.is_valid());
        assert(json.tokens() == 31);

        assert(json.members() == 3);

        auto a = json.at("a");
        assert(a.is_valid());
        assert(a.type() == flatjson::FJ_TYPE_ARRAY);
        assert(a.members() == 4);
        assert(!a.is_object());
        assert(a.is_array());
        assert(!a.is_number());
        assert(!a.is_string());
        assert(!a.is_null());
        assert(!a.is_bool());

        auto b = json.at("b");
        assert(b.is_valid());
        assert(b.type() == flatjson::FJ_TYPE_ARRAY);
        assert(b.members() == 3);
        assert(!b.is_object());
        assert(b.is_array());
        assert(!b.is_number());
        assert(!b.is_string());
        assert(!b.is_null());
        assert(!b.is_bool());

        auto bbeg = b.begin();
        auto bend = b.end();

        bool dist_0 = false;
        bool dist_1 = false;
        bool dist_2 = false;
        std::size_t idx = 0u;
        for ( auto it = ++bbeg; it != bend; ++it ) {
            idx = distance(bbeg, it);
            switch ( idx ) {
                case 0: {
                    assert(it->is_object());
                    fjson j0 = json.at(it);
                    assert(j0.is_valid());
                    assert(j0.is_object());
                    assert(j0.members() == 3);

                    auto a0 = j0.at("a");
                    assert(a0.is_valid());
                    assert(!a0.is_object());
                    assert(!a0.is_array());
                    assert(a0.is_number());
                    assert(!a0.is_string());
                    assert(!a0.is_null());
                    assert(!a0.is_bool());
                    assert(a0.members() == 1);
                    assert(a0.to_string_view() == "0");
                    assert(a0.to_int() == 0);

                    auto b0 = j0.at("b");
                    assert(b0.is_valid());
                    assert(!b0.is_object());
                    assert(!b0.is_array());
                    assert(b0.is_number());
                    assert(!b0.is_string());
                    assert(!b0.is_null());
                    assert(!b0.is_bool());
                    assert(b0.members() == 1);
                    assert(b0.to_string_view() == "1");
                    assert(b0.to_int() == 1);

                    auto c0 = j0.at("c");
                    assert(c0.is_valid());
                    assert(!c0.is_object());
                    assert(!c0.is_array());
                    assert(c0.is_number());
                    assert(!c0.is_string());
                    assert(!c0.is_null());
                    assert(!c0.is_bool());
                    assert(c0.members() == 1);
                    assert(c0.to_string_view() == "2");
                    assert(c0.to_int() == 2);

                    dist_0 = true;
                    break;
                }
                case 1: {
                    assert(it->is_object());
                    fjson j1 = json.at(it);
                    assert(j1.is_valid());
                    assert(j1.is_object());
                    assert(j1.members() == 3);

                    auto a1 = j1.at("a");
                    assert(a1.is_valid());
                    assert(!a1.is_object());
                    assert(!a1.is_array());
                    assert(a1.is_number());
                    assert(!a1.is_string());
                    assert(!a1.is_null());
                    assert(!a1.is_bool());
                    assert(a1.members() == 1);
                    assert(a1.to_string_view() == "3");
                    assert(a1.to_int() == 3);

                    auto b1 = j1.at("b");
                    assert(b1.is_valid());
                    assert(!b1.is_object());
                    assert(!b1.is_array());
                    assert(b1.is_number());
                    assert(!b1.is_string());
                    assert(!b1.is_null());
                    assert(!b1.is_bool());
                    assert(b1.members() == 1);
                    assert(b1.to_string_view() == "4");
                    assert(b1.to_int() == 4);

                    auto c1 = j1.at("c");
                    assert(c1.is_valid());
                    assert(!c1.is_object());
                    assert(!c1.is_array());
                    assert(c1.is_number());
                    assert(!c1.is_string());
                    assert(!c1.is_null());
                    assert(!c1.is_bool());
                    assert(c1.members() == 1);
                    assert(c1.to_string_view() == "5");
                    assert(c1.to_int() == 5);

                    dist_1 = true;
                    break;
                }
                case 2: {
                    assert(it->is_object());
                    fjson j2 = json.at(it);
                    assert(j2.is_valid());
                    assert(j2.is_object());
                    assert(j2.members() == 3);

                    auto a2 = j2.at("a");
                    assert(a2.is_valid());
                    assert(!a2.is_object());
                    assert(!a2.is_array());
                    assert(a2.is_number());
                    assert(!a2.is_string());
                    assert(!a2.is_null());
                    assert(!a2.is_bool());
                    assert(a2.members() == 1);
                    assert(a2.to_string_view() == "6");
                    assert(a2.to_int() == 6);

                    auto b2 = j2.at("b");
                    assert(b2.is_valid());
                    assert(!b2.is_object());
                    assert(!b2.is_array());
                    assert(b2.is_number());
                    assert(!b2.is_string());
                    assert(!b2.is_null());
                    assert(!b2.is_bool());
                    assert(b2.members() == 1);
                    assert(b2.to_string_view() == "7");
                    assert(b2.to_int() == 7);

                    auto c2 = j2.at("c");
                    assert(c2.is_valid());
                    assert(!c2.is_object());
                    assert(!c2.is_array());
                    assert(c2.is_number());
                    assert(!c2.is_string());
                    assert(!c2.is_null());
                    assert(!c2.is_bool());
                    assert(c2.members() == 1);
                    assert(c2.to_string_view() == "8");
                    assert(c2.to_int() == 8);

                    dist_2 = true;
                    break;
                }
                default:
                    assert(!"unreachable!");
            }
        }

        assert(idx == 2);
        assert(dist_0);
        assert(dist_1);
        assert(dist_2);
    };

    test += FJ_TEST(test for equality for compare(markup)) {
        using namespace flatjson;

        static const char str0[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser0 = alloc_parser(str0);
        auto toknum0 = parse(parser0);

        assert(is_valid(parser0));
        assert(toknum0 == 9);
        assert(parser0->toks_end == parser0->toks_beg + toknum0);

        static const char str1[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser1 = alloc_parser(str1);
        auto toknum1 = parse(parser1);

        assert(is_valid(parser1));
        assert(toknum1 == 9);
        assert(parser1->toks_end == parser1->toks_beg + toknum1);

        iterator ldiff, rdiff;
        auto r = compare(&ldiff, &rdiff, parser0, parser1);
        assert(r == compare_result::OK);
        assert(std::strcmp(compare_result_string(r), "OK") == 0);

        free_parser(parser1);
        free_parser(parser0);
    };

    test += FJ_TEST(test for equality for the same JSON but with reordered key for compare(markup)) {
        using namespace flatjson;

        static const char str0[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser0 = alloc_parser(str0);
        auto toknum0 = parse(parser0);

        assert(is_valid(parser0));
        assert(toknum0 == 9);
        assert(parser0->toks_end == parser0->toks_beg + toknum0);

        static const char str1[] = R"({"b":1, "a":0, "c":{"d":2, "e":3}, "f":4})";
        auto *parser1 = alloc_parser(str1);
        auto toknum1 = parse(parser1);

        assert(is_valid(parser1));
        assert(toknum1 == 9);
        assert(parser1->toks_end == parser1->toks_beg + toknum1);

        iterator ldiff, rdiff;
        auto r = compare(&ldiff, &rdiff, parser0, parser1);
        assert(r == compare_result::OK);
        assert(std::strcmp(compare_result_string(r), "OK") == 0);

        free_parser(parser1);
        free_parser(parser0);
    };

    test += FJ_TEST(test 2 for equality for the same JSON but with reordered key for compare(markup)) {
        using namespace flatjson;

        static const char str0[] = R"({"b":1, "a":0, "c":{"d":2, "e":3}, "f":4})";
        auto *parser0 = alloc_parser(str0);
        auto toknum0 = parse(parser0);

        assert(is_valid(parser0));
        assert(toknum0 == 9);
        assert(parser0->toks_end == parser0->toks_beg + toknum0);

        static const char str1[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser1 = alloc_parser(str1);
        auto toknum1 = parse(parser1);

        assert(is_valid(parser1));
        assert(toknum1 == 9);
        assert(parser1->toks_end == parser1->toks_beg + toknum1);

        iterator ldiff, rdiff;
        auto r = compare(&ldiff, &rdiff, parser0, parser1);
        assert(r == compare_result::OK);
        assert(std::strcmp(compare_result_string(r), "OK") == 0);

        free_parser(parser1);
        free_parser(parser0);
    };

    test += FJ_TEST(test 3 for equality for the same JSON but with reordered key for compare(markup)) {
        using namespace flatjson;

        static const char str0[] = R"({"b":1, "a":0, "c":{"d":2, "e":3}, "f":4})";
        auto *parser0 = alloc_parser(str0);
        auto toknum0 = parse(parser0);

        assert(is_valid(parser0));
        assert(toknum0 == 9);
        assert(parser0->toks_end == parser0->toks_beg + toknum0);

        static const char str1[] = R"({"a":0, "c":{"d":2, "e":3}, "b":1, "f":4})";
        auto *parser1 = alloc_parser(str1);
        auto toknum1 = parse(parser1);

        assert(is_valid(parser1));
        assert(toknum1 == 9);
        assert(parser1->toks_end == parser1->toks_beg + toknum1);

        iterator ldiff, rdiff;
        auto r = compare(&ldiff, &rdiff, parser0, parser1);
        assert(r == compare_result::OK);
        assert(std::strcmp(compare_result_string(r), "OK") == 0);

        free_parser(parser1);
        free_parser(parser0);
    };

    test += FJ_TEST(test for a different keys for compare(markup)) {
        using namespace flatjson;

        static const char str0[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser0 = alloc_parser(str0);
        auto toknum0 = parse(parser0);

        assert(is_valid(parser0));
        assert(toknum0 == 9);
        assert(parser0->toks_end == parser0->toks_beg + toknum0);

        static const char str1[] = R"({"g":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser1 = alloc_parser(str1);
        auto toknum1 = parse(parser1);

        assert(is_valid(parser1));
        assert(toknum1 == 9);
        assert(parser1->toks_end == parser1->toks_beg + toknum1);

        iterator ldiff, rdiff;
        auto r = compare(&ldiff, &rdiff, parser0, parser1);
        assert(r == compare_result::no_key);
        assert(std::strcmp(compare_result_string(r), "no required key") == 0);

        assert(ldiff.key() == "a");
        assert(iter_at("a", parser0).cur == ldiff.cur);

        free_parser(parser1);
        free_parser(parser0);
    };

    test += FJ_TEST(test for a different values but with the same length for compare(length_only)) {
        using namespace flatjson;

        static const char str0[] = R"({"a":0, "b":12, "c":{"d":2, "e":3}, "f":4})";
        auto *parser0 = alloc_parser(str0);
        auto toknum0 = parse(parser0);

        assert(is_valid(parser0));
        assert(toknum0 == 9);
        assert(parser0->toks_end == parser0->toks_beg + toknum0);

        static const char str1[] = R"({"a":0, "b":11, "c":{"d":2, "e":3}, "f":4})";
        auto *parser1 = alloc_parser(str1);
        auto toknum1 = parse(parser1);

        assert(is_valid(parser1));
        assert(toknum1 == 9);
        assert(parser1->toks_end == parser1->toks_beg + toknum1);

        iterator ldiff, rdiff;
        auto r = compare(&ldiff, &rdiff, parser0, parser1, compare_mode::length_only);
        assert(r == compare_result::OK);

        free_parser(parser1);
        free_parser(parser0);
    };

    test += FJ_TEST(test for a different values for compare(full)) {
        using namespace flatjson;

        static const char str0[] = R"({"a":0, "b":12, "c":{"d":2, "e":3}, "f":4})";
        auto *parser0 = alloc_parser(str0);
        auto toknum0 = parse(parser0);

        assert(is_valid(parser0));
        assert(toknum0 == 9);
        assert(parser0->toks_end == parser0->toks_beg + toknum0);

        static const char str1[] = R"({"a":0, "b":11, "c":{"d":2, "e":3}, "f":4})";
        auto *parser1 = alloc_parser(str1);
        auto toknum1 = parse(parser1);

        assert(is_valid(parser1));
        assert(toknum1 == 9);
        assert(parser1->toks_end == parser1->toks_beg + toknum1);

        iterator ldiff, rdiff;
        auto r = compare(&ldiff, &rdiff, parser0, parser1, compare_mode::full);
        assert(r == compare_result::value);
        assert(std::strcmp(compare_result_string(r), "value do not match") == 0);

        assert(ldiff.key() == "b");
        assert(iter_at("b", parser0).cur == ldiff.cur);
        assert(rdiff.key() == "b");
        assert(iter_at("b", parser1).cur == rdiff.cur);

        free_parser(parser1);
        free_parser(parser0);
    };

    test += FJ_TEST(test for a different length JSON for compare(markup)) {
        using namespace flatjson;

        static const char str0[] = R"({"a":0, "b":12, "c":{"d":2, "e":3}, "f":4})";
        auto *parser0 = alloc_parser(str0);
        auto toknum0 = parse(parser0);

        assert(is_valid(parser0));
        assert(toknum0 == 9);
        assert(parser0->toks_end == parser0->toks_beg + toknum0);

        static const char str1[] = R"({"a":0, "b":11, "c":{"d":2, "e":3}, "f":4, "g":5})";
        auto *parser1 = alloc_parser(str1);
        auto toknum1 = parse(parser1);

        assert(is_valid(parser1));
        assert(toknum1 == 10);
        assert(parser1->toks_end == parser1->toks_beg + toknum1);

        iterator ldiff, rdiff;
        auto r = compare(&ldiff, &rdiff, parser0, parser1);
        assert(r == compare_result::longer);
        assert(std::strcmp(compare_result_string(r), "the right-side JSON is longer") == 0);

        free_parser(parser1);
        free_parser(parser0);
    };

    test += FJ_TEST(test for a different length JSON for compare(markup)) {
        using namespace flatjson;

        static const char str0[] = R"({"a":0, "b":12, "c":{"d":2, "e":3}, "f":4})";
        auto *parser0 = alloc_parser(str0);
        auto toknum0 = parse(parser0);

        assert(is_valid(parser0));
        assert(toknum0 == 9);
        assert(parser0->toks_end == parser0->toks_beg + toknum0);

        static const char str1[] = R"({"a":0, "b":11, "c":{"d":2, "e":3}})";
        auto *parser1 = alloc_parser(str1);
        auto toknum1 = parse(parser1);

        assert(is_valid(parser1));
        assert(toknum1 == 8);
        assert(parser1->toks_end == parser1->toks_beg + toknum1);

        iterator ldiff, rdiff;
        auto r = compare(&ldiff, &rdiff, parser0, parser1);
        assert(r == compare_result::shorter);
        assert(std::strcmp(compare_result_string(r), "the right-side JSON is shorter") == 0);

        free_parser(parser1);
        free_parser(parser0);
    };

    test += FJ_TEST(test for fj_packed_state_size()) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":12, "c":{"d":2, "e":3}, "f":4})";
        auto parser = make_parser(str);
        auto toknum = parse(&parser);
        assert(is_valid(&parser));
        assert(toknum == 9);

        auto size = packed_state_size(&parser);
        assert(size == 122);

        free_parser(&parser);
    };

    test += FJ_TEST(test for fj_pack_state()) {
        using namespace flatjson;

        static const char str[] = R"({"a":0, "b":12, "c":{"d":2, "e":3}, "f":4})";
        auto parser = make_parser(str);
        auto toknum = parse(&parser);
        assert(is_valid(&parser));
        assert(toknum == 9);

        auto size = packed_state_size(&parser);

        char *ptr = static_cast<char *>(parser.alloc_fn(size));
        auto writen = pack_state(ptr, size, &parser);
        assert(size == writen);

        auto parser2 = init_parser();
        auto ok = unpack_state(&parser2, ptr, size);
        assert(ok);
        assert(is_valid(&parser2));
        assert(num_tokens(&parser2) == toknum);

        iterator left_diff, right_diff;
        auto res = compare(&left_diff, &right_diff, &parser, &parser2, compare_mode::full);
        assert(res == compare_result::OK);

        parser.free_fn(ptr);
        free_parser(&parser2);
        free_parser(&parser);
    };

    /*********************************************************************************************/

    test.run();

    return EXIT_SUCCESS;
}

/*************************************************************************************************/

#ifdef _MSC_VER
#pragma warning(pop)
#endif
