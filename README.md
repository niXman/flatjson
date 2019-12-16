# flatjson
The [header-only implementation](https://github.com/niXman/flatjson/blob/master/flatjson.h) of very fast zero allocation and zero copy JSON parser

# Intro
There are two types:
1) fjson - which is the template and which uses the stack
2) fdyjson - which is not the template and which uses the heap(just one allocation!)

#Examples

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
