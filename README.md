[![Build Status](https://travis-ci.org/niXman/flatjson.svg?branch=master)](https://travis-ci.org/niXman/flatjson)

# flatjson

The [header-only implementation](https://github.com/niXman/flatjson/blob/master/flatjson.hpp) of extremely fast zero allocation and zero copy JSON parser


# Intro
There are two classes:
1) `fjson` - is template which uses the stack to store tokens
2) `fdyjson` - is not template and which uses the heap(just one allocation!)

# Examples
```cpp
const char *jsstr = R"({"a":true, "b":null})";
flatjson::fjson<> json{jsstr};
assert(json.is_valid());
assert(json.is_object());
assert(json.size() == 2);

auto a = json.at("a");
assert(a.is_bool());
assert(a.to_bool() == true);

auto b = json.at("b");
assert(b.is_null());
```
