
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2020 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#include "flatjson.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <cassert>

/*************************************************************************************************/

std::string read_file(const char *fname) {
    std::ifstream file(fname);
    assert(file.good());

    std::string str(
        (std::istreambuf_iterator<char>(file))
        ,std::istreambuf_iterator<char>())
    ;
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

    std::string buf = read_file(p.c_str());
    auto res = flatjson::details::fj_num_tokens(buf.c_str(), buf.size());
    return res.ec;
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

void unit_00() {
    flatjson::fjson json;

    assert(json.valid() == false);
}

void unit_01() {
    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
    flatjson::fjson json;
    json.load(str);

    assert(json.valid());
    assert(json.size() == 5);
    assert(json.is_object());
    assert(!json.is_array());
    assert(!json.is_number());
    assert(!json.is_string());
    assert(!json.is_null());
    assert(!json.is_bool());
}

void unit_02() {
    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
    flatjson::fjson json(str, sizeof(str)-1, 0);
    json.load(str);

    assert(json.valid());
    assert(json.size() == 5);
    assert(json.is_object());
    assert(!json.is_array());
    assert(!json.is_number());
    assert(!json.is_string());
    assert(!json.is_null());
    assert(!json.is_bool());
}

void unit_0() {
    static const char str[] = R"({})";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 0);
    assert(json.empty());
    assert(json.is_object());
    assert(!json.is_array());
    assert(!json.is_null());
    assert(!json.is_string());
    assert(!json.is_bool());
    assert(!json.is_number());
}

void unit_1() {
    static const char str[] = R"([])";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 0);
    assert(json.empty());
    assert(json.is_array());
    assert(!json.is_object());
    assert(!json.is_null());
    assert(!json.is_string());
    assert(!json.is_bool());
    assert(!json.is_number());
}

void unit_2() {
    static const char str[] = R"(true)";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 1);
    assert(!json.empty());
    assert(json.is_bool());
    assert(json.to_bool() == true);
    assert(!json.is_array());
    assert(!json.is_object());
    assert(!json.is_null());
    assert(!json.is_string());
    assert(!json.is_number());
}

void unit_3() {
    static const char str[] = R"(false)";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 1);
    assert(!json.empty());
    assert(json.is_bool());
    assert(json.to_bool() == false);
    assert(!json.is_array());
    assert(!json.is_object());
    assert(!json.is_null());
    assert(!json.is_string());
    assert(!json.is_number());
}

void unit_4() {
    static const char str[] = R"(null)";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 1);
    assert(!json.empty());
    assert(json.is_null());
    assert(!json.is_bool());
    assert(!json.is_array());
    assert(!json.is_object());
    assert(!json.is_string());
    assert(!json.is_number());
}

void unit_5() {
    static const char str[] = R"("")";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 1);
    assert(!json.empty());
    assert(json.is_string());
    assert(json.to_sstring() == "");
    assert(json.to_string() == "");
    assert(!json.is_null());
    assert(!json.is_bool());
    assert(!json.is_array());
    assert(!json.is_object());
    assert(!json.is_number());
}

void unit_6() {
    static const char str[] = R"("string")";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 1);
    assert(!json.empty());
    assert(json.is_string());
    assert(json.to_sstring() == "string");
    assert(json.to_string() == "string");
    assert(!json.is_null());
    assert(!json.is_bool());
    assert(!json.is_array());
    assert(!json.is_object());
    assert(!json.is_number());
}

void unit_7() {
    static const char str[] = R"(1234)";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 1);
    assert(!json.empty());
    assert(json.is_number());
    assert(json.to_uint() == 1234);
    assert(!json.is_string());
    assert(!json.is_null());
    assert(!json.is_bool());
    assert(!json.is_array());
    assert(!json.is_object());
}

void unit_8() {
    static const char str[] = R"(3.14)";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 1);
    assert(!json.empty());
    assert(json.is_number());
    assert(json.to_double() == 3.14);
    assert(!json.is_string());
    assert(!json.is_null());
    assert(!json.is_bool());
    assert(!json.is_array());
    assert(!json.is_object());
}

void unit_9() {
    static const char str[] = R"([0, "1", 3.14])";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 3);
    assert(!json.empty());
    assert(json.is_array());
    assert(!json.is_number());
    assert(!json.is_string());
    assert(!json.is_null());
    assert(!json.is_bool());
    assert(!json.is_object());

    assert(json.at(0).is_number());
    assert(json.at(0).to_uint() == 0);

    assert(json.at(1).is_string());
    assert(json.at(1).to_sstring() == "1");

    assert(json.at(2).is_number());
    assert(json.at(2).to_double() == 3.14);
}

void unit_10() {
    static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 5);
    assert(json.is_object());
    assert(!json.is_array());
    assert(!json.is_number());
    assert(!json.is_string());
    assert(!json.is_null());
    assert(!json.is_bool());

    auto j0 = json.at("a");
    assert(j0.valid());
    assert(j0.size() == 1);
    assert(!j0.empty());
    assert(j0.is_bool());
    assert(j0.to_bool() == true);
    assert(!j0.is_array());
    assert(!j0.is_object());
    assert(!j0.is_null());
    assert(!j0.is_string());
    assert(!j0.is_number());

    auto j1 = json.at("b");
    assert(j1.valid());
    assert(j1.size() == 1);
    assert(!j1.empty());
    assert(j1.is_bool());
    assert(j1.to_bool() == false);
    assert(!j1.is_array());
    assert(!j1.is_object());
    assert(!j1.is_null());
    assert(!j1.is_string());
    assert(!j1.is_number());

    auto j2 = json.at("c");
    assert(j2.valid());
    assert(j2.size() == 1);
    assert(!j2.empty());
    assert(j2.is_null());
    assert(j2.to_sstring() == "");
    assert(!j2.is_array());
    assert(!j2.is_object());
    assert(!j2.is_bool());
    assert(!j2.is_string());
    assert(!j2.is_number());

    auto j3 = json.at("d");
    assert(j3.valid());
    assert(j3.size() == 1);
    assert(!j3.empty());
    assert(j3.is_number());
    assert(j3.to_uint() == 0);
    assert(j3.to_sstring() == "0");
    assert(!j3.is_array());
    assert(!j3.is_object());
    assert(!j3.is_bool());
    assert(!j3.is_string());
    assert(!j3.is_null());

    auto j4 = json.at("e");
    assert(j4.valid());
    assert(j4.size() == 1);
    assert(!j4.empty());
    assert(j4.is_string());
    assert(j4.to_sstring() == "e");
    assert(!j4.is_array());
    assert(!j4.is_object());
    assert(!j4.is_bool());
    assert(!j4.is_number());
    assert(!j4.is_null());
}

void unit_11() {
    static const char str[] = R"({"a":{"b":true, "c":1234}})";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 1);
    assert(json.is_object());
    assert(!json.is_array());
    assert(!json.is_number());
    assert(!json.is_string());
    assert(!json.is_null());
    assert(!json.is_bool());

    auto j0 = json.at("a");
    assert(j0.valid());
    assert(j0.size() == 2);
    assert(j0.is_object());
    assert(!j0.is_array());
    assert(!j0.is_number());
    assert(!j0.is_string());
    assert(!j0.is_null());
    assert(!j0.is_bool());

    auto j1 = j0.at("b");
    assert(j1.valid());
    assert(j1.size() == 1);
    assert(j1.is_bool());
    assert(j1.to_bool() == true);
    assert(!j1.is_array());
    assert(!j1.is_number());
    assert(!j1.is_string());
    assert(!j1.is_null());
    assert(!j1.is_object());

    auto j2 = j0.at("c");
    assert(j2.valid());
    assert(j2.size() == 1);
    assert(j2.is_number());
    assert(j2.to_int() == 1234);
    assert(!j2.is_array());
    assert(!j2.is_string());
    assert(!j2.is_null());
    assert(!j2.is_object());
    assert(!j2.is_bool());
}

void unit_12() {
    static const char str[] = R"({"a":[0,1,2]})";
    flatjson::fjson json(str);

    assert(json.valid());
    assert(json.size() == 1);
    assert(json.is_object());
    assert(!json.is_array());
    assert(!json.is_number());
    assert(!json.is_string());
    assert(!json.is_null());
    assert(!json.is_bool());

    auto j0 = json.at("a");
    assert(j0.valid());
    assert(j0.size() == 3);
    assert(j0.is_array());
    assert(!j0.is_object());
    assert(!j0.is_number());
    assert(!j0.is_string());
    assert(!j0.is_null());
    assert(!j0.is_bool());

    assert(j0[0].to_int() == 0);
    assert(j0[1].to_int() == 1);
    assert(j0[2].to_int() == 2);
}

void unit_13() {
    static const char jsstr[] = R"({"a":true, "b":{"c":{"d":1, "e":2}}, "c":[0,1,2,3]})";
    flatjson::fjson json{jsstr};
    auto str = json.dump(4);

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
    assert(str == expected);

    std::ostringstream ss;
    json.dump(ss, 4);
    assert(ss.str() == expected);
}

void unit_14() {
    static const char jsstr[] = R"({"a":[4,3,2,1], "b":[{"a":0,"b":0,"c":1},{"b":1,"a":0,"c":0},{"c":2,"b":0,"a":0}], "c":[0,1,2,3]})";
    flatjson::fjson json{jsstr};
    assert(json.valid());
    assert(json.is_object());

    const auto a = json.at("b");
    assert(a.is_array());
    for ( auto idx = 0u; idx < a.size(); ++idx ) {
        const auto o = a.at(idx);
        assert(o.valid());
        assert(o.is_object());
        char key[] = {static_cast<char>('a'+idx), '\0'};
        const auto v = o.at(key);
        assert(v.valid());
        assert(v.is_number());
        assert(v.to_uint() == idx);
    }

    const auto b = json.at("b");
    assert(b.is_array());

    const auto c = json.at("c");
    assert(c.is_array());
}

/*************************************************************************************************/

void unit_15() {
    {
        static const char jsstr[] = R"([0])";

        flatjson::fjson json{jsstr};
        assert(json.valid());
        assert(json.is_array());
        assert(json.size() == 1);
        auto tok = json.tokens();
        assert(tok == 3);

        for ( auto idx = 0u; idx < json.size(); ++idx ) {
            auto item = json.at(idx);
            assert(item.is_number());
            assert(item.size() == 1);
            assert(item.to_uint() == idx);
        }
    }

    {
        static const char jsstr[] = R"([0, 1])";

        flatjson::fjson json{jsstr};
        assert(json.valid());
        assert(json.is_array());
        assert(json.size() == 2);
        assert(json.tokens() == 4);

        for ( auto idx = 0u; idx < json.size(); ++idx ) {
            auto item = json.at(idx);
            assert(item.is_number());
            assert(item.size() == 1);
            assert(item.to_uint() == idx);
        }
    }

    {
        static const char jsstr[] = R"([[0, 1]])";

        flatjson::fjson json{jsstr};
        assert(json.valid());
        assert(json.is_array());
        assert(json.size() == 1);
        assert(json.tokens() == 6);

        auto subarr = json.at(0);
        assert(subarr.valid());
        assert(subarr.is_array());
        assert(subarr.size() == 2);
        assert(subarr.tokens() == 4);

        for ( auto idx = 0u; idx < subarr.size(); ++idx ) {
            auto item = subarr.at(idx);
            assert(item.is_number());
            assert(item.size() == 1);
            assert(item.to_uint() == idx);
        }
    }
}

/*************************************************************************************************/

int main() {
    auto res = test_conformance();
    if ( res.ec ) {
        std::cout << "test \"" << res.name << "\" FAILED!" << std::endl;
    }

    unit_00();
    unit_01();
    unit_02();
    unit_0();
    unit_1();
    unit_2();
    unit_3();
    unit_4();
    unit_5();
    unit_6();
    unit_7();
    unit_8();
    unit_9();
    unit_10();
    unit_11();
    unit_12();
    unit_13();
    unit_14();
    unit_15();

    return EXIT_SUCCESS;
}
