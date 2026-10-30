#include <flatjson/flatjson.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

static std::string read_file(const char *fname) {
    std::ifstream file(fname);
    assert(file.good());
    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
}

int main(int argc, char **argv) {
    if ( argc < 2 || argc > 3 ) {
        std::cout << argv[0] << " <filename.json> [runs]" << std::endl;
        return EXIT_FAILURE;
    }

    const char *fname = argv[1];
    const int runs = (argc == 3) ? std::atoi(argv[2]) : 11;
    if ( runs <= 0 ) {
        std::cout << "runs must be > 0" << std::endl;
        return EXIT_FAILURE;
    }

    std::string body = read_file(fname);
    std::vector<long long> parse_us_samples;
    parse_us_samples.reserve(static_cast<std::size_t>(runs));

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

        const auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        parse_us_samples.push_back(duration);

        flatjson::free_parser(parser);
    }

    std::sort(parse_us_samples.begin(), parse_us_samples.end());
    const auto parse_us_sum = std::accumulate(parse_us_samples.begin(), parse_us_samples.end(), 0LL);
    const auto parse_us_avg = static_cast<double>(parse_us_sum) / static_cast<double>(parse_us_samples.size());
    const auto parse_us_median = parse_us_samples[parse_us_samples.size() / 2];

    std::cout
        << "tokens=" << expected_toknum
        << ", parse_avg_us=" << static_cast<long long>(parse_us_avg)
        << ", parse_median_us=" << parse_us_median
        << std::endl;

    return EXIT_SUCCESS;
}
