
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

#include <cstring>
#include <iterator>

#define FJ_RUN_TEST(test) \
    test(); std::cout << "test \"" << #test "\" passed!" << std::endl;

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
    token tokens[12];
    parser parser{};

    auto json = pparse(
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
        ,args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    assert(json.is_valid());
    assert(json.is_object());
    assert(json.tokens() == 12);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_object_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    token tokens[12];
    parser parser{};

    auto json = pparse(
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
        ,args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    assert(json.is_valid());
    assert(json.is_object());
    assert(json.tokens() == 12);

    auto beg = json.begin();
    assert(beg->is_object());

    auto end = json.end();
    for (auto it = ++beg; it != end; ++it ) {
        std::cout << it->key() << ":" << it->type_name() << " -> " << it->value() << std::endl;
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_array() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    token tokens[7];
    parser parser{};

    auto json = pparse(
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
        ,args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    assert(json.is_valid());
    assert(json.is_array());
    assert(json.tokens() == 6);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_array_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    token tokens[7];
    parser parser{};

    auto json = pparse(
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
        ,args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    assert(json.is_valid());
    assert(json.is_array());
    assert(json.tokens() == 6);

    auto beg = json.begin();
    assert(beg->is_array());

    auto end = json.end();
    for (auto it = ++beg; it != end; ++it ) {
        std::cout << it->type_name() << " -> " << it->value() << std::endl;
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_object_and_iteration_on_nested_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":{"f":false, "g":3, "h":"4"}, "e":"e"})";

    token tokens[19];
    parser parser{};

    auto json = pparse(
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
        ,args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    assert(json.is_valid());
    assert(json.is_object());
    assert(json.tokens() == 19);

    auto beg = json.begin();
    assert(beg->is_object());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        if ( it->is_object() ) {
            fjson subjson = json.at(it);
            auto oit  = subjson.begin();
            auto oend = subjson.end();
            for ( ++oit; oit != oend; ++oit ) {
                std::cout << "  " << oit->key() << ":" << oit->type_name() << " -> " << oit->value() << std::endl;
            }
        } else {
            std::cout << it->key() << ":" << it->type_name() << " -> " << it->value() << std::endl;
        }
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_tokens_for_array_and_iteration_on_nested_array() {
    using namespace flatjson;

    static const char str[] = R"([[0,1,2,3], [4,5,6,7], [8,9,10,11]])";

    token tokens[20];
    parser parser{};

    auto json = pparse(
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
        ,args.toks_beg = std::begin(tokens)
        ,args.toks_end = std::end(tokens)
    );

    assert(json.is_valid());
    assert(json.is_array());
    assert(json.tokens() == 20);

    auto beg = json.begin();
    assert(beg->is_array());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        std::cout << it->type_name() << std::endl;

        fjson subjson = json.at(it);
        auto ait  = subjson.begin();
        auto aend = subjson.end();
        for ( ++ait; ait != aend; ++ait ) {
            std::cout << "  " << ait->type_name() << " -> " << ait->value() << std::endl;
        }

        std::cout << ait->type_name() << std::endl;
    }
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    parser parser{};

    fjson json{
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
    };

    assert(json.is_valid());
    assert(json.is_object());
    assert(json.tokens() == 12);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    parser parser{};

    fjson json{
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
    };

    assert(json.is_valid());
    assert(json.is_object());
    assert(json.tokens() == 12);

    auto beg = json.begin();
    assert(beg->is_object());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        std::cout << it->key() << ":" << it->type_name() << " -> " << it->value() << std::endl;
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_array() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    parser parser{};

    fjson json{
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
    };

    assert(json.is_valid());
    assert(json.is_array());
    assert(json.tokens() == 6);
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    parser parser{};

    fjson json{
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
    };

    assert(json.is_valid());
    assert(json.is_array());
    assert(json.tokens() == 6);

    auto beg = json.begin();
    assert(beg->is_array());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        std::cout << it->type_name() << " -> " << it->value() << std::endl;
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration_on_nested_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":{"f":false, "g":3, "h":"4"}, "e":"e"})";

    parser parser{};

    fjson json{
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
    };

    assert(json.is_valid());
    assert(json.is_object());
    assert(json.tokens() == 19);

    auto beg = json.begin();
    assert(beg->is_object());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        if ( it->is_object() ) {
            auto subjson = json.at(it);
            auto oit  = subjson.begin();
            auto oend = subjson.end();
            for ( ++oit; oit != oend; ++oit ) {
                std::cout << "  " << oit->key() << ":" << oit->type_name() << " -> " << oit->value() << std::endl;
            }
        } else {
            std::cout << it->key() << ":" << it->type_name() << " -> " << it->value() << std::endl;
        }
    }
}

/*************************************************************************************************/

static void stack_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration_on_nested_array() {
    using namespace flatjson;

    static const char str[] = R"([[0,1,2,3], [4,5,6,7], [8,9,10,11]])";

    parser parser{};

    fjson json{
         args.parser = &parser
        ,args.json_beg = std::begin(str)
        ,args.json_end = std::end(str)
    };

    assert(json.is_valid());
    assert(json.is_array());
    assert(json.tokens() == 20);

    auto beg = json.begin();
    assert(beg->is_array());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        std::cout << it->type_name() << std::endl;

        fjson subjson = json.at(it);
        auto ait  = subjson.begin();
        auto aend = subjson.end();
        for ( ++ait; ait != aend; ++ait ) {
            std::cout << "  " << ait->type_name() << " -> " << ait->value() << std::endl;
        }

        std::cout << ait->type_name() << std::endl;
    }
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    fjson json{args.json_beg = std::begin(str), args.json_end = std::end(str)};

    assert(json.is_valid());
    assert(json.is_object());
    assert(json.tokens() == 12);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";

    fjson json{args.json_beg = std::begin(str), args.json_end = std::end(str)};

    assert(json.is_valid());
    assert(json.is_object());
    assert(json.tokens() == 12);

    auto beg = json.begin();
    assert(beg->is_object());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        std::cout << it->key() << ":" << it->type_name() << " -> " << it->value() << std::endl;
    }
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_array() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    fjson json{args.json_beg = std::begin(str), args.json_end = std::end(str)};

    assert(json.is_valid());
    assert(json.is_array());
    assert(json.tokens() == 6);
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration() {
    using namespace flatjson;

    static const char str[] = R"([4,3,2,1])";

    fjson json{args.json_beg = std::begin(str), args.json_end = std::end(str)};

    assert(json.is_valid());
    assert(json.is_array());
    assert(json.tokens() == 6);

    auto beg = json.begin();
    assert(beg->is_array());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        std::cout << it->type_name() << " -> " << it->value() << std::endl;
    }
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_object_and_iteration_on_nested_object() {
    using namespace flatjson;

    static const char str[] = R"({"a":true, "b":false, "c":null, "d":{"f":false, "g":3, "h":"4"}, "e":"e"})";

    fjson json{args.json_beg = std::begin(str), args.json_end = std::end(str)};

    assert(json.is_valid());
    assert(json.is_object());
    assert(json.tokens() == 19);

    auto beg = json.begin();
    assert(beg->is_object());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        if ( it->is_object() ) {
            auto subjson = json.at(it);
            auto oit  = subjson.begin();
            auto oend = subjson.end();
            for ( ++oit; oit != oend; ++oit ) {
                std::cout << "  " << oit->key() << ":" << oit->type_name() << " -> " << oit->value() << std::endl;
            }
        } else {
            std::cout << it->key() << ":" << it->type_name() << " -> " << it->value() << std::endl;
        }
    }
}

/*************************************************************************************************/

static void dyn_allocated_parser_and_dyn_allocated_tokens_for_array_and_iteration_on_nested_array() {
    using namespace flatjson;

    static const char str[] = R"([[0,1,2,3], [4,5,6,7], [8,9,10,11]])";

    fjson json{args.json_beg = std::begin(str), args.json_end = std::end(str)};

    assert(json.is_valid());
    assert(json.is_array());
    assert(json.tokens() == 20);

    auto beg = json.begin();
    assert(beg->is_array());

    auto end = json.end();
    for ( auto it = ++beg; it != end; ++it ) {
        std::cout << it->type_name() << std::endl;

        fjson subjson = json.at(it);
        auto ait  = subjson.begin();
        auto aend = subjson.end();
        for ( ++ait; ait != aend; ++ait ) {
            std::cout << "  " << ait->type_name() << " -> " << ait->value() << std::endl;
        }

        std::cout << ait->type_name() << std::endl;
    }
}

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/

static void highlevel_api() {
    static const char str[] = R"([[0,1,2,3], [4,5,6,7], [8,9,10,11]])";
    // user-provided parser and tokens
    {
        flatjson::token tokens[20];
        flatjson::parser parser{};
        auto json = flatjson::pparse(
             flatjson::args.parser = &parser
            ,flatjson::args.json_beg = str
            ,flatjson::args.json_end = std::end(str)
            ,flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        assert(json.is_valid());
        assert(json.is_array());
        assert(json.tokens() == 20);
    }
    {
        const char *ptr = str;
        flatjson::token tokens[20];
        flatjson::parser parser{};
        auto json = flatjson::pparse(
             flatjson::args.parser = &parser
            ,flatjson::args.json_beg = ptr
            ,flatjson::args.json_end = ptr + std::strlen(ptr) + 1
            ,flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        assert(json.is_valid());
        assert(json.is_array());
        assert(json.tokens() == 20);
    }
    // user-provided parser and dyn tokens
    {
        flatjson::parser parser{};
        auto json = flatjson::pparse(
             flatjson::args.parser = &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );
        assert(json.is_valid());
        assert(json.is_array());
        assert(json.tokens() == 20);
    }
    {
        const char *ptr = str;
        flatjson::parser parser{};
        auto json = flatjson::pparse(
             flatjson::args.parser = &parser
            ,flatjson::args.json_beg = ptr
            ,flatjson::args.json_end = ptr + std::strlen(ptr) + 1
        );
        assert(json.is_valid());
        assert(json.is_array());
        assert(json.tokens() == 20);
    }
    // dyn parser and dyn tokens
    {
        auto json = flatjson::pparse(
             flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );
        assert(json.is_valid());
        assert(json.is_array());
        assert(json.tokens() == 20);
    }
    {
        const char *ptr = str;
        auto json = flatjson::pparse(
             flatjson::args.json_beg = ptr
            ,flatjson::args.json_end = ptr + std::strlen(ptr) + 1
        );
        assert(json.is_valid());
        assert(json.is_array());
        assert(json.tokens() == 20);
    }
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

    FJ_RUN_TEST(highlevel_api);

    return EXIT_SUCCESS;
}

/*************************************************************************************************/
