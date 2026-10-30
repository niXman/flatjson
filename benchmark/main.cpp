
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2022 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------


// to compile: g++ -std=c++11 main.cpp -O2 -o benchmark

#include <flatjson/flatjson.hpp>

#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdlib>

#include <cassert>

std::string read_file(const char *fname) {
    std::ifstream file(fname);
    assert(file.good());

    std::string str(
        (std::istreambuf_iterator<char>(file))
        ,std::istreambuf_iterator<char>())
    ;
    return str;
}

int main(int argc, char **argv) {
    if ( argc < 2 || argc > 3 ) {
        std::cout << argv[0] << " <filename.json> [runs]" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "sizeof(token) = " << sizeof(flatjson::token) << std::endl;
    const char *fname = argv[1];
    const int runs = (argc == 3) ? std::atoi(argv[2]) : 7;
    if ( runs <= 0 ) {
        std::cout << "runs must be > 0" << std::endl;
        return EXIT_FAILURE;
    }

    std::string body = read_file(fname);

    std::vector<long long> samples;
    samples.reserve(static_cast<std::size_t>(runs));
    std::size_t expected_toknum = 0;
    bool has_toknum = false;

    for ( int i = 0; i < runs; ++i ) {
        auto *parser = flatjson::alloc_parser(
             flatjson::args.init_size = (body.size() + 1) / 4 + FJ_TOKEN_INIT_CAP
        );
        if ( !parser ) {
            std::cout << "alloc parser failed" << std::endl;
            return EXIT_FAILURE;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        const auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = body.c_str()
            ,flatjson::args.json_end = body.c_str() + body.size() + 1
        );
        auto t2 = std::chrono::high_resolution_clock::now();

        if ( !flatjson::is_valid(parser) ) {
            std::cout
                << "parse error: " << parser->error.code
                << ", msg=" << flatjson::error_message(parser->error)
                << std::endl;
            flatjson::free_parser(parser);
            return EXIT_FAILURE;
        }

        if ( !has_toknum ) {
            expected_toknum = toknum;
            has_toknum = true;
        } else if ( expected_toknum != toknum ) {
            std::cout
                << "token count mismatch: expected " << expected_toknum
                << ", got " << toknum << std::endl;
            flatjson::free_parser(parser);
            return EXIT_FAILURE;
        }

        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        samples.push_back(duration);
        flatjson::free_parser(parser);
    }

    std::sort(samples.begin(), samples.end());
    const auto sum = std::accumulate(samples.begin(), samples.end(), 0LL);
    const auto avg = static_cast<double>(sum) / static_cast<double>(samples.size());
    const auto median = samples[samples.size() / 2];

    std::cout
        << "tokens=" << expected_toknum
        << ", avg=" << static_cast<long long>(avg) << " us"
        << ", median=" << median << " us"
        << std::endl;

    return EXIT_SUCCESS;
}
