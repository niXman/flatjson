#include <flatjson/flatjson.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
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

static bool is_unsigned_integer_literal(const char *ptr, std::size_t len) {
    if ( len == 0 ) {
        return false;
    }
    for ( std::size_t i = 0; i < len; ++i ) {
        const auto c = static_cast<std::uint8_t>(ptr[i]);
        if ( c < static_cast<std::uint8_t>('0') || c > static_cast<std::uint8_t>('9') ) {
            return false;
        }
    }
    return true;
}

// Based on niXman's gist implementation (shift variant):
// https://gist.github.com/niXman/5c0e53ad0dc98e66399658915747828e#file-strtoint-cpp-L90
static std::uint64_t nixmans_atou64_shift(const char *ptr, std::size_t len) noexcept {
#define __FJ__PER_CHAR_EXPR(n) ((res << 1) + (res << 3) + (str[len - n] - '0'))
    const auto *str = reinterpret_cast<const std::uint8_t *>(ptr);
    std::uint64_t res = 0;
    switch ( len ) {
        case 20: res = __FJ__PER_CHAR_EXPR(20); /* fallthrough */
        case 19: res = __FJ__PER_CHAR_EXPR(19); /* fallthrough */
        case 18: res = __FJ__PER_CHAR_EXPR(18); /* fallthrough */
        case 17: res = __FJ__PER_CHAR_EXPR(17); /* fallthrough */
        case 16: res = __FJ__PER_CHAR_EXPR(16); /* fallthrough */
        case 15: res = __FJ__PER_CHAR_EXPR(15); /* fallthrough */
        case 14: res = __FJ__PER_CHAR_EXPR(14); /* fallthrough */
        case 13: res = __FJ__PER_CHAR_EXPR(13); /* fallthrough */
        case 12: res = __FJ__PER_CHAR_EXPR(12); /* fallthrough */
        case 11: res = __FJ__PER_CHAR_EXPR(11); /* fallthrough */
        case 10: res = __FJ__PER_CHAR_EXPR(10); /* fallthrough */
        case 9 : res = __FJ__PER_CHAR_EXPR(9 ); /* fallthrough */
        case 8 : res = __FJ__PER_CHAR_EXPR(8 ); /* fallthrough */
        case 7 : res = __FJ__PER_CHAR_EXPR(7 ); /* fallthrough */
        case 6 : res = __FJ__PER_CHAR_EXPR(6 ); /* fallthrough */
        case 5 : res = __FJ__PER_CHAR_EXPR(5 ); /* fallthrough */
        case 4 : res = __FJ__PER_CHAR_EXPR(4 ); /* fallthrough */
        case 3 : res = __FJ__PER_CHAR_EXPR(3 ); /* fallthrough */
        case 2 : res = __FJ__PER_CHAR_EXPR(2 ); /* fallthrough */
        case 1 : res = __FJ__PER_CHAR_EXPR(1 ); break;
        default: assert(!"unsupported integer length");
    }
#undef __FJ__PER_CHAR_EXPR
    return res;
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
    std::vector<long long> samples_conv;
    std::vector<long long> samples_gist;
    samples_conv.reserve(static_cast<std::size_t>(runs));
    samples_gist.reserve(static_cast<std::size_t>(runs));

    std::size_t ints_count = 0;
    std::uint64_t checksum_conv = 0;
    std::uint64_t checksum_gist = 0;

    for ( int i = 0; i < runs; ++i ) {
        auto *parser = flatjson::alloc_parser(
             flatjson::args.init_size = (body.size() + 1) / 4 + FJ_TOKEN_INIT_CAP
        );
        if ( !parser ) {
            std::cout << "alloc parser failed" << std::endl;
            return EXIT_FAILURE;
        }

        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = body.c_str()
            ,flatjson::args.json_end = body.c_str() + body.size() + 1
        );
        if ( !flatjson::is_valid(parser) ) {
            std::cout
                << "parse error: " << parser->error.code
                << ", msg=" << flatjson::error_message(parser->error)
                << std::endl;
            flatjson::free_parser(parser);
            return EXIT_FAILURE;
        }

        const auto *json_beg = body.c_str();
        const auto *tbeg = parser->toks_beg;
        const auto *tend = parser->toks_cur;

        auto t1 = std::chrono::high_resolution_clock::now();
        std::size_t local_count = 0;
        std::uint64_t local_sum_conv = 0;
        for ( const auto *t = tbeg; t != tend; ++t ) {
            if ( t->type != flatjson::FJ_TYPE_NUMBER ) {
                continue;
            }
            const char *ptr = json_beg + t->vpos;
            const std::size_t len = t->vlen;
            if ( !is_unsigned_integer_literal(ptr, len) ) {
                continue;
            }
            local_sum_conv += flatjson::details::conv_to(ptr, len, std::uint64_t{});
            ++local_count;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        std::uint64_t local_sum_gist = 0;
        auto t3 = std::chrono::high_resolution_clock::now();
        for ( const auto *t = tbeg; t != tend; ++t ) {
            if ( t->type != flatjson::FJ_TYPE_NUMBER ) {
                continue;
            }
            const char *ptr = json_beg + t->vpos;
            const std::size_t len = t->vlen;
            if ( !is_unsigned_integer_literal(ptr, len) ) {
                continue;
            }
            local_sum_gist += nixmans_atou64_shift(ptr, len);
        }
        auto t4 = std::chrono::high_resolution_clock::now();

        if ( i == 0 ) {
            ints_count = local_count;
        }
        checksum_conv ^= local_sum_conv + static_cast<std::uint64_t>(toknum);
        checksum_gist ^= local_sum_gist + static_cast<std::uint64_t>(toknum);
        samples_conv.push_back(std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());
        samples_gist.push_back(std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count());
        flatjson::free_parser(parser);
    }

    std::sort(samples_conv.begin(), samples_conv.end());
    std::sort(samples_gist.begin(), samples_gist.end());
    const auto sum_conv = std::accumulate(samples_conv.begin(), samples_conv.end(), 0LL);
    const auto avg_conv = static_cast<double>(sum_conv) / static_cast<double>(samples_conv.size());
    const auto median_conv = samples_conv[samples_conv.size() / 2];
    const auto sum_gist = std::accumulate(samples_gist.begin(), samples_gist.end(), 0LL);
    const auto avg_gist = static_cast<double>(sum_gist) / static_cast<double>(samples_gist.size());
    const auto median_gist = samples_gist[samples_gist.size() / 2];

    std::cout
        << "ints=" << ints_count
        << ", conv_avg_us=" << static_cast<long long>(avg_conv)
        << ", conv_median_us=" << median_conv
        << ", gist_avg_us=" << static_cast<long long>(avg_gist)
        << ", gist_median_us=" << median_gist
        << ", checksum_conv=" << checksum_conv
        << ", checksum_gist=" << checksum_gist
        << std::endl;

    return EXIT_SUCCESS;
}
