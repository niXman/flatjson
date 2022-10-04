
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

#define FJ_RUN_TEST(test) \
    test(); std::cout << "test \"" << #test "\" passed!" << std::endl;

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
    fj_token tokens[10];
    auto parser = fj_make_parser(
         std::begin(tokens)
        ,std::end(tokens)
        ,std::begin(str)
        ,std::end(str)
    );

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 7);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_object_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    fj_token tokens[10];
    auto parser = fj_make_parser(
         std::begin(tokens)
        ,std::end(tokens)
        ,std::begin(str)
        ,std::end(str)
    );

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 7);

    auto beg = fj_iter_begin(&parser);
    assert(beg.is_object());

    auto end = fj_iter_end(&parser);
    for (auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_array() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";
    fj_token tokens[17];
    auto parser = fj_make_parser(
         std::begin(tokens)
        ,std::end(tokens)
        ,std::begin(str)
        ,std::end(str)
    );

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 6);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_array_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    fj_token tokens[10];
    auto parser = fj_make_parser(
         std::begin(tokens)
        ,std::end(tokens)
        ,std::begin(str)
        ,std::end(str)
    );

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 6);

    auto beg = fj_iter_begin(&parser);
    assert(beg.is_array());

    auto end = fj_iter_end(&parser);
    for (auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_object_and_iteration_on_nested_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":{"f":false, "g":3, "h":"4"}, "e":"e"})";

    fj_token tokens[11];
    auto parser = fj_make_parser(
         std::begin(tokens)
        ,std::end(tokens)
        ,std::begin(str)
        ,std::end(str)
    );

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 11);

    auto beg = fj_iter_begin(&parser);
    assert(beg.is_object());

    auto end = fj_iter_end(&parser);
    for (auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        if ( it.is_object() ) {
            auto oit = fj_iter_begin(it);
            auto oend = fj_iter_end(it);
            for ( oit = fj_iter_next(oit); fj_iter_not_equal(oit, oend); oit = fj_iter_next(oit) ) {
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

    fj_token tokens[20];
    auto parser = fj_make_parser(
         std::begin(tokens)
        ,std::end(tokens)
        ,std::begin(str)
        ,std::end(str)
    );

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 20);

    auto beg = fj_iter_begin(&parser);
    assert(beg.is_array());

    auto end = fj_iter_end(&parser);
    for ( auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        std::cout << it.type_name() << std::endl;

        auto ait = fj_iter_begin(it);
        auto aend = fj_iter_end(it);
        for ( ait = fj_iter_next(ait); fj_iter_not_equal(ait, aend); ait = fj_iter_next(ait) ) {
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

    auto parser = fj_make_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 7);

    fj_free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    auto parser = fj_make_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 7);

    auto beg = fj_iter_begin(&parser);
    assert(beg.is_object());

    auto end = fj_iter_end(&parser);
    for (auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
    }

    fj_free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_array() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    auto parser = fj_make_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 6);

    fj_free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    auto parser = fj_make_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 6);

    auto beg = fj_iter_begin(&parser);
    assert(beg.is_array());

    auto end = fj_iter_end(&parser);
    for (auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        std::cout << it.type_name() << " -> " << it.value() << std::endl;
    }

    fj_free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration_on_nested_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":{"f":false, "g":3, "h":"4"}, "e":"e"})";

    auto parser = fj_make_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 11);

    auto beg = fj_iter_begin(&parser);
    assert(beg.is_object());

    auto end = fj_iter_end(&parser);
    for (auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        if ( it.is_object() ) {
            auto oit = fj_iter_begin(it);
            auto oend = fj_iter_end(it);
            for ( oit = fj_iter_next(oit); fj_iter_not_equal(oit, oend); oit = fj_iter_next(oit) ) {
                std::cout << "  " << oit.key() << ":" << oit.type_name() << " -> " << oit.value() << std::endl;
            }
        } else {
            std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
        }
    }

    fj_free_parser(&parser);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration_on_nested_array() {
    using namespace flatjson;

    static const char str[] = R"([[0,1,2,3], [4,5,6,7], [8,9,10,11]])";

    auto parser = fj_make_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(&parser);
    assert(fj_is_valid(&parser));
    assert(toknum == 20);

    auto beg = fj_iter_begin(&parser);
    assert(beg.is_array());

    auto end = fj_iter_end(&parser);
    for ( auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        std::cout << it.type_name() << std::endl;

        auto ait = fj_iter_begin(it);
        auto aend = fj_iter_end(it);
        for ( ait = fj_iter_next(ait); fj_iter_not_equal(ait, aend); ait = fj_iter_next(ait) ) {
            std::cout << "  " << ait.type_name() << " -> " << ait.value() << std::endl;
        }

        std::cout << ait.type_name() << std::endl;
    }

    fj_free_parser(&parser);
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    auto *parser = fj_alloc_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(parser);
    assert(fj_is_valid(parser));
    assert(toknum == 7);

    fj_free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    auto *parser = fj_alloc_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(parser);
    assert(fj_is_valid(parser));
    assert(toknum == 7);

    auto beg = fj_iter_begin(parser);
    assert(beg.is_object());

    auto end = fj_iter_end(parser);
    for (auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
    }

    fj_free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_array() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    auto *parser = fj_alloc_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(parser);
    assert(fj_is_valid(parser));
    assert(toknum == 6);

    fj_free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    auto *parser = fj_alloc_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(parser);
    assert(fj_is_valid(parser));
    assert(toknum == 6);

    auto beg = fj_iter_begin(parser);
    assert(beg.is_array());

    auto end = fj_iter_end(parser);
    for (auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        std::cout << it.type_name() << " -> " << it.value() << std::endl;
    }

    fj_free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration_on_nested_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":{"f":false, "g":3, "h":"4"}, "e":"e"})";

    auto *parser = fj_alloc_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(parser);
    assert(fj_is_valid(parser));
    assert(toknum == 11);

    auto beg = fj_iter_begin(parser);
    assert(beg.is_object());

    auto end = fj_iter_end(parser);
    for (auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        if ( it.is_object() ) {
            auto oit = fj_iter_begin(it);
            auto oend = fj_iter_end(it);
            for ( oit = fj_iter_next(oit); fj_iter_not_equal(oit, oend); oit = fj_iter_next(oit) ) {
                std::cout << "  " << oit.key() << ":" << oit.type_name() << " -> " << oit.value() << std::endl;
            }
        } else {
            std::cout << it.key() << ":" << it.type_name() << " -> " << it.value() << std::endl;
        }
    }

    fj_free_parser(parser);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration_on_nested_array() {
    using namespace flatjson;

    static const char str[] = R"([[0,1,2,3], [4,5,6,7], [8,9,10,11]])";

    auto *parser = fj_alloc_parser(std::begin(str), std::end(str));

    auto toknum = fj_parse(parser);
    assert(fj_is_valid(parser));
    assert(toknum == 20);

    auto beg = fj_iter_begin(parser);
    assert(beg.is_array());

    auto end = fj_iter_end(parser);
    for ( auto it = fj_iter_next(beg); fj_iter_not_equal(it, end); it = fj_iter_next(it) ) {
        std::cout << it.type_name() << std::endl;

        auto ait = fj_iter_begin(it);
        auto aend = fj_iter_end(it);
        for ( ait = fj_iter_next(ait); fj_iter_not_equal(ait, aend); ait = fj_iter_next(ait) ) {
            std::cout << "  " << ait.type_name() << " -> " << ait.value() << std::endl;
        }

        std::cout << ait.type_name() << std::endl;
    }

    fj_free_parser(parser);
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
