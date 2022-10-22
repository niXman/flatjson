
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2022 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------


// to compile: g++ -std=c++11 parsetest.cpp -O2 -o parsetest

#include <flatjson/flatjson.hpp>

#include <iostream>
#include <fstream>
#include <chrono>

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
	if ( argc != 2 ) {
		std::cout << "parsetest <filename.json>" << std::endl;

		return EXIT_FAILURE;
	}

	std::cout << "sizeof(token) = " << sizeof(flatjson::token) << std::endl;
	const char *fname = argv[1];
	std::string body = read_file(fname);

	auto t1 = std::chrono::high_resolution_clock::now();
	
	flatjson::fjson json(body.c_str(), body.c_str() + body.size());
	if ( !json.is_valid() ) {
	    std::cout << "parse error: " << json.error() << ", msg=" << json.error_string() << std::endl;

	    return EXIT_FAILURE;
	}

	auto t2 = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

	std::cout << "tokens: " << json.tokens() << ", parse time: " << duration << " ms" << std::endl;

	return EXIT_SUCCESS;
}
