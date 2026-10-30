#include <flatjson/flatjson.hpp>
#include <flatjson/state.hpp>

#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::string read_file(const std::string &fname) {
    std::ifstream file(fname, std::ios::binary);
    if ( !file.good() ) {
        return {};
    }
    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
}

std::size_t raw_state_size(const flatjson::parser *parser, std::size_t json_size) {
    return sizeof(std::uint32_t)
        + json_size
        + sizeof(std::uint32_t)
        + static_cast<std::size_t>(parser->toks_cur - parser->toks_beg)
            * sizeof(flatjson::token);
}

} // namespace

int main() {
    const std::vector<std::string> files = {
        "../assets/canada.json",
        "../assets/citm_catalog.json",
        "../assets/jeopardy.json",
        "../assets/twitter.json"
    };

    std::cout
        << std::left
        << std::setw(24) << "file"
        << std::setw(12) << "tokens"
        << std::setw(14) << "raw_bytes"
        << std::setw(16) << "packed_bytes"
        << std::setw(10) << "ratio"
        << std::setw(14) << "pack_us"
        << std::setw(14) << "unpack_us"
        << '\n';

    for ( const auto &path: files ) {
        const std::string body = read_file(path);
        if ( body.empty() ) {
            std::cout << path << " : failed to read" << std::endl;
            continue;
        }

        auto *parser = flatjson::alloc_parser();
        if ( !parser ) {
            std::cout << path << " : alloc_parser failed" << std::endl;
            continue;
        }

        const auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = body.c_str()
            ,flatjson::args.json_end = body.c_str() + body.size() + 1
        );
        if ( !flatjson::is_valid(parser) ) {
            std::cout << path << " : parse failed" << std::endl;
            flatjson::free_parser(parser);
            continue;
        }

        const std::size_t raw_sz = raw_state_size(parser, body.size() + 1);
        const std::size_t packed_sz = flatjson::packed_state_size(parser);
        std::vector<char> packed(packed_sz);

        constexpr int runs = 10;
        long long pack_sum_us = 0;
        long long unpack_sum_us = 0;

        for ( int i = 0; i < runs; ++i ) {
            auto t1 = std::chrono::high_resolution_clock::now();
            const auto wr = flatjson::pack_state(packed.data(), packed.size(), parser);
            auto t2 = std::chrono::high_resolution_clock::now();
            assert(wr == packed_sz);

            flatjson::parser p2{};
            std::vector<flatjson::token> toks2(static_cast<std::size_t>(toknum));
            p2.toks_beg = toks2.data();
            p2.toks_cur = toks2.data();
            p2.toks_end = toks2.data() + toks2.size();
            p2.dyn_alloc = false;

            auto t3 = std::chrono::high_resolution_clock::now();
            const bool ok = flatjson::unpack_state(&p2, packed.data(), packed.size());
            auto t4 = std::chrono::high_resolution_clock::now();
            assert(ok);
            assert(flatjson::is_valid(&p2));
            assert(static_cast<std::size_t>(p2.toks_cur - p2.toks_beg) == static_cast<std::size_t>(parser->toks_cur - parser->toks_beg));

            pack_sum_us += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            unpack_sum_us += std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
        }

        const double ratio = raw_sz ? static_cast<double>(packed_sz) / static_cast<double>(raw_sz) : 0.0;
        const auto slash = path.find_last_of('/');
        const std::string short_name = (slash == std::string::npos) ? path : path.substr(slash + 1);

        std::cout
            << std::left
            << std::setw(24) << short_name
            << std::setw(12) << toknum
            << std::setw(14) << raw_sz
            << std::setw(16) << packed_sz
            << std::setw(10) << std::fixed << std::setprecision(3) << ratio
            << std::setw(14) << (pack_sum_us / runs)
            << std::setw(14) << (unpack_sum_us / runs)
            << '\n';

        flatjson::free_parser(parser);
    }

    return 0;
}
