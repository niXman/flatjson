# FlatJson

The header-only implementation of an extremely fast zero allocation and zero copying JSON parser

![Forks](https://img.shields.io/github/forks/nixman/flatjson?style=social)
![Stars](https://img.shields.io/github/stars/nixman/flatjson?style=social)
![Release](https://img.shields.io/github/v/release/nixman/flatjson)
![Code size](https://img.shields.io/github/languages/code-size/nixman/flatjson)
![Repo size](https://img.shields.io/github/repo-size/nixman/flatjson)
![CI build status](https://github.com/nixman/flatjson/actions/workflows/c-cpp.yml/badge.svg)

## CI Job Matrix

| Target System | Toolchain          | Note   |
|---------------|--------------------|--------|
| Linux         | G++                | 32-bit |
| Linux         | G++                | 64-bit |
| OSX           | Apple Clang++      | 32-bit |
| OSX           | Apple Clang++      | 64-bit |
| Windows       | Visual Studio 2026 | 32-bit |
| Windows       | Visual Studio 2026 | 64-bit |
| Windows       | Visual Studio 2022 | 32-bit |
| Windows       | Visual Studio 2022 | 64-bit |
| Windows       | Visual Studio 2019 | 32-bit |
| Windows       | Visual Studio 2019 | 64-bit |

## Usage

One document, with the parser and the token array on the stack.

```cpp
#include <flatjson/flatjson.hpp>

const char text[] = "{\"id\":7,\"ok\":true}";
flatjson::token tokens[8]{};
auto parser = flatjson::make_parser(
     flatjson::args.toks_beg = tokens
    ,flatjson::args.toks_end = tokens + 8
);
flatjson::parse(
     &parser
    ,flatjson::args.json_beg = text
    ,flatjson::args.json_end = text + sizeof(text) - 1
);
if ( flatjson::is_valid(&parser) ) {
    const int id = flatjson::at(&parser, "id").to_int();
}
```

A document split across several buffers. `feed()` keeps the tokens and reads the next chunk as a continuation of the same value. The chunk table is on the stack, so this path does not allocate either. Each chunk must stay alive while the parser is used: a token points into the buffer it was parsed from. A number or a string must not be cut at the last byte of a chunk.

```cpp
#include <flatjson/flatjson.hpp>

#include <cstring>

flatjson::token tokens[8]{};
flatjson::chunk chunks[4]{};
auto parser = flatjson::make_parser(
     flatjson::args.toks_beg = tokens
    ,flatjson::args.toks_end = tokens + 8
    ,flatjson::args.chunks_beg = chunks
    ,flatjson::args.chunks_end = chunks + 4
);
const char *parts[] = {"[1,", "2,", " 3]"};
flatjson::cursor cur{};
for ( const char *part: parts ) {
    cur = flatjson::feed(&parser, part, std::strlen(part));
}
if ( cur.code == flatjson::FJ_OK ) {
    const int first = flatjson::at(&parser, 0u).to_int();
}
```

## LICENSE

License under the Apache-2.0 (http://www.apache.org/licenses/)
