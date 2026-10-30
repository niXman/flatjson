
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

#include <iostream>

#include <iterator>

#define FJ_RUN_TEST(test) \
    test(); std::cout << "test \"" << #test "\" passed!" << std::endl;

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
    token tokens[12];
    auto parser = make_parser(
         args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    auto toknum = parse(&parser, args.json_beg = std::begin(str), args.json_end = end(str));
    assert(is_valid(&parser));
    assert(toknum == 12);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_object_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    token tokens[12];
    auto parser = make_parser(
         args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    auto toknum = parse(&parser, args.json_beg = std::begin(str), args.json_end = end(str));
    assert(is_valid(&parser));
    assert(toknum == 12);

    auto beg = begin(&parser);
    assert(beg.is_object());

    auto end = flatjson::end(&parser);
    for (auto it = next(beg); not_equal(it, end); it = next(it) ) {
        std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_array() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";
    token tokens[17];
    auto parser = make_parser(
         args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    auto toknum = parse(&parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(&parser));
    assert(toknum == 6);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_array_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    token tokens[10];
    auto parser = make_parser(
         args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    auto toknum = parse(&parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(&parser));
    assert(toknum == 6);

    auto beg = begin(&parser);
    assert(beg.is_array());

    auto end = flatjson::end(&parser);
    for (auto it = next(beg); not_equal(it, end); it = next(it) ) {
        std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_object_and_iteration_on_nested_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":{"f":false, "g":3, "h":"4"}, "e":"e"})";

    token tokens[19];
    auto parser = make_parser(
         args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    auto toknum = parse(&parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(&parser));
    assert(toknum == 19);

    auto beg = begin(&parser);
    assert(beg.is_object());

    auto end = flatjson::end(&parser);
    for (auto it = next(beg); not_equal(it, end); it = next(it) ) {
        if ( it.is_object() ) {
            auto oit = begin(it);
            auto oend = flatjson::end(it);
            for ( oit = next(oit); not_equal(oit, oend); oit = next(oit) ) {
                std::cout << "  " << oit.key() << ":" << oit.type_name() << " -> " << oit.value() << std::endl;
            }
        } else {
            std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
        }
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_array_and_iteration_on_nested_array() {
    using namespace flatjson;

    static const char str[] = R"([[0,1,2,3], [4,5,6,7], [8,9,10,11]])";

    token tokens[20];
    auto parser = make_parser(
         args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    auto toknum = parse(&parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(&parser));
    assert(toknum == 20);

    auto beg = begin(&parser);
    assert(beg.is_array());

    auto end = flatjson::end(&parser);
    for ( auto it = next(beg); not_equal(it, end); it = next(it) ) {
        std::cout << it.type_name() << std::endl;

        auto ait = begin(it);
        auto aend = flatjson::end(it);
        for ( ait = next(ait); not_equal(ait, aend); ait = next(ait) ) {
            std::cout << "  " << ait.type_name() << " -> " << ait.value() << std::endl;
        }

        std::cout << ait.type_name() << std::endl;
    }
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    // An initialized parser without tokens grows its own array; feed() takes
    // the whole document as a single chunk.
    parser parser;
    init_parser(args.parser = &parser);
    auto cur = feed(&parser, str, sizeof(str) - 1);
    assert(cur.code == FJ_OK);

    auto toknum = static_cast<std::size_t>(parser.toks_cur - parser.toks_beg);
    assert(is_valid(&parser));
    assert(toknum == 12);

    free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    // An initialized parser without tokens grows its own array; feed() takes
    // the whole document as a single chunk.
    parser parser;
    init_parser(args.parser = &parser);
    auto cur = feed(&parser, str, sizeof(str) - 1);
    assert(cur.code == FJ_OK);

    auto toknum = static_cast<std::size_t>(parser.toks_cur - parser.toks_beg);
    assert(is_valid(&parser));
    assert(toknum == 12);

    auto beg = begin(&parser);
    assert(beg.is_object());

    auto end = flatjson::end(&parser);
    for (auto it = next(beg); not_equal(it, end); it = next(it) ) {
        std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
    }

    free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_array() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    // An initialized parser without tokens grows its own array; feed() takes
    // the whole document as a single chunk.
    parser parser;
    init_parser(args.parser = &parser);
    auto cur = feed(&parser, str, sizeof(str) - 1);
    assert(cur.code == FJ_OK);

    auto toknum = static_cast<std::size_t>(parser.toks_cur - parser.toks_beg);
    assert(is_valid(&parser));
    assert(toknum == 6);

    free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    // An initialized parser without tokens grows its own array; feed() takes
    // the whole document as a single chunk.
    parser parser;
    init_parser(args.parser = &parser);
    auto cur = feed(&parser, str, sizeof(str) - 1);
    assert(cur.code == FJ_OK);

    auto toknum = static_cast<std::size_t>(parser.toks_cur - parser.toks_beg);
    assert(is_valid(&parser));
    assert(toknum == 6);

    auto beg = begin(&parser);
    assert(beg.is_array());

    auto end = flatjson::end(&parser);
    for (auto it = next(beg); not_equal(it, end); it = next(it) ) {
        std::cout << it.type_name() << " -> " << it.value() << std::endl;
    }

    free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration_on_nested_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":{"f":false, "g":3, "h":"4"}, "e":"e"})";

    // An initialized parser without tokens grows its own array; feed() takes
    // the whole document as a single chunk.
    parser parser;
    init_parser(args.parser = &parser);
    auto cur = feed(&parser, str, sizeof(str) - 1);
    assert(cur.code == FJ_OK);

    auto toknum = static_cast<std::size_t>(parser.toks_cur - parser.toks_beg);
    assert(is_valid(&parser));
    assert(toknum == 19);

    auto beg = begin(&parser);
    assert(beg.is_object());

    auto end = flatjson::end(&parser);
    for (auto it = next(beg); not_equal(it, end); it = next(it) ) {
        if ( it.is_object() ) {
            auto oit = begin(it);
            auto oend = flatjson::end(it);
            for ( oit = next(oit); not_equal(oit, oend); oit = next(oit) ) {
                std::cout << "  " << oit.key() << ":" << oit.type_name() << " -> " << oit.value() << std::endl;
            }
        } else {
            std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
        }
    }

    free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration_on_nested_array() {
    using namespace flatjson;

    static const char str[] = R"([[0,1,2,3], [4,5,6,7], [8,9,10,11]])";

    // An initialized parser without tokens grows its own array; feed() takes
    // the whole document as a single chunk.
    parser parser;
    init_parser(args.parser = &parser);
    auto cur = feed(&parser, str, sizeof(str) - 1);
    assert(cur.code == FJ_OK);

    auto toknum = static_cast<std::size_t>(parser.toks_cur - parser.toks_beg);
    assert(is_valid(&parser));
    assert(toknum == 20);

    auto beg = begin(&parser);
    assert(beg.is_array());

    auto end = flatjson::end(&parser);
    for ( auto it = next(beg); not_equal(it, end); it = next(it) ) {
        std::cout << it.type_name() << std::endl;

        auto ait = begin(it);
        auto aend = flatjson::end(it);
        for ( ait = next(ait); not_equal(ait, aend); ait = next(ait) ) {
            std::cout << "  " << ait.type_name() << " -> " << ait.value() << std::endl;
        }

        std::cout << ait.type_name() << std::endl;
    }

    free_parser(&parser);
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    auto *parser = alloc_parser();

    auto toknum = parse(parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(parser));
    assert(toknum == 12);

    free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    auto *parser = alloc_parser();

    auto toknum = parse(parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(parser));
    assert(toknum == 12);

    auto beg = begin(parser);
    assert(beg.is_object());

    auto end = flatjson::end(parser);
    for (auto it = next(beg); not_equal(it, end); it = next(it) ) {
        std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
    }

    free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_array() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    auto *parser = alloc_parser();

    auto toknum = parse(parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(parser));
    assert(toknum == 6);

    free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    auto *parser = alloc_parser();

    auto toknum = parse(parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(parser));
    assert(toknum == 6);

    auto beg = begin(parser);
    assert(beg.is_array());

    auto end = flatjson::end(parser);
    for (auto it = next(beg); not_equal(it, end); it = next(it) ) {
        std::cout << it.type_name() << " -> " << it.value() << std::endl;
    }

    free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration_on_nested_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":{"f":false, "g":3, "h":"4"}, "e":"e"})";

    auto *parser = alloc_parser();

    auto toknum = parse(parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(parser));
    assert(toknum == 19);

    auto beg = begin(parser);
    assert(beg.is_object());

    auto end = flatjson::end(parser);
    for (auto it = next(beg); not_equal(it, end); it = next(it) ) {
        if ( it.is_object() ) {
            auto oit = begin(it);
            auto oend = flatjson::end(it);
            for ( oit = next(oit); not_equal(oit, oend); oit = next(oit) ) {
                std::cout << "  " << oit.key() << ":" << oit.type_name() << " -> " << oit.value() << std::endl;
            }
        } else {
            std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
        }
    }

    free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration_on_nested_array() {
    using namespace flatjson;

    static const char str[] = R"([[0,1,2,3], [4,5,6,7], [8,9,10,11]])";

    auto *parser = alloc_parser();

    auto toknum = parse(parser, args.json_beg = std::begin(str), args.json_end = std::end(str));
    assert(is_valid(parser));
    assert(toknum == 20);

    auto beg = begin(parser);
    assert(beg.is_array());

    auto end = flatjson::end(parser);
    for ( auto it = next(beg); not_equal(it, end); it = next(it) ) {
        std::cout << it.type_name() << std::endl;

        auto ait = begin(it);
        auto aend = flatjson::end(it);
        for ( ait = next(ait); not_equal(ait, aend); ait = next(ait) ) {
            std::cout << "  " << ait.type_name() << " -> " << ait.value() << std::endl;
        }

        std::cout << ait.type_name() << std::endl;
    }

    free_parser(parser);
}

/*************************************************************************************************/

int main() {
    FJ_RUN_TEST(stack_allocated_parser_and_tokens_for_object);
    FJ_RUN_TEST(stack_allocated_parser_and_tokens_for_object_and_iteration);
    FJ_RUN_TEST(stack_allocated_parser_and_tokens_for_array);
    FJ_RUN_TEST(stack_allocated_parser_and_tokens_for_array_and_iteration);
    FJ_RUN_TEST(stack_allocated_parser_and_tokens_for_object_and_iteration_on_nested_object);
    FJ_RUN_TEST(stack_allocated_parser_and_tokens_for_array_and_iteration_on_nested_array);

    FJ_RUN_TEST(stack_allocated_parser_and_dyn_allocated_tokens_for_object);
    FJ_RUN_TEST(stack_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration);
    FJ_RUN_TEST(stack_allocated_parser_and_dyn_allocated_tokens_for_array);
    FJ_RUN_TEST(stack_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration);
    FJ_RUN_TEST(stack_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration_on_nested_object);
    FJ_RUN_TEST(stack_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration_on_nested_array);

    FJ_RUN_TEST(dyn_allocated_parser_and_dyn_allocated_tokens_for_object);
    FJ_RUN_TEST(dyn_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration);
    FJ_RUN_TEST(dyn_allocated_parser_and_dyn_allocated_tokens_for_array);
    FJ_RUN_TEST(dyn_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration);
    FJ_RUN_TEST(dyn_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration_on_nested_object);
    FJ_RUN_TEST(dyn_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration_on_nested_array);

    return EXIT_SUCCESS;
}

/*************************************************************************************************/
