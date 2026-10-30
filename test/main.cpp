
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
#include <flatjson/compare.hpp>
#include <flatjson/serialize.hpp>
#include <flatjson/state.hpp>

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <map>
#include <deque>
#include <functional>

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <limits>

#ifdef WIN32
static const char dir_separator = '\\';
#else
static const char dir_separator = '/';
#endif // WIN32

// conditions for `flag`:
// skip - to skip the test
// this - to perform only tests marked as `this`
// only - to perform only one test marked as `only`

enum class test_cond {
     skip_
    ,this_
    ,only_
    ,none_
};

#define FJ_TEST(flag, ...) []() -> std::pair<test_cond, const char *> { \
        const char *fileline = __FILE__ "(" FJ_STRINGIZE(__LINE__) ")"; \
        const char *ptr = std::strrchr(fileline, dir_separator); \
        ptr = ptr ? ptr+1 : "flatjson-test" "(" FJ_STRINGIZE(__LINE__) ")"; \
        static char buf[256]{0}; \
        int len = std::snprintf(buf, sizeof(buf), "%2d: %s", __COUNTER__, ptr); \
        static const char title[] = FJ_STRINGIZE(__VA_ARGS__); \
        const ::flatjson::string_view strflag{FJ_STRINGIZE(flag)}; \
        if ( sizeof(title)-1 != 0 ) { \
            std::snprintf(buf+len, sizeof(buf)-len, "(%s)", title); \
        } else { \
            std::snprintf(buf+len, sizeof(buf)-len, "(%.*s)", (int)strflag.length(), strflag.data()); \
        } \
        auto cond = (strflag == "SKIP") \
            ? test_cond::skip_ \
            : (strflag == "THIS") \
                ? test_cond::this_ \
                : (strflag == "ONLY") \
                    ? test_cond::only_ \
                    : test_cond::none_ \
        ; \
        return std::make_pair(cond, &buf[0]); \
    }() + []()

#define FJ_STRINGIZE_IMPL(x, ...) #x
#define FJ_STRINGIZE(...) FJ_STRINGIZE_IMPL(__VA_ARGS__,)

/*************************************************************************************************/

namespace {

// Text of a token read from the token array directly. A container spread over several chunks
// gives an empty view.
flatjson::string_view token_text(
 const flatjson::parser *parser
,const flatjson::token *tok) noexcept
{
    const char *ptr = flatjson::details::span_at(parser->chunks, tok->vpos, tok->vlen);

    return ptr ? flatjson::string_view{ptr, tok->vlen} : flatjson::string_view{};
}

// Bytes outside 0x20..0x7E are shown as '.'. The caret sits under byte in_dump_pos,
// at column 2 + 3 * in_dump_pos: the "c|" prefix, then three columns per byte.
std::string expected_error_message(const flatjson::error_info &e) {
    std::string prev;
    if ( e.prev_func ) {
        prev = std::string{", prev function \""} + e.prev_func + "()\"";
    }
    char head[256];
    const int n = std::snprintf(
         head
        ,sizeof(head)
        ,"%s, in function \"%s()\"(%u)%s, at json position %u, content:\n"
        ,flatjson::error_string(e.code)
        ,e.func
        ,e.line
        ,prev.c_str()
        ,e.js_pos
    );
    assert(n > 0 && static_cast<std::size_t>(n) < sizeof(head));

    std::string msg{head, static_cast<std::size_t>(n)};
    msg += "c|";
    for ( int i = 0; i < 16; ++i ) {
        const auto byte = static_cast<unsigned char>(e.js_str[i]);
        const char shown = (byte >= 0x20 && byte <= 0x7e) ? static_cast<char>(byte) : '.';
        msg += shown;
        msg += (i + 1 == 16) ? " |" : "  ";
    }
    msg += '\n';
    msg += 'x';
    for ( int i = 0; i < 16; ++i ) {
        char hex[8];
        std::snprintf(
             hex
            ,sizeof(hex)
            ,"|%02x"
            ,static_cast<unsigned>(static_cast<unsigned char>(e.js_str[i]))
        );
        msg += hex;
    }
    msg += "|\n";
    msg.append(static_cast<std::size_t>(2 + 3 * e.in_dump_pos), ' ');
    msg += "^\n";

    return msg;
}

std::string assert_error_message(const flatjson::error_info &e) {
    const std::string msg{flatjson::error_message(e)};
    const std::string expected = expected_error_message(e);
    if ( msg != expected ) {
        std::fprintf(stderr, "--- actual\n%s--- expected\n%s", msg.c_str(), expected.c_str());
    }
    assert(msg == expected);

    return msg;
}

void *fail_alloc(void *, std::size_t) { return nullptr; }

void fail_alloc_free(void *) {}

void *fail_realloc(void *ptr, std::size_t size) {
    if ( ptr ) {
        return nullptr;
    }

    return std::malloc(size);
}

void fail_realloc_free(void *ptr) { std::free(ptr); }

} // anon ns

/*************************************************************************************************/

template<typename F>
std::pair<std::pair<test_cond, std::string>, std::function<void()>>
operator+ (const std::pair<test_cond, const char *> cond_and_caption, F &&f) {
    return {{cond_and_caption.first, cond_and_caption.second}, std::forward<F>(f)};
}

struct test_t {
    test_t& operator+= (std::pair<std::pair<test_cond, std::string>, std::function<void()>> pair) {
        std::string caption = pair.first.second;
        auto cond = pair.first.first;
        auto skip = std::move(pair.second);
        auto res  = m_funcs.insert(
            std::make_pair(
                 std::move(caption)
                ,std::make_pair(cond, std::move(skip))
            )
        );
        assert(res.second);

        only_one_mode = (cond == test_cond::only_) ? true : only_one_mode;
        only_this_mode= (cond == test_cond::this_) ? true : only_this_mode;

        return *this;
    }
    void run() {
        for ( const auto &it: m_funcs ) {
            const auto &caption = it.first;
            const auto cond = it.second.first;
            const auto &func = it.second.second;
            if ( only_one_mode ) {
                if ( cond != test_cond::only_ ) continue;
                std::cout << "unit " << caption << "..." << std::flush;
                func();
                std::cout << "passed!" << std::endl;
                break;
            }

            if ( only_this_mode ) {
                if ( cond != test_cond::this_ ) continue;
                std::cout << "unit " << caption << "..." << std::flush;
                func();
                std::cout << "passed!" << std::endl;
            } else {
                if ( cond == test_cond::skip_ ) {
                    std::cout << "unit " << caption << " SKIPPED!" << std::endl;
                } else {
                    std::cout << "unit " << caption << "..." << std::flush;
                    func();
                    std::cout << "passed!" << std::endl;
                }
            }
        }
    }

private:
    std::map<std::string, std::pair<test_cond, std::function<void()>>> m_funcs;
    bool only_one_mode = false;
    bool only_this_mode = false;
};

/*************************************************************************************************/

struct {
    void* alloc(std::size_t size) {
        void *ptr = std::malloc(size);
        m_map.emplace(ptr, stat_t{ptr, size});

        return ptr;
    }
    void* realloc(void *ptr, std::size_t size) {
        if ( !ptr ) {
            return alloc(size);
        }
        auto it = m_map.find(ptr);
        assert(it != m_map.end());
        void *mem = std::realloc(ptr, size);
        if ( !mem ) {
            return nullptr;
        }
        m_map.erase(it);
        m_map.emplace(mem, stat_t{mem, size});
        ++m_reallocs;

        return mem;
    }
    void free(const void *ptr) {
        auto it = m_map.find(ptr);
        assert(it != m_map.end());

        std::free(it->second.ptr);
        m_map.erase(it);
    }
    bool valid() const { return m_map.empty(); }

    std::size_t allocations() const { return m_map.size(); }
    std::size_t reallocations() const { return m_reallocs; }
    std::size_t total_alloc() const {
        std::size_t res{};
        for ( const auto &it: m_map ) {
            res += it.second.size;
        }

        return res;
    }

    void reset() {
        for ( auto &it: m_map ) {
            void *ptr = it.second.ptr;
            std::free(ptr);
        }

        m_map.clear();
    }

    struct stat_t {
        void *ptr;
        std::size_t size;
    };

    void dump(std::ostream &os) const {
        for ( const auto &it: m_map) {
            os << it.second.ptr << ": " << it.second.size << std::endl;
        }
    }

private:

    std::map<const void *, stat_t> m_map;
    std::size_t m_reallocs{};
} myallocator;

static void* my_realloc(void *ptr, std::size_t size) { return myallocator.realloc(ptr, size); }
static void my_free(void *ptr) { return myallocator.free(ptr); }

/*************************************************************************************************/

std::string read_file(const char *fname) {
    std::string str;
    const std::string path0{fname};
    const std::string path1 = std::string{".."} + dir_separator + path0;

    const auto try_read = [&](const std::string &path) -> bool {
        std::ifstream file{path, std::ios::binary};
        if ( !file ) {
            return false;
        }
        str.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});

        return !file.bad();
    };

    if ( !try_read(path0) ) {
        const bool is_absolute = !path0.empty() && path0.front() == dir_separator;
        if ( !is_absolute ) {
            const bool ok = try_read(path1);
            assert(ok);
        } else {
            assert(false);
        }
    }

    return str;
}

/*************************************************************************************************/

struct test_result {
    int ec;
    int expected;
    std::string emsg;
    const char *path;
    const char *fname;
};

flatjson::error_info parse_file(std::size_t *readed, const char *path, const char *fname) {
    std::string p{path};
    p += dir_separator;
    p += fname;

    std::string content = read_file(p.c_str());
    assert(!content.empty());

    (*readed) += content.size();
    content.push_back('\0');

    flatjson::error_info perr{};
    auto *parser = flatjson::parse(
         flatjson::args.json_beg = content.c_str()
        ,flatjson::args.json_end = content.c_str() + content.length()
        ,flatjson::args.error_info = &perr
    );
    if ( !parser ) {
        if ( perr.code == flatjson::FJ_OK ) {
            perr.code = flatjson::FJ_INVALID;
        }
        return perr;
    }

    auto ec = parser->error;

    flatjson::free_parser(parser);

    return ec;
}

enum class chunked_first_phase {
     incomplete
    ,sv_cross_chunk
    ,non_ok
};

template<typename VerifyFn>
void run_chunked_case(
     const char *chunk0
    ,const char *chunk1
    ,chunked_first_phase first_phase
    ,VerifyFn verify)
{

    char json[256]{};
    const auto len0 = std::strlen(chunk0);
    const auto len1 = std::strlen(chunk1);
    assert(len0 + len1 + 1 < sizeof(json));

    std::memcpy(json, chunk0, len0);
    json[len0] = '\0';

    flatjson::token toks[64]{};
    flatjson::chunk chunks[2];
    auto parser = flatjson::make_parser(
         flatjson::args.toks_beg = toks
        ,flatjson::args.toks_end = toks + 64
        ,flatjson::args.chunks_beg = chunks
        ,flatjson::args.chunks_end = chunks + 2
    );

    auto cur = flatjson::feed(&parser, json, len0);
    if ( first_phase == chunked_first_phase::incomplete ) {
        assert(cur.code == flatjson::FJ_INCOMPLETE);
    } else if ( first_phase == chunked_first_phase::sv_cross_chunk ) {
        assert(cur.code == flatjson::FJ_SV_CROSS_CHUNK);
    } else {
        assert(cur.code != flatjson::FJ_OK);
    }

    // The second chunk in a buffer of its own.
    char tail[256]{};
    std::memcpy(tail, chunk1, len1);
    cur = flatjson::feed(&parser, tail, len1);
    if ( first_phase != chunked_first_phase::incomplete ) {
        // A cut leaf breaks the chunk contract: the error stays until reset().
        assert(cur.code != flatjson::FJ_OK);
        assert(cur.code != flatjson::FJ_INCOMPLETE);

        flatjson::reset(&parser);
        std::memcpy(json + len0, chunk1, len1);
        cur = flatjson::feed(&parser, json, len0 + len1);
    }
    assert(cur.code == flatjson::FJ_OK);
    assert(flatjson::num_tokens(&parser) > 0);
    assert(flatjson::is_valid(&parser));

    verify(&parser);
}

test_result real_test(
     std::size_t *readed
    ,std::size_t *scounter
    ,std::size_t *fcounter
    ,std::size_t *tcounter
    ,const char *path
    ,const char **tests)
{
    for ( const char **it = tests; *it; ++it ) {
        int expected_error = *it[0] - '0';
        const char *fname = (*it)+2;
        std::fprintf(stdout, "test %2zu: %s%c%s", *tcounter, path, dir_separator, fname);
        std::fflush(stdout);
        if ( std::strcmp(fname, "n_multidigit_number_then_00.json") == 0 ) {
            std::cout << std::flush;
        }

        (*tcounter)++;
        auto ec = parse_file(readed, path, fname);
        int received_error = ec.code != flatjson::FJ_OK;
        std::fprintf(stdout, ", expected=%s, received=%s\n"
            ,expected_error ? "ERROR" : "OK"
            ,received_error ? "ERROR" : "OK"
        );
        std::fflush(stdout);

        if ( received_error ) {
            (*fcounter)++;
        } else {
            (*scounter)++;
        }

        if ( expected_error != received_error ) {
            return {ec.code, expected_error, flatjson::error_message(ec), path, *it+2};
        }
    }

    return {};
}

test_result test_conformance() {
    std::size_t readed{}, test_counter{};

    const std::size_t expected_success_part0 = 3;
    const std::size_t expected_failure_part0 = 29;
    std::size_t part0_scounter{}, part0_fcounter{};

    // excluded:
    // "fail29.json"
    // "fail30.json"

    static const char *part0_path = "jsonchecker";
    static const char *part0[] = {
         "0:pass01.json","0:pass02.json","0:pass03.json","1:fail02.json"
        ,"1:fail03.json","1:fail04.json","1:fail05.json","1:fail06.json"
        ,"1:fail07.json","1:fail08.json","1:fail09.json","1:fail10.json"
        ,"1:fail11.json","1:fail12.json","1:fail13.json","1:fail14.json"
        ,"1:fail15.json","1:fail16.json","1:fail17.json","1:fail19.json"
        ,"1:fail20.json","1:fail21.json","1:fail22.json","1:fail23.json"
        ,"1:fail24.json","1:fail25.json","1:fail26.json","1:fail27.json"
        ,"1:fail28.json","1:fail31.json","1:fail32.json","1:fail33.json"
        ,nullptr
    };
    auto ec = real_test(&readed, &part0_scounter, &part0_fcounter, &test_counter, part0_path, part0);
    if ( ec.expected != ec.ec ) {
        return ec;
    }
    assert(readed == 2401);
    assert(part0_scounter == expected_success_part0);
    assert(part0_fcounter == expected_failure_part0);

    const std::size_t expected_success_part1 = 28;
    const std::size_t expected_failure_part1 = 0;
    std::size_t part1_scounter{}, part1_fcounter{};
    static const char *part1_path = "roundtrip";
    static const char *part1[] = {
         "0:roundtrip01.json","0:roundtrip04.json","0:roundtrip07.json","0:roundtrip10.json"
        ,"0:roundtrip13.json","0:roundtrip16.json","0:roundtrip19.json","0:roundtrip22.json"
        ,"0:roundtrip25.json","0:roundtrip02.json","0:roundtrip05.json","0:roundtrip08.json"
        ,"0:roundtrip11.json","0:roundtrip14.json","0:roundtrip17.json","0:roundtrip20.json"
        ,"0:roundtrip23.json","0:roundtrip26.json","0:roundtrip03.json","0:roundtrip06.json"
        ,"0:roundtrip09.json","0:roundtrip12.json","0:roundtrip15.json","0:roundtrip18.json"
        ,"0:roundtrip21.json","0:roundtrip24.json","0:roundtrip27.json","0:roundtrip-double.json"
        ,nullptr
    };
    ec = real_test(&readed, &part1_scounter, &part1_fcounter, &test_counter, part1_path, part1);
    if ( ec.ec != flatjson::FJ_OK ) {
        return ec;
    }
    assert(readed == 3048);
    assert(part1_scounter == expected_success_part1);
    assert(part1_fcounter == expected_failure_part1);

    // excluded:
    // "n_structure_100000_opening_arrays.json" - because I think it is imposible.
    // "n_structure_no_data.json" - because nonsensical.
    // "n_structure_open_array_object.json" - imposible case.
    // "n_multidigit_number_then_00.json" - I think trailing zero bytes should interpreted as white-space bytes.
    // "n_structure_null-byte-outside-string.json" - the same.
    // "n_structure_whitespace_formfeed.json" - the same

    const std::size_t expected_success_part2 = 95;
    const std::size_t expected_failure_part2 = 182;
    std::size_t part2_scounter{}, part2_fcounter{};
    static const char *part2_path = "test_parsing";
    static const char *part2[] = {
         "1:n_array_1_true_without_comma.json", "1:n_array_a_invalid_utf8.json", "1:n_array_colon_instead_of_comma.json"
        ,"1:n_array_comma_after_close.json", "1:n_array_comma_and_number.json", "1:n_array_double_comma.json"
        ,"1:n_array_double_extra_comma.json", "1:n_array_extra_close.json", "1:n_array_extra_comma.json"
        ,"1:n_array_incomplete_invalid_value.json", "1:n_array_incomplete.json", "1:n_array_inner_array_no_comma.json"
        ,"1:n_array_invalid_utf8.json", "1:n_array_items_separated_by_semicolon.json", "1:n_array_just_comma.json"
        ,"1:n_array_just_minus.json", "1:n_array_missing_value.json", "1:n_array_newlines_unclosed.json"
        ,"1:n_array_number_and_comma.json", "1:n_array_number_and_several_commas.json", "1:n_array_spaces_vertical_tab_formfeed.json"
        ,"1:n_array_star_inside.json", "1:n_array_unclosed.json", "1:n_array_unclosed_trailing_comma.json"
        ,"1:n_array_unclosed_with_new_lines.json", "1:n_array_unclosed_with_object_inside.json", "1:n_incomplete_false.json"
        ,"1:n_incomplete_null.json", "1:n_incomplete_true.json", "1:n_number_0.1.2.json"
        ,"1:n_number_-01.json", "1:n_number_0.3e+.json", "1:n_number_0.3e.json", "1:n_number_0_capital_E+.json"
        ,"1:n_number_0_capital_E.json", "1:n_number_0.e1.json", "1:n_number_0e+.json", "1:n_number_0e.json", "1:n_number_1_000.json"
        ,"1:n_number_1.0e+.json", "1:n_number_1.0e-.json", "1:n_number_1.0e.json", "1:n_number_-1.0..json", "1:n_number_1eE2.json"
        ,"1:n_number_+1.json", "1:n_number_.-1.json", "1:n_number_2.e+3.json", "1:n_number_2.e-3.json", "1:n_number_2.e3.json"
        ,"1:n_number_.2e-3.json", "1:n_number_-2..json", "1:n_number_9.e+.json", "1:n_number_expression.json"
        ,"1:n_number_hex_1_digit.json", "1:n_number_hex_2_digits.json", "1:n_number_infinity.json", "1:n_number_+Inf.json"
        ,"1:n_number_Inf.json", "1:n_number_invalid+-.json", "1:n_number_invalid-negative-real.json"
        ,"1:n_number_invalid-utf-8-in-bigger-int.json", "1:n_number_invalid-utf-8-in-exponent.json"
        ,"1:n_number_invalid-utf-8-in-int.json", "1:n_number_++.json", "1:n_number_minus_infinity.json"
        ,"1:n_number_minus_sign_with_trailing_garbage.json", "1:n_number_minus_space_1.json", "1:n_number_-NaN.json"
        ,"1:n_number_NaN.json", "1:n_number_neg_int_starting_with_zero.json", "1:n_number_neg_real_without_int_part.json"
        ,"1:n_number_neg_with_garbage_at_end.json", "1:n_number_real_garbage_after_e.json", "1:n_number_real_with_invalid_utf8_after_e.json"
        ,"1:n_number_real_without_fractional_part.json", "1:n_number_starting_with_dot.json", "1:n_number_U+FF11_fullwidth_digit_one.json"
        ,"1:n_number_with_alpha_char.json", "1:n_number_with_alpha.json", "1:n_number_with_leading_zero.json", "1:n_object_bad_value.json"
        ,"1:n_object_bracket_key.json", "1:n_object_comma_instead_of_colon.json", "1:n_object_double_colon.json", "1:n_object_emoji.json"
        ,"1:n_object_garbage_at_end.json", "1:n_object_key_with_single_quotes.json", "1:n_object_lone_continuation_byte_in_key_and_trailing_comma.json"
        ,"1:n_object_missing_colon.json", "1:n_object_missing_key.json", "1:n_object_missing_semicolon.json", "1:n_object_missing_value.json"
        ,"1:n_object_no-colon.json", "1:n_object_non_string_key_but_huge_number_instead.json", "1:n_object_non_string_key.json"
        ,"1:n_object_repeated_null_null.json", "1:n_object_several_trailing_commas.json", "1:n_object_single_quote.json"
        ,"1:n_object_trailing_comma.json", "1:n_object_trailing_comment.json", "1:n_object_trailing_comment_open.json"
        ,"1:n_object_trailing_comment_slash_open_incomplete.json", "1:n_object_trailing_comment_slash_open.json"
        ,"1:n_object_two_commas_in_a_row.json", "1:n_object_unquoted_key.json", "1:n_object_unterminated-value.json"
        ,"1:n_object_with_single_string.json", "1:n_object_with_trailing_garbage.json", "1:n_single_space.json"
        ,"1:n_string_1_surrogate_then_escape.json", "1:n_string_1_surrogate_then_escape_u1.json", "1:n_string_1_surrogate_then_escape_u1x.json"
        ,"1:n_string_1_surrogate_then_escape_u.json", "1:n_string_accentuated_char_no_quotes.json", "1:n_string_backslash_00.json"
        ,"1:n_string_escaped_backslash_bad.json", "1:n_string_escaped_ctrl_char_tab.json", "1:n_string_escaped_emoji.json"
        ,"1:n_string_escape_x.json", "1:n_string_incomplete_escaped_character.json", "1:n_string_incomplete_escape.json"
        ,"1:n_string_incomplete_surrogate_escape_invalid.json", "1:n_string_incomplete_surrogate.json", "1:n_string_invalid_backslash_esc.json"
        ,"1:n_string_invalid_unicode_escape.json", "1:n_string_invalid_utf8_after_escape.json", "1:n_string_invalid-utf-8-in-escape.json"
        ,"1:n_string_leading_uescaped_thinspace.json", "1:n_string_no_quotes_with_bad_escape.json", "1:n_string_single_doublequote.json"
        ,"1:n_string_single_quote.json", "1:n_string_single_string_no_double_quotes.json", "1:n_string_start_escape_unclosed.json"
        ,"1:n_string_unescaped_crtl_char.json", "1:n_string_unescaped_newline.json", "1:n_string_unescaped_tab.json"
        ,"1:n_string_unicode_CapitalU.json", "1:n_string_with_trailing_garbage.json"
        ,"1:n_structure_angle_bracket_..json", "1:n_structure_angle_bracket_null.json", "1:n_structure_array_trailing_garbage.json"
        ,"1:n_structure_array_with_extra_array_close.json", "1:n_structure_array_with_unclosed_string.json"
        ,"1:n_structure_ascii-unicode-identifier.json", "1:n_structure_capitalized_True.json", "1:n_structure_close_unopened_array.json"
        ,"1:n_structure_comma_instead_of_closing_brace.json", "1:n_structure_double_array.json", "1:n_structure_end_array.json"
        ,"1:n_structure_incomplete_UTF8_BOM.json", "1:n_structure_lone-invalid-utf-8.json", "1:n_structure_lone-open-bracket.json"
        ,"1:n_structure_number_with_trailing_garbage.json"
        ,"1:n_structure_object_followed_by_closing_object.json", "1:n_structure_object_unclosed_no_value.json"
        ,"1:n_structure_object_with_comment.json", "1:n_structure_object_with_trailing_garbage.json"
        ,"1:n_structure_open_array_apostrophe.json", "1:n_structure_open_array_comma.json"
        ,"1:n_structure_open_array_open_object.json", "1:n_structure_open_array_open_string.json", "1:n_structure_open_array_string.json"
        ,"1:n_structure_open_object_close_array.json", "1:n_structure_open_object_comma.json", "1:n_structure_open_object.json"
        ,"1:n_structure_open_object_open_array.json", "1:n_structure_open_object_open_string.json"
        ,"1:n_structure_open_object_string_with_apostrophes.json", "1:n_structure_open_open.json", "1:n_structure_single_eacute.json"
        ,"1:n_structure_single_star.json", "1:n_structure_trailing_#.json", "1:n_structure_U+2060_word_joined.json"
        ,"1:n_structure_uescaped_LF_before_string.json", "1:n_structure_unclosed_array.json", "1:n_structure_unclosed_array_partial_null.json"
        ,"1:n_structure_unclosed_array_unfinished_false.json", "1:n_structure_unclosed_array_unfinished_true.json"
        ,"1:n_structure_unclosed_object.json", "1:n_structure_unicode-identifier.json", "1:n_structure_UTF8_BOM_no_data.json"
        ,"1:n_structure_whitespace_U+2060_word_joiner.json", "0:y_array_arraysWithSpaces.json"
        ,"0:y_array_empty.json", "0:y_array_empty-string.json", "0:y_array_ending_with_newline.json", "0:y_array_false.json"
        ,"0:y_array_heterogeneous.json", "0:y_array_null.json", "0:y_array_with_1_and_newline.json", "0:y_array_with_leading_space.json"
        ,"0:y_array_with_several_null.json", "0:y_array_with_trailing_space.json", "0:y_number_0e+1.json", "0:y_number_0e1.json"
        ,"0:y_number_after_space.json", "0:y_number_double_close_to_zero.json", "0:y_number_int_with_exp.json", "0:y_number.json"
        ,"0:y_number_minus_zero.json", "0:y_number_negative_int.json", "0:y_number_negative_one.json", "0:y_number_negative_zero.json"
        ,"0:y_number_real_capital_e.json", "0:y_number_real_capital_e_neg_exp.json", "0:y_number_real_capital_e_pos_exp.json"
        ,"0:y_number_real_exponent.json", "0:y_number_real_fraction_exponent.json", "0:y_number_real_neg_exp.json"
        ,"0:y_number_real_pos_exponent.json", "0:y_number_simple_int.json", "0:y_number_simple_real.json", "0:y_object_basic.json"
        ,"0:y_object_duplicated_key_and_value.json", "0:y_object_duplicated_key.json", "0:y_object_empty.json", "0:y_object_empty_key.json"
        ,"0:y_object_escaped_null_in_key.json", "0:y_object_extreme_numbers.json", "0:y_object.json", "0:y_object_long_strings.json"
        ,"0:y_object_simple.json", "0:y_object_string_unicode.json", "0:y_object_with_newlines.json"
        ,"0:y_string_1_2_3_bytes_UTF-8_sequences.json", "0:y_string_accepted_surrogate_pair.json", "0:y_string_accepted_surrogate_pairs.json"
        ,"0:y_string_allowed_escapes.json", "0:y_string_backslash_and_u_escaped_zero.json", "0:y_string_backslash_doublequotes.json"
        ,"0:y_string_comments.json", "0:y_string_double_escape_a.json", "0:y_string_double_escape_n.json"
        ,"0:y_string_escaped_control_character.json", "0:y_string_escaped_noncharacter.json", "0:y_string_in_array.json"
        ,"0:y_string_in_array_with_leading_space.json", "0:y_string_last_surrogates_1_and_2.json", "0:y_string_nbsp_uescaped.json"
        ,"0:y_string_nonCharacterInUTF-8_U+10FFFF.json", "0:y_string_nonCharacterInUTF-8_U+FFFF.json", "0:y_string_null_escape.json"
        ,"0:y_string_one-byte-utf-8.json", "0:y_string_pi.json", "0:y_string_reservedCharacterInUTF-8_U+1BFFF.json"
        ,"0:y_string_simple_ascii.json", "0:y_string_space.json", "0:y_string_surrogates_U+1D11E_MUSICAL_SYMBOL_G_CLEF.json"
        ,"0:y_string_three-byte-utf-8.json", "0:y_string_two-byte-utf-8.json", "0:y_string_u+2028_line_sep.json"
        ,"0:y_string_u+2029_par_sep.json", "0:y_string_uescaped_newline.json", "0:y_string_uEscape.json"
        ,"0:y_string_unescaped_char_delete.json", "0:y_string_unicode_2.json", "0:y_string_unicodeEscapedBackslash.json"
        ,"0:y_string_unicode_escaped_double_quote.json", "0:y_string_unicode.json", "0:y_string_unicode_U+10FFFE_nonchar.json"
        ,"0:y_string_unicode_U+1FFFE_nonchar.json", "0:y_string_unicode_U+200B_ZERO_WIDTH_SPACE.json"
        ,"0:y_string_unicode_U+2064_invisible_plus.json", "0:y_string_unicode_U+FDD0_nonchar.json", "0:y_string_unicode_U+FFFE_nonchar.json"
        ,"0:y_string_utf8.json", "0:y_string_with_del_character.json", "0:y_structure_lonely_false.json", "0:y_structure_lonely_int.json"
        ,"0:y_structure_lonely_negative_real.json", "0:y_structure_lonely_null.json", "0:y_structure_lonely_string.json"
        ,"0:y_structure_lonely_true.json", "0:y_structure_string_empty.json", "0:y_structure_trailing_newline.json"
        ,"0:y_structure_true_in_array.json", "0:y_structure_whitespace_array.json"
        ,nullptr
    };
    ec = real_test(&readed, &part2_scounter, &part2_fcounter, &test_counter, part2_path, part2);
    if ( ec.ec != ec.expected ) {
        return ec;
    }
    assert(readed == 5499);
    assert(part2_scounter == expected_success_part2);
    assert(part2_fcounter == expected_failure_part2);

    return {};
}

/*************************************************************************************************/

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif // _MSC_VER

int main() {
    /*************************************************************************************************/
    std::cout << "sizeof(fj_token) = " << sizeof(flatjson::token) << std::endl;
    std::cout << "version str=" << FJ_VERSION_STRING << std::endl;
#ifdef FJ_SIMD_TYPE
    std::cout << "simd type=" << FJ_SIMD_TYPE << std::endl;
#endif // FJ_SIMD_TYPE

    test_t test;

    /*************************************************************************************************/

    static_assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_INVALID) == false, "");
    static_assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_STRING) == true, "");
    static_assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_NUMBER) == true, "");
    static_assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_BOOL) == true, "");
    static_assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_NULL) == true, "");
    static_assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_OBJECT) == false, "");
    static_assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_OBJECT_END) == false, "");
    static_assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_ARRAY) == false, "");
    static_assert(fj_is_simple_type_macro(flatjson::FJ_TYPE_ARRAY_END) == false, "");

    /*************************************************************************************************/

    test += FJ_TEST(test for empty JSON) {
        static const char str[] = R"()";

        auto *parser = flatjson::alloc_parser();
        assert(parser != nullptr);
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );
        assert(toknum == 0);
        assert(!flatjson::is_valid(parser));
        assert(flatjson::get_error(parser) == flatjson::FJ_INVALID);
        assert(std::strcmp(parser->error.func, "parse") == 0);
        assert(std::strcmp(parser->error.prev_func, "parse") == 0);
        assert(parser->error.js_pos == 0);
        assert(parser->error.in_dump_pos == 0);
        assert_error_message(parser->error);

        // No document was bound: the report still has to be a filled error_info.
        flatjson::reset(parser);
        assert(flatjson::parse(parser) == 0);
        assert(flatjson::get_error(parser) == flatjson::FJ_INVALID);
        assert(parser->error.js_pos == 0);
        assert(parser->error.in_dump_pos == 0);
        assert_error_message(parser->error);
        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for json_end with and without the terminating NUL) {
        struct outcome {
            std::size_t toks;
            flatjson::error_code code;
            std::string value;
        };
        const auto run = [](const std::string &doc, bool with_nul) {
            auto *p = flatjson::alloc_parser();
            const char *beg = doc.c_str();
            const auto toks = flatjson::parse(
                 p
                ,flatjson::args.json_beg = beg
                ,flatjson::args.json_end = beg + doc.size() + (with_nul ? 1u : 0u)
            );
            outcome out{toks, flatjson::get_error(p), {}};
            if ( flatjson::is_valid(p) ) {
                out.value = flatjson::begin(p).to_string();
            }
            flatjson::free_parser(p);

            return out;
        };

        struct expected {
            const char *doc;
            flatjson::error_code code;
            const char *value;
        };
        const expected docs[] = {
             {"123", flatjson::FJ_OK, "123"}
            ,{"-1.5e+3", flatjson::FJ_OK, "-1.5e+3"}
            ,{"\"abc\"", flatjson::FJ_OK, "abc"}
            ,{"true", flatjson::FJ_OK, "true"}
            ,{"null", flatjson::FJ_OK, "null"}
            ,{"  42  ", flatjson::FJ_OK, "42"}
            ,{"[1,2]", flatjson::FJ_OK, ""}
            ,{"{\"a\":1}", flatjson::FJ_OK, ""}
            ,{"", flatjson::FJ_INVALID, ""}
            ,{" ", flatjson::FJ_INCOMPLETE, ""}
            ,{"[1,2", flatjson::FJ_INCOMPLETE, ""}
            ,{"\"abc", flatjson::FJ_SV_CROSS_CHUNK, ""}
            ,{"12 3", flatjson::FJ_INVALID, ""}
            ,{"[1] x", flatjson::FJ_INVALID, ""}
        };
        for ( const auto &e: docs ) {
            const auto exact = run(e.doc, false);
            const auto nul = run(e.doc, true);
            assert(exact.code == e.code);
            assert(exact.value == e.value);
            assert(nul.code == exact.code);
            assert(nul.toks == exact.toks);
            assert(nul.value == exact.value);
        }

        // A NUL inside the range is not the terminator.
        static const char inner_nul[] = {'[', '1', ']', '\0', ' '};
        auto *p = flatjson::alloc_parser();
        flatjson::parse(
             p
            ,flatjson::args.json_beg = std::begin(inner_nul)
            ,flatjson::args.json_end = std::end(inner_nul)
        );
        assert(flatjson::get_error(p) == flatjson::FJ_INVALID);
        flatjson::free_parser(p);
    };

    test += FJ_TEST(test for empty string) {
        static const char str[] = R"("")";

        auto *parser = flatjson::alloc_parser();
        assert(parser);
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_valid());
        assert(beg.is_string());
        assert(beg.to_string_view().size() == 0);

        auto end = flatjson::end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for raw control chars inside a string) {
        auto parse_str = [](const std::string &body, std::size_t *len) {
            const std::string json = "[\"" + body + "\"]";
            flatjson::token toks[4];
            auto p = flatjson::make_parser(
                 flatjson::args.toks_beg = toks
                ,flatjson::args.toks_end = toks + 4
            );
            flatjson::error_code ec = flatjson::FJ_OK;
            flatjson::parse(
                 &p
                ,flatjson::args.json_beg = json.c_str()
                ,flatjson::args.json_end = json.c_str() + json.size() + 1
                ,flatjson::args.error_code = &ec
            );
            if ( ec == flatjson::FJ_OK ) {
                *len = toks[1].vlen;
            }

            return ec;
        };

        // every offset inside the 16- and 8-byte blocks and the byte tail
        for ( std::size_t prefix = 0; prefix < 40; ++prefix ) {
            for ( int c = 0x01; c < 0x20; ++c ) {
                std::string body(prefix, 'a');
                body += static_cast<char>(c);
                body += "bcdefghijklmn";
                std::size_t len = 0;
                assert(parse_str(body, &len) == flatjson::FJ_INVALID);
            }

            const std::string plain = std::string(prefix, 'a') + "\x7f\xc3\xa9z";
            std::size_t len = 0;
            assert(parse_str(plain, &len) == flatjson::FJ_OK);
            assert(len == plain.size());

            const std::string escaped = std::string(prefix, 'a') + "\\\"\\\\\\u00e9tail";
            len = 0;
            assert(parse_str(escaped, &len) == flatjson::FJ_OK);
            assert(len == escaped.size());
        }
    };

    test += FJ_TEST(test for strings and numbers across scan blocks) {
        auto parse_one = [](const std::string &json, std::size_t *len) {
            flatjson::token toks[4];
            auto p = flatjson::make_parser(
                 flatjson::args.toks_beg = toks
                ,flatjson::args.toks_end = toks + 4
            );
            flatjson::error_code ec = flatjson::FJ_OK;
            flatjson::parse(
                 &p
                ,flatjson::args.json_beg = json.c_str()
                ,flatjson::args.json_end = json.c_str() + json.size() + 1
                ,flatjson::args.error_code = &ec
            );
            if ( ec == flatjson::FJ_OK ) {
                *len = toks[1].vlen;
            }

            return ec;
        };
        auto feed_split = [](const std::string &json, std::size_t split, std::size_t *len) {
            flatjson::token toks[4]{};
            flatjson::chunk chunks[2];
            auto p = flatjson::make_parser(
                 flatjson::args.toks_beg = toks
                ,flatjson::args.toks_end = toks + 4
                ,flatjson::args.chunks_beg = chunks
                ,flatjson::args.chunks_end = chunks + 2
            );
            const std::string head = json.substr(0, split);
            const std::string tail = json.substr(split);
            flatjson::feed(&p, head.data(), head.size());
            const auto c = flatjson::feed(&p, tail.data(), tail.size());
            if ( c.code == flatjson::FJ_OK ) {
                *len = toks[1].vlen;
            }

            return c.code;
        };
        auto check_value = [&](const std::string &json, std::size_t expected_len) {
            std::size_t len = 0;
            assert(parse_one(json, &len) == flatjson::FJ_OK);
            assert(len == expected_len);
            // The leaf starts after '['; a number also needs the byte after it in its chunk.
            const bool is_number = json[1] != '"';
            const std::size_t whole_from = is_number ? 1 + expected_len + 1 : 1 + expected_len + 2;
            for ( std::size_t split = 1; split < json.size(); ++split ) {
                len = 0;
                const bool whole = split == 1 || split >= whole_from;
                assert(feed_split(
                     json
                    ,split
                    ,&len
                ) == (whole ? flatjson::FJ_OK : flatjson::FJ_SV_CROSS_CHUNK));
                if ( whole ) {
                    assert(len == expected_len);
                }
            }
        };

        for ( std::size_t n = 0; n < 40; ++n ) {
            const std::string plain(n, 'a');
            check_value("[\"" + plain + "\"]", plain.size());
            const std::string escaped = plain + "\\n" + plain + "\\u00e9";
            check_value("[\"" + escaped + "\"]", escaped.size());
        }

        const char *exps[] = {"", "e5", "E-12", "e+123"};
        for ( std::size_t int_len = 1; int_len < 21; ++int_len ) {
            for ( std::size_t frac_len = 0; frac_len < 19; frac_len += 3 ) {
                for ( const char *exp: exps ) {
                    std::string num = "-";
                    for ( std::size_t i = 0; i < int_len; ++i ) {
                        num += static_cast<char>('1' + i % 9);
                    }
                    if ( frac_len ) {
                        num += '.';
                        num += std::string(frac_len, '7');
                    }
                    num += exp;
                    check_value("[" + num + "]", num.size());
                    check_value("[" + num + " ]", num.size());
                }
            }
        }

        const char *bad_tails[] = {".]", ".e5]", "e]", "e+]", "x]", "0]"};
        for ( std::size_t int_len = 1; int_len < 21; ++int_len ) {
            for ( const char *tail: bad_tails ) {
                const std::string digits = std::string(1, '1') + std::string(int_len - 1, '0');
                const std::string json = "[" + digits + tail;
                std::size_t len = 0;
                const bool leading_zero_ok = std::strcmp(tail, "0]") == 0;
                assert(parse_one(
                     json
                    ,&len
                ) == (leading_zero_ok ? flatjson::FJ_OK : flatjson::FJ_INVALID));
                assert(parse_one("[0" + std::string(tail), &len) == flatjson::FJ_INVALID);
            }
        }
        std::size_t len = 0;
        assert(parse_one("[-]", &len) == flatjson::FJ_INVALID);
        assert(parse_one("[-a]", &len) == flatjson::FJ_INVALID);
    };

    test += FJ_TEST(test for NUMBER integer) {
        static const char str[] = R"(3)";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_valid());
        assert(beg.is_number());
        assert(beg.to_string_view() == "3");

        auto end = flatjson::end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for signed conv_to edge cases) {
        auto check_i32 = [](const char *str, std::int32_t expected) {
            auto got = flatjson::details::conv_to(str, std::strlen(str), std::int32_t{});
            assert(got == expected);
        };
        auto check_i64 = [](const char *str, std::int64_t expected) {
            auto got = flatjson::details::conv_to(str, std::strlen(str), std::int64_t{});
            assert(got == expected);
        };

        check_i32("0", 0);
        check_i32("-0", 0);
        check_i32("2147483647", std::numeric_limits<std::int32_t>::max());
        check_i32("-2147483648", std::numeric_limits<std::int32_t>::min());

        check_i64("9223372036854775807", std::numeric_limits<std::int64_t>::max());
        check_i64("-9223372036854775808", std::numeric_limits<std::int64_t>::min());
    };

    test += FJ_TEST(test for NUMBER float) {
        static const char str[] = R"(3.14)";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_valid());
        assert(beg.is_number());
        assert(beg.to_string_view() == "3.14");

        auto end = flatjson::end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for parse error message correctness) {
        // A short document is copied from its first byte through the trailing NUL the caller
        // included in the range. The cursor is within the first ten bytes, so the caret byte
        // is json[js_pos].
        auto check_parse = [](const char *json
            ,flatjson::error_code expected_code
            ,const char *func
            ,const char *prev
            ,unsigned pos)
        {
            auto *held = flatjson::alloc_parser();
            assert(held);
            flatjson::error_info ei{};
            const auto toknum = flatjson::parse(
                 held
                ,flatjson::args.json_beg = json
                ,flatjson::args.json_end = json + std::strlen(json) + 1
                ,flatjson::args.error_info = &ei
            );
            assert(toknum == 0);
            assert(ei.code == expected_code);
            assert(std::strcmp(ei.func, func) == 0);
            assert(ei.prev_func != nullptr);
            assert(std::strcmp(ei.prev_func, prev) == 0);
            assert(ei.line != 0);
            assert(ei.js_pos == pos);
            assert(ei.in_dump_pos == pos);
            const auto n = std::strlen(json) + 1;
            assert(n <= sizeof(ei.js_str));
            assert(std::memcmp(ei.js_str, json, n) == 0);
            for ( std::size_t i = n; i < sizeof(ei.js_str); ++i ) {
                assert(ei.js_str[i] == 0);
            }
            assert_error_message(ei);
            flatjson::free_parser(held);
        };

        check_parse("\"\\q\"", flatjson::FJ_INVALID, "parse_string", "parse_doc", 2);
        check_parse("\"abc", flatjson::FJ_SV_CROSS_CHUNK, "parse_string", "parse_doc", 4);
        check_parse("\"\\u12", flatjson::FJ_SV_CROSS_CHUNK, "parse_string", "parse_doc", 2);
        check_parse("[01]", flatjson::FJ_INVALID, "parse_number", "parse_doc", 1);
        check_parse("[1e+]", flatjson::FJ_INVALID, "parse_number", "parse_doc", 1);
        check_parse("{\"a\" 1}", flatjson::FJ_INVALID, "parse_doc", "parse", 5);
        check_parse("[1,", flatjson::FJ_INCOMPLETE, "parse_doc", "parse", 3);

        flatjson::token toks[2]{};
        auto limited = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 2
        );
        flatjson::error_info ei{};
        const char limited_json[] = "[1,2,3]";
        const auto limited_n = flatjson::parse(
             &limited
            ,flatjson::args.json_beg = limited_json
            ,flatjson::args.json_end = limited_json + sizeof(limited_json)
            ,flatjson::args.error_info = &ei
        );
        assert(limited_n == 0);
        assert(ei.code == flatjson::FJ_NO_FREE_TOKENS);
        assert(std::strcmp(ei.func, "ensure_token_slot") == 0);
        assert(std::strcmp(ei.prev_func, "parse_doc") == 0);
        assert(ei.js_pos == 3);
        assert(ei.in_dump_pos == 3);
        assert(std::memcmp(ei.js_str, limited_json, sizeof(limited_json)) == 0);
        for ( std::size_t i = sizeof(limited_json); i < sizeof(ei.js_str); ++i ) {
            assert(ei.js_str[i] == 0);
        }
        assert_error_message(ei);

        flatjson::chunk chunks[1];
        flatjson::token stream_toks[8]{};
        auto stream = flatjson::make_parser(
             flatjson::args.toks_beg = stream_toks
            ,flatjson::args.toks_end = stream_toks + 8
            ,flatjson::args.chunks_beg = chunks
            ,flatjson::args.chunks_end = chunks + 1
        );
        assert(flatjson::feed(&stream, "[1,", 3).code == flatjson::FJ_INCOMPLETE);
        const auto blocked = flatjson::feed(&stream, "2]", 2);
        assert(blocked.code == flatjson::FJ_NO_FREE_CHUNKS);
        assert(stream.error.code == flatjson::FJ_NO_FREE_CHUNKS);
        assert(std::strcmp(stream.error.func, "feed") == 0);
        assert(std::strcmp(stream.error.prev_func, "feed") == 0);
        assert(stream.error.js_pos == 3);
        assert(stream.error.in_dump_pos == 3);
        assert(std::memcmp(stream.error.js_str, "[1,", 3) == 0);
        for ( std::size_t i = 3; i < sizeof(stream.error.js_str); ++i ) {
            assert(stream.error.js_str[i] == 0);
        }
        assert_error_message(stream.error);

        // alloc_parser fills the code before any document exists, so the window is zeros.
        flatjson::error_info failed{};
        flatjson::error_code failed_code = flatjson::FJ_OK;
        auto *none = flatjson::alloc_parser(
             flatjson::args.mem_fns = {&fail_alloc, &fail_alloc_free}
            ,flatjson::args.error_info = &failed
            ,flatjson::args.error_code = &failed_code
        );
        assert(none == nullptr);
        assert(failed_code == flatjson::FJ_CANT_ALLOCATE);
        assert(failed.code == flatjson::FJ_CANT_ALLOCATE);
        assert(std::strcmp(failed.func, "alloc_parser") == 0);
        assert(failed.prev_func == nullptr);
        assert(failed.js_pos == 0);
        assert(failed.in_dump_pos == 0);
        assert(failed.line != 0);
        for ( std::size_t i = 0; i < sizeof(failed.js_str); ++i ) {
            assert(failed.js_str[i] == 0);
        }
        assert_error_message(failed);

        // Eighty numbers overrun the size estimate. Realloc returns null on the missing slot.
        std::string many = "[";
        for ( int i = 0; i < 80; ++i ) {
            if ( i ) {
                many += ',';
            }
            many += '1';
        }
        many += ']';
        flatjson::error_info grew{};
        auto *parser = flatjson::alloc_parser(
             flatjson::args.mem_fns = {&fail_realloc, &fail_realloc_free}
            ,flatjson::args.init_size = std::size_t{4}
        );
        assert(parser);
        const auto grew_n = flatjson::parse(
             parser
            ,flatjson::args.json_beg = many.data()
            ,flatjson::args.json_end = many.data() + many.size() + 1
            ,flatjson::args.error_info = &grew
        );
        assert(grew_n == 0);
        assert(grew.code == flatjson::FJ_CANT_ALLOCATE);
        assert(std::strcmp(grew.func, "ensure_token_slot") == 0);
        assert(std::strcmp(grew.prev_func, "parse_doc") == 0);
        assert(grew.js_pos == 111);
        assert(grew.in_dump_pos == 10);
        assert(std::memcmp(grew.js_str, many.data() + (grew.js_pos - grew.in_dump_pos), 16) == 0);
        assert(grew.js_str[grew.in_dump_pos] == '1');
        assert_error_message(grew);
        flatjson::free_parser(parser);

        flatjson::token feed_toks[4]{};
        auto fed = flatjson::make_parser(
             flatjson::args.toks_beg = feed_toks
            ,flatjson::args.toks_end = feed_toks + 4
        );
        const auto bad_feed = flatjson::feed(&fed, nullptr, 4);
        assert(bad_feed.code == flatjson::FJ_INVALID);
        assert(flatjson::get_error(&fed) == flatjson::FJ_INVALID);
        assert(std::strcmp(fed.error.func, "feed") == 0);
        assert(std::strcmp(fed.error.prev_func, "feed") == 0);
        assert(fed.error.js_pos == 0);
        assert(fed.error.in_dump_pos == 0);
        assert_error_message(fed.error);
        assert(flatjson::feed(nullptr, "[]", 2).code == flatjson::FJ_INVALID);
    };

    test += FJ_TEST(test for error_message formatting) {
        flatjson::error_info e{};
        std::memcpy(e.js_str, "hello", 5);
        e.js_str[5] = '\x01';
        e.js_str[6] = '\x80';
        e.js_str[7] = ' ';
        e.js_str[8] = '\x7f';
        e.js_str[9] = '\xff';
        e.js_pos = 3;
        e.in_dump_pos = 1;
        e.line = 10;
        e.func = "parse_doc";
        e.prev_func = "parse";

        const flatjson::error_code codes[] = {
             flatjson::FJ_OK
            ,flatjson::FJ_INVALID
            ,flatjson::FJ_INCOMPLETE
            ,flatjson::FJ_EXTRA_DATA
            ,flatjson::FJ_NO_FREE_TOKENS
            ,flatjson::FJ_CANT_ALLOCATE
            ,flatjson::FJ_SV_CROSS_CHUNK
            ,flatjson::FJ_NO_FREE_CHUNKS
        };
        for ( const auto code: codes ) {
            e.code = code;
            const std::string msg = assert_error_message(e);
            const std::string head = std::string{flatjson::error_string(code)}
                + ", in function \"parse_doc()\"(10), prev function \"parse()\""
                  ", at json position 3, content:\n";
            assert(msg.compare(0, head.size(), head) == 0);
            assert(msg.find(
                "x|68|65|6c|6c|6f|01|80|20|7f|ff|00|00|00|00|00|00|\n"
            ) != std::string::npos);
            assert(msg.compare(msg.size() - 7, 7, "     ^\n") == 0);
            const auto at = msg.find("\nc|");
            assert(at != std::string::npos);
            const auto col = at + 1;
            assert(msg[col + 2] == 'h');
            assert(msg[col + 3] == ' ');
            assert(msg[col + 4] == ' ');
            assert(msg[col + 2 + 3 * 6] == '.');
            assert(msg[col + 2 + 3 * 7] == ' ');
            assert(msg[col + 2 + 3 * 8] == '.');
            assert(msg[col + 2 + 3 * 9] == '.');
            assert(msg.compare(col + 2 + 3 * 15, 4, ". |\n") == 0);
        }

        e.code = flatjson::FJ_EXTRA_DATA;
        e.prev_func = nullptr;
        e.js_pos = 0;
        e.in_dump_pos = 0;
        std::memset(e.js_str, 0, sizeof(e.js_str));
        const std::string bare = assert_error_message(e);
        const char bare_head[] =
            "EXTRA_DATA, in function \"parse_doc()\"(10), at json position 0, content:\n";
        const char bare_hex[] =
            "x|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|\n";
        assert(bare.find("prev function") == std::string::npos);
        assert(bare.compare(0, sizeof(bare_head) - 1, bare_head) == 0);
        assert(bare.find(bare_hex) != std::string::npos);
        assert(bare.compare(bare.size() - 4, 4, "  ^\n") == 0);

        e.code = flatjson::FJ_INVALID;
        e.js_pos = 15;
        e.in_dump_pos = 15;
        e.js_str[15] = 'Z';
        const std::string edge = assert_error_message(e);
        const char edge_hex[] =
            "x|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|5a|\n";
        assert(edge.find(edge_hex) != std::string::npos);
        const std::string caret(47, ' ');
        assert(edge.compare(edge.size() - 49, 49, caret + "^\n") == 0);
        const auto edge_at = edge.find("\nc|");
        assert(edge_at != std::string::npos);
        assert(edge.compare(edge_at + 1 + 2 + 3 * 15, 4, "Z |\n") == 0);

        // A caret past the buffer used to write one byte past the 512-byte message.
        e.in_dump_pos = 1000;
        const std::string cut{flatjson::error_message(e)};
        assert(cut.size() == 511);
        assert(cut.compare(0, 9, "INVALID, ") == 0);

        std::string long_func(400, 'F');
        e.func = long_func.c_str();
        e.prev_func = "parse";
        e.in_dump_pos = 1;
        const std::string long_cut{flatjson::error_message(e)};
        assert(long_cut.size() == 511);
        assert(long_cut.find("FFFFFFFF") != std::string::npos);

        e.func = "parse_doc";
        e.prev_func = "parse";
        e.js_pos = 3;
        e.in_dump_pos = 1;
        std::memcpy(e.js_str, "hello", 5);
        e.js_str[5] = '\x01';
        e.js_str[6] = '\x80';
        e.js_str[7] = ' ';
        e.js_str[8] = '\x7f';
        e.js_str[9] = '\xff';
        e.code = flatjson::FJ_INVALID;
        const std::string again = assert_error_message(e);
        assert(again.compare(again.size() - 7, 7, "     ^\n") == 0);
    };

    test += FJ_TEST(test for pparse from a C string) {
        const char *ptr = "[1,[2,3]]";
        {
            auto json = flatjson::pparse(
                 flatjson::args.json_beg = ptr
                ,flatjson::args.json_end = ptr + std::strlen(ptr) + 1
            );
            assert(json.is_valid());
            assert(json.tokens() == 7);
        }
        {
            flatjson::parser p{};
            auto json = flatjson::pparse(
                 flatjson::args.parser = &p
                ,flatjson::args.json_beg = ptr
                ,flatjson::args.json_end = ptr + std::strlen(ptr) + 1
            );
            assert(json.is_valid());
            assert(json.tokens() == 7);
        }
        {
            flatjson::parser p{};
            flatjson::token toks[8];
            auto json = flatjson::pparse(
                 flatjson::args.parser = &p
                ,flatjson::args.json_beg = ptr
                ,flatjson::args.json_end = ptr + std::strlen(ptr) + 1
                ,flatjson::args.toks_beg = std::begin(toks)
                ,flatjson::args.toks_end = std::end(toks)
            );
            assert(json.is_valid());
            assert(json.tokens() == 7);
        }
    };

    test += FJ_TEST(test for compare) {
        auto load = [](const char *json) {
            auto *parser = flatjson::alloc_parser();
            flatjson::parse(
                 parser
                ,flatjson::args.json_beg = json
                ,flatjson::args.json_end = json + std::strlen(json) + 1
            );
            assert(flatjson::is_valid(parser));

            return parser;
        };
        struct row {
            const char *left;
            const char *right;
            flatjson::compare_result markup;
            flatjson::compare_result length;
            flatjson::compare_result full;
        };
        const row rows[] = {
             {
                  "[]"
                 ,"[]"
                 ,flatjson::compare_result::equal
                 ,flatjson::compare_result::equal
                 ,flatjson::compare_result::equal
             }
            ,{
                 "{}"
                ,"{}"
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal
            }
            ,{
                 "null"
                ,"null"
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal
            }
            ,{
                 "1"
                ,"\"1\""
                ,flatjson::compare_result::type
                ,flatjson::compare_result::type
                ,flatjson::compare_result::type
            }
            ,{
                 "true"
                ,"false"
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::length
                ,flatjson::compare_result::value
            }
            ,{"\"ab\"", "\"a\""
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::length
                ,flatjson::compare_result::value}
            ,{
                 "[1,2]"
                ,"[2,1]"
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::value
            }
            ,{"[1,2]", "[1,2,3]"
                ,flatjson::compare_result::longer
                ,flatjson::compare_result::longer
                ,flatjson::compare_result::longer}
            ,{"[1,2,3]", "[1,2]"
                ,flatjson::compare_result::shorter
                ,flatjson::compare_result::shorter
                ,flatjson::compare_result::shorter}
            ,{"[1,2]", "[1,{\"a\":2}]"
                ,flatjson::compare_result::type
                ,flatjson::compare_result::type
                ,flatjson::compare_result::type}
            ,{"{\"a\":1,\"b\":2}", "{\"b\":2,\"a\":1}"
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal}
            ,{"{\"b\":[1],\"a\":{\"z\":true}}", "{\"a\":{\"z\":true},\"b\":[1]}"
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal}
            ,{"{\"a\":1}", "{\"a\":1,\"b\":2}"
                ,flatjson::compare_result::longer
                ,flatjson::compare_result::longer
                ,flatjson::compare_result::longer}
            ,{"{\"a\":1,\"b\":2}", "{\"a\":1,\"c\":2}"
                ,flatjson::compare_result::no_key
                ,flatjson::compare_result::no_key
                ,flatjson::compare_result::no_key}
            ,{"{\"a\":\"hi\",\"b\":\"yo\"}", "{\"b\":\"y\",\"a\":\"hi\"}"
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::length
                ,flatjson::compare_result::value}
            ,{"{\"a\":[1,2]}", "{\"a\":[1,2,3]}"
                ,flatjson::compare_result::longer
                ,flatjson::compare_result::longer
                ,flatjson::compare_result::longer}
            ,{"[1,{\"a\":2},3]", "[1,{\"a\":3},3]"
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::equal
                ,flatjson::compare_result::value}
            ,{
                 "{\"a\":1}"
                ,"[1]"
                ,flatjson::compare_result::type
                ,flatjson::compare_result::type
                ,flatjson::compare_result::type
            }
        };
        const flatjson::compare_mode modes[] = {
             flatjson::compare_mode::markup_only
            ,flatjson::compare_mode::length_only
            ,flatjson::compare_mode::full
        };
        for ( const auto &item: rows ) {
            auto *left = load(item.left);
            auto *right = load(item.right);
            const flatjson::compare_result expected[] = {item.markup, item.length, item.full};
            for ( int i = 0; i < 3; ++i ) {
                flatjson::iterator left_diff{};
                flatjson::iterator right_diff{};
                const auto result = flatjson::compare(
                     &left_diff
                    ,&right_diff
                    ,left
                    ,right
                    ,modes[i]
                );
                assert(result == expected[i]);
                assert(flatjson::compare_result_string(result) != nullptr);
                if ( result != flatjson::compare_result::equal ) {
                    assert(left_diff.cur != nullptr);
                    assert(right_diff.cur != nullptr);
                }
            }
            flatjson::free_parser(left);
            flatjson::free_parser(right);
        }
        assert(flatjson::compare_result_string(flatjson::compare_result::key) != nullptr);

        auto *left = load("{\"a\":1,\"b\":2}");
        auto *right = load("{\"a\":1,\"c\":3}");
        flatjson::iterator left_diff{};
        flatjson::iterator right_diff{};
        assert(flatjson::compare(&left_diff, &right_diff, left, right, flatjson::compare_mode::full)
            == flatjson::compare_result::no_key);
        assert(left_diff.key() == "b");
        assert(right_diff.type() == flatjson::FJ_TYPE_OBJECT_END);
        flatjson::free_parser(left);
        flatjson::free_parser(right);

        left = load("{\"a\":[1,2]}");
        right = load("{\"a\":[1]}");
        assert(flatjson::compare(
             &left_diff
            ,&right_diff
            ,left
            ,right
            ,flatjson::compare_mode::markup_only
        ) == flatjson::compare_result::shorter);
        assert(left_diff.is_array());
        assert(left_diff.key() == "a");
        assert(right_diff.is_array());
        flatjson::free_parser(left);
        flatjson::free_parser(right);

        left = load("{\"a\":{\"x\":1}}");
        right = load("{\"a\":{\"x\":2}}");
        assert(flatjson::compare(&left_diff, &right_diff, left, right, flatjson::compare_mode::full)
            == flatjson::compare_result::value);
        assert(left_diff.key() == "x");
        assert(left_diff.to_int() == 1);
        assert(right_diff.to_int() == 2);
        flatjson::free_parser(left);
        flatjson::free_parser(right);
    };

    test += FJ_TEST(test for fjson with a user parser frees its tokens) {
        static const char str[] = R"({"a":[1,2,3],"b":{"c":null}})";
        flatjson::parser p{};
        {
            flatjson::fjson json{
                 flatjson::args.parser = &p
                ,flatjson::args.json_beg = std::begin(str)
                ,flatjson::args.json_end = std::end(str)
            };
            assert(json.is_valid());
            assert(p.ref_counter == 1u);
            auto copy = json.at("a");
            assert(copy.is_array());
            assert(copy.size() == 3);
            assert(p.ref_counter == 2u);
        }
        assert(p.ref_counter == 0u);
    };

    test += FJ_TEST(test for fjson views conversions and lifetime) {
        flatjson::fjson empty;
        assert(!empty.is_valid());
        assert(empty.begin() == empty.end());
        assert(empty.cbegin() == empty.cend());

        static const char str[] =
            R"({"b":true,"n":-12,"u":7,"f":1.5,"s":"hi","z":null,)"
            R"("a":[10,{"k":false}],"o":{"x":"y"},"e":[],"g":{}})";
        flatjson::fjson json{
             flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        };
        assert(json.is_valid());
        assert(json.error() == flatjson::FJ_OK);
        assert(json.is_object());
        assert(!json.is_array());
        assert(!json.is_null());
        assert(!json.is_bool());
        assert(!json.is_number());
        assert(!json.is_string());
        assert(!json.is_simple_type());
        assert(json.type() == flatjson::FJ_TYPE_OBJECT);
        assert(std::strcmp(json.type_name(), "OBJECT") == 0);
        assert(json.size() == 10);
        assert(json.members() == json.size());
        assert(!json.is_empty());
        assert(json.keys_num() == 10);

        const auto ks = json.keys();
        assert(ks.size() == 10);
        assert(ks[0] == "b");
        assert(ks[5] == "z");
        assert(ks[9] == "g");
        std::size_t seen = 0;
        auto cur = json.begin();
        ++cur;
        for ( ; cur != json.end(); ++cur, ++seen ) {
            assert(cur->key() == ks[seen]);
        }
        assert(seen == 10);

        auto it = json.cbegin();
        auto post = it++;
        assert(post == json.cbegin());
        assert(post != json.cend());
        assert(post->is_object());
        assert((*it).is_bool());
        assert(json.at(it).to_bool());
        assert(flatjson::distance(++json.begin(), json.end()) == 10);

        assert(json.contains("s"));
        assert(json.contains("s", 1));
        const char *skey = "s";
        assert(json.contains(skey));
        assert(!json.contains("missing"));
        assert(json.at("missing").begin() == json.at("missing").end());

        assert(json["b"].is_bool());
        assert(json["b"].is_simple_type());
        assert(json["b"].to_bool());
        assert(json["b"].type() == flatjson::FJ_TYPE_BOOL);
        assert(std::strcmp(json["b"].type_name(), "BOOL") == 0);
        assert(json["n"].is_number());
        assert(json["n"].to_int() == -12);
        assert(json["n"].to_int64() == -12);
        assert(json["n"].to<std::int32_t>() == -12);
        assert(json["u"].to_uint() == 7u);
        assert(json["u"].to_uint64() == 7u);
        assert(json["f"].to_double() == 1.5);
        assert(json["f"].to_float() == 1.5f);
        assert(json["f"].to_string_view() == "1.5");
        assert(json[skey].is_string());
        assert(json[skey].to_string() == "hi");
        assert(json[skey].to_string_view() == "hi");
        assert(json["z"].is_null());
        assert(json["z"].is_simple_type());
        assert(json["z"].to_string_view() == "null");

        auto arr = json["a"];
        assert(arr.is_array());
        assert(arr.type() == flatjson::FJ_TYPE_ARRAY);
        assert(std::strcmp(arr.type_name(), "ARRAY") == 0);
        assert(arr.size() == 2);
        assert(!arr.is_empty());
        assert(arr.keys_num() == 0);
        assert(arr.keys().empty());
        assert(arr[0].to_int() == 10);
        assert(arr[1].is_object());
        assert(!arr[1]["k"].to_bool());
        assert(arr.at(std::size_t{99}).begin() == arr.at(std::size_t{99}).end());
        assert(json.at("o")["x"].to_string_view() == "y");
        assert(json["e"].is_empty());
        assert(json["e"].members() == 0);
        assert(json["g"].is_object());
        assert(json["g"].is_empty());

        flatjson::fjson kept;
        {
            flatjson::fjson parent{
                 flatjson::args.json_beg = std::begin(str)
                ,flatjson::args.json_end = std::end(str)
            };
            kept = parent["n"];
        }
        assert(kept.is_valid());
        assert(kept.to_int() == -12);
        flatjson::fjson moved = std::move(kept);
        assert(!kept.is_valid());
        assert(kept.begin() == kept.end());
        assert(moved.to_int() == -12);
        flatjson::fjson narrowed{
             flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        };
        narrowed = narrowed["n"];
        assert(narrowed.is_valid());
        assert(narrowed.to_int() == -12);

        const auto before = myallocator.allocations();
        {
            flatjson::fjson a{
                 flatjson::args.json_beg = std::begin(str)
                ,flatjson::args.json_end = std::end(str)
                ,flatjson::args.mem_fns = {&my_realloc, &my_free}
            };
            flatjson::fjson b = a;
            flatjson::fjson c;
            c = b;
            flatjson::fjson d = std::move(c);
            assert(!c.is_valid());
            assert(c.begin() == c.end());
            a = std::move(d);
            assert(!d.is_valid());
            assert(d.begin() == d.end());
            assert(a["s"].to_string_view() == "hi");
            flatjson::fjson *self = &a;
            a = *self;
            b = a;
        }
        assert(myallocator.allocations() == before);

        static const char pair[] = R"([1,2])";
        flatjson::token toks[8];
        {
            flatjson::fjson owned{
                 flatjson::args.json_beg = std::begin(pair)
                ,flatjson::args.json_end = std::end(pair)
                ,flatjson::args.toks_beg = std::begin(toks)
                ,flatjson::args.toks_end = std::end(toks)
            };
            assert(owned.is_valid());
            assert(owned.tokens() == 4);
            assert(owned[1].to_int() == 2);
        }
        flatjson::parser stack{};
        flatjson::token stack_toks[8];
        {
            flatjson::fjson view{
                 flatjson::args.parser = &stack
                ,flatjson::args.toks_beg = std::begin(stack_toks)
                ,flatjson::args.toks_end = std::end(stack_toks)
                ,flatjson::args.mem_fns = {&my_realloc, &my_free}
                ,flatjson::args.json_beg = std::begin(pair)
                ,flatjson::args.json_end = std::end(pair)
            };
            assert(view.is_valid());
            assert(view[0].to_int() == 1);
        }
        assert(stack.ref_counter == 0u);

        flatjson::parser shared{};
        flatjson::fjson alias;
        {
            flatjson::fjson owner{
                 flatjson::args.parser = &shared
                ,flatjson::args.json_beg = std::begin(str)
                ,flatjson::args.json_end = std::end(str)
            };
            assert(shared.ref_counter == 1u);
            alias = flatjson::fjson{&shared};
            assert(shared.ref_counter == 2u);
            flatjson::fjson again{
                 flatjson::args.parser = &shared
                ,flatjson::args.json_beg = std::begin(pair)
                ,flatjson::args.json_end = std::end(pair)
            };
            assert(shared.ref_counter == 3u);
            assert(owner["n"].to_int() == -12);
            assert(again["n"].to_int() == -12);
            flatjson::token extra_toks[4];
            flatjson::fjson with_toks{
                 flatjson::args.parser = &shared
                ,flatjson::args.toks_beg = std::begin(extra_toks)
                ,flatjson::args.toks_end = std::end(extra_toks)
                ,flatjson::args.json_beg = std::begin(pair)
                ,flatjson::args.json_end = std::end(pair)
            };
            assert(shared.ref_counter == 4u);
            assert(with_toks["n"].to_int() == -12);
        }
        assert(shared.ref_counter == 1u);
        assert(alias.is_valid());
        assert(alias["n"].to_int() == -12);
        alias = flatjson::fjson{};
        assert(shared.ref_counter == 0u);
        {
            flatjson::fjson reused{
                 flatjson::args.parser = &shared
                ,flatjson::args.json_beg = std::begin(pair)
                ,flatjson::args.json_end = std::end(pair)
            };
            assert(reused.is_valid());
            assert(reused.is_array());
            assert(reused[0].to_int() == 1);
            assert(shared.ref_counter == 1u);
        }
        assert(shared.ref_counter == 0u);

        auto *held = flatjson::alloc_parser();
        assert(flatjson::parse(
             held
            ,flatjson::args.json_beg = std::begin(pair)
            ,flatjson::args.json_end = std::end(pair)
        ) == 4);
        {
            flatjson::fjson view{held};
            assert(view.is_valid());
            assert(view.tokens() == 4);
            assert(view.is_array());
        }
        assert(flatjson::is_valid(held));
        flatjson::free_parser(held);

        static const char cut[] = "[";
        flatjson::fjson broken{
             flatjson::args.json_beg = std::begin(cut)
            ,flatjson::args.json_end = std::end(cut)
        };
        assert(!broken.is_valid());
        assert(broken.error() == flatjson::FJ_INCOMPLETE);
        assert(std::strstr(broken.error_string(), "INCOMPLETE") != nullptr);

        static const char commented[] = "//h\n[NaN]";
        flatjson::fjson nan{
             flatjson::args.json_beg = std::begin(commented)
            ,flatjson::args.json_end = std::end(commented)
            ,flatjson::args.flags = flatjson::FJ_PARSE_ALLOW_COMMENTS
                | flatjson::FJ_PARSE_ALLOW_INF_NAN
        };
        assert(nan.is_valid());
        assert(nan[0].to_string_view() == "NaN");
        flatjson::fjson rejected{
             flatjson::args.json_beg = std::begin(commented)
            ,flatjson::args.json_end = std::end(commented)
        };
        assert(!rejected.is_valid());

        static const char compact[] = R"({"a":1})";
        static const char spaced[] = R"({"a": 1})";
        flatjson::fjson tight{
             flatjson::args.json_beg = std::begin(compact)
            ,flatjson::args.json_end = std::end(compact)
            ,flatjson::args.despaced = true
        };
        assert(tight.is_valid());
        assert(tight["a"].to_int() == 1);
        flatjson::fjson loose{
             flatjson::args.json_beg = std::begin(spaced)
            ,flatjson::args.json_end = std::end(spaced)
            ,flatjson::args.despaced = true
        };
        assert(!loose.is_valid());

        flatjson::parser user{};
        flatjson::fjson failed_alloc{
             flatjson::args.parser = &user
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
            ,flatjson::args.mem_fns = {&fail_alloc, &fail_alloc_free}
        };
        assert(!failed_alloc.is_valid());
        assert(failed_alloc.error() == flatjson::FJ_CANT_ALLOCATE);
        assert(std::strstr(failed_alloc.error_string(), "CANT_ALLOCATE") != nullptr);
    };

    test += FJ_TEST(test for is_empty on a parser) {
        static const char str[] = R"([1])";
        auto *parser = flatjson::alloc_parser();
        assert(parser);
        assert(flatjson::is_empty(parser));

        assert(flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        ) == 3);
        assert(!flatjson::is_empty(parser));
        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for error_string names every error_code) {
        assert(std::strcmp(flatjson::error_string(flatjson::FJ_OK), "OK") == 0);
        assert(std::strcmp(flatjson::error_string(flatjson::FJ_INVALID), "INVALID") == 0);
        assert(std::strcmp(flatjson::error_string(flatjson::FJ_INCOMPLETE), "INCOMPLETE") == 0);
        assert(std::strcmp(flatjson::error_string(flatjson::FJ_EXTRA_DATA), "EXTRA_DATA") == 0);
        assert(std::strcmp(
             flatjson::error_string(flatjson::FJ_NO_FREE_TOKENS)
            ,"NO_FREE_TOKENS"
        ) == 0);
        assert(std::strcmp(
             flatjson::error_string(flatjson::FJ_CANT_ALLOCATE)
            ,"CANT_ALLOCATE"
        ) == 0);
        assert(std::strcmp(
             flatjson::error_string(flatjson::FJ_SV_CROSS_CHUNK)
            ,"SV_CROSS_CHUNK"
        ) == 0);
        assert(std::strcmp(
             flatjson::error_string(flatjson::FJ_NO_FREE_CHUNKS)
            ,"NO_FREE_CHUNKS"
        ) == 0);
    };

    test += FJ_TEST(test for strict handling of jsonchecker accept cases) {
        auto check_error_code = [](const char *json, flatjson::error_code expected_code) {
            auto *held = flatjson::alloc_parser();
            assert(held);
            flatjson::error_code ec = flatjson::FJ_OK;
            auto toknum = flatjson::parse(
                 held
                ,flatjson::args.json_beg = json
                ,flatjson::args.json_end = json + std::strlen(json) + 1
                ,flatjson::args.error_code = &ec
            );
            if ( expected_code == flatjson::FJ_OK ) {
                assert(toknum > 0);
            } else {
                assert(toknum == 0);
            }
            assert(ec == expected_code);
            flatjson::free_parser(held);
        };

        check_error_code(
             "{\"Extra comma\": true,}"
            ,flatjson::FJ_INVALID
        );          // fail09, trailing comma
        check_error_code(
             "{\"Numbers cannot have leading zeroes\": 013}"
            ,flatjson::FJ_INVALID
        ); // fail13
        check_error_code("[\"Illegal backslash escape: \\x15\"]", flatjson::FJ_INVALID); // fail15
        check_error_code("[\"Illegal backslash escape: \\017\"]", flatjson::FJ_INVALID); // fail17
        check_error_code("[\"\ttab\tcharacter\tin\tstring\t\"]", flatjson::FJ_INVALID); // fail25
        check_error_code(
             "[\"tab\\   character\\   in\\  string\\  \"]"
            ,flatjson::FJ_INVALID
        ); // fail26
        check_error_code("[\"line\nbreak\"]", flatjson::FJ_INVALID);                 // fail27

        std::string fail28 = "[\"line\\";
        fail28.push_back('\n');
        fail28 += "break\"]";
        check_error_code(fail28.c_str(), flatjson::FJ_INCOMPLETE);                   // fail28

        check_error_code(
             "[0e]"
            ,flatjson::FJ_INVALID
        );                              // fail29 (full-buffer)
        check_error_code(
             "[0e+]"
            ,flatjson::FJ_INVALID
        );                             // fail30 (full-buffer)
        check_error_code("[0e+-1]", flatjson::FJ_INVALID);                           // fail31
    };

    test += FJ_TEST(test for chunked parse numbers) {
        run_chunked_case("[0e", "1]", chunked_first_phase::non_ok, [](flatjson::parser *p) {
            auto it0 = flatjson::at(p, static_cast<std::size_t>(0));
            assert(it0.is_number());
            assert(it0.to_double() == 0.0);
        });
        run_chunked_case("[0e+", "1]", chunked_first_phase::non_ok, [](flatjson::parser *p) {
            auto it0 = flatjson::at(p, static_cast<std::size_t>(0));
            assert(it0.is_number());
            assert(it0.to_double() == 0.0);
        });
    };

    test += FJ_TEST(test for chunked parse string) {
        run_chunked_case("{\"i\":42,\"s\":\"hel", "lo\"}", chunked_first_phase::sv_cross_chunk
            ,[](flatjson::parser *p) {
                auto i = flatjson::at(p, "i");
                assert(i.is_number());
                assert(i.to_int() == 42);
                auto s = flatjson::at(p, "s");
                assert(s.is_string());
                assert(s.to_string_view() == "hello");
            });
    };

    test += FJ_TEST(test for chunked parse literals) {
        run_chunked_case("{\"flag\":tr", "ue,\"val\":7}", chunked_first_phase::non_ok
            ,[](flatjson::parser *p) {
                auto flag = flatjson::at(p, "flag");
                assert(flag.is_bool());
                assert(flag.to_bool() == true);
                auto val = flatjson::at(p, "val");
                assert(val.is_number());
                assert(val.to_int() == 7);
            });
        run_chunked_case("{\"flag\":fal", "se,\"val\":8}", chunked_first_phase::non_ok
            ,[](flatjson::parser *p) {
                auto flag = flatjson::at(p, "flag");
                assert(flag.is_bool());
                assert(flag.to_bool() == false);
                auto val = flatjson::at(p, "val");
                assert(val.is_number());
                assert(val.to_int() == 8);
            });
        run_chunked_case("{\"opt\":nu", "ll,\"val\":9}", chunked_first_phase::non_ok
            ,[](flatjson::parser *p) {
                auto opt = flatjson::at(p, "opt");
                assert(opt.is_null());
                auto val = flatjson::at(p, "val");
                assert(val.is_number());
                assert(val.to_int() == 9);
            });
    };

    test += FJ_TEST(test for chunked parse edge cases) {
        // split inside unicode escape
        run_chunked_case("{\"s\":\"\\u12", "34\"}", chunked_first_phase::sv_cross_chunk
            ,[](flatjson::parser *p) {
                auto s = flatjson::at(p, "s");
                assert(s.is_string());
                assert(s.to_string_view() == "\\u1234");
            });

        // split right after backslash
        run_chunked_case("{\"s\":\"abc\\", "n\"}", chunked_first_phase::sv_cross_chunk
            ,[](flatjson::parser *p) {
                auto s = flatjson::at(p, "s");
                assert(s.is_string());
                assert(s.to_string_view() == "abc\\n");
            });

        // split inside nested object value
        run_chunked_case("{\"n\":{\"k\":\"va", "l\"},\"x\":1}", chunked_first_phase::sv_cross_chunk
            ,[](flatjson::parser *p) {
                auto n = flatjson::at(p, "n");
                assert(n.is_object());
                auto k = flatjson::at(n, "k");
                assert(k.is_string());
                assert(k.to_string_view() == "val");
                auto x = flatjson::at(p, "x");
                assert(x.is_number());
                assert(x.to_int() == 1);
            });

        // split between array elements
        run_chunked_case("[{\"a\":1},", "{\"b\":2}]", chunked_first_phase::incomplete
            ,[](flatjson::parser *p) {
                auto it0 = flatjson::at(p, static_cast<std::size_t>(0));
                assert(it0.is_object());
                auto a = flatjson::at(it0, "a");
                assert(a.is_number());
                assert(a.to_int() == 1);
                auto it1 = flatjson::at(p, static_cast<std::size_t>(1));
                assert(it1.is_object());
                auto b = flatjson::at(it1, "b");
                assert(b.is_number());
                assert(b.to_int() == 2);
            });

        // split in nested array closing sequence
        run_chunked_case("{\"a\":[1,2", ",3]}", chunked_first_phase::sv_cross_chunk
            ,[](flatjson::parser *p) {
                auto a = flatjson::at(p, "a");
                assert(a.is_array());
                auto i0 = flatjson::at(a, static_cast<std::size_t>(0));
                auto i1 = flatjson::at(a, static_cast<std::size_t>(1));
                auto i2 = flatjson::at(a, static_cast<std::size_t>(2));
                assert(i0.to_int() == 1);
                assert(i1.to_int() == 2);
                assert(i2.to_int() == 3);
            });

        // split in key token
        run_chunked_case("{\"lo", "ng\":10}", chunked_first_phase::sv_cross_chunk
            ,[](flatjson::parser *p) {
                auto v = flatjson::at(p, "long");
                assert(v.is_number());
                assert(v.to_int() == 10);
            });
    };

    test += FJ_TEST(test for jsonrefl-like two-buffer continuation) {
        static const char chunk0[] = "{\"i\":10,\"n\":{\"i\":20,\"s\":\"inner\"},\"sv\":\"vi";
        static const char chunk1[] = "ew\"}";

        char buf0[128]{};
        std::memcpy(buf0, chunk0, sizeof(chunk0));

        flatjson::token toks[64]{};
        flatjson::chunk chunks[4];
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 64
            ,flatjson::args.chunks_beg = chunks
            ,flatjson::args.chunks_end = chunks + 4
        );

        // The string is cut by the end of the chunk: that is known at once.
        auto cur = flatjson::feed(&parser, buf0, std::strlen(buf0));
        assert(cur.code == flatjson::FJ_SV_CROSS_CHUNK);

        char buf1[64]{};
        std::memcpy(buf1, chunk1, sizeof(chunk1));
        cur = flatjson::feed(&parser, buf1, std::strlen(buf1));
        assert(cur.code == flatjson::FJ_SV_CROSS_CHUNK);

        // Chunks that keep every leaf whole, each in a buffer of its own.
        flatjson::reset(&parser);
        const std::string parts[] = {
             "{\"i\":10,"
            ,"\"n\":{\"i\":20,"
            ,"\"s\":\"inner\"},"
            ,"\"sv\":\"view\"}"
        };
        for ( const auto &part: parts ) {
            cur = flatjson::feed(&parser, part.data(), part.size());
            assert(cur.next == part.data() + part.size());
            assert(cur.remaining == 0);
        }
        assert(cur.code == flatjson::FJ_OK);
        assert(parser.chunks.num == 4u);
        assert(flatjson::at(&parser, "i").to_int() == 10);
        assert(flatjson::at(flatjson::at(&parser, "n"), "i").to_int() == 20);
        assert(flatjson::at(flatjson::at(&parser, "n"), "s").to_string_view() == "inner");
        auto sv = flatjson::at(&parser, "sv");
        assert(sv.to_string_view() == "view");
        assert(sv.to_string_view().data() == parts[3].data() + 6);
        assert(flatjson::begin(&parser).key().empty());
    };

    test += FJ_TEST(test for streaming chunks in separate buffers) {
        flatjson::token toks[32]{};
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 32
            ,flatjson::args.mem_fns = {&my_realloc, &my_free}
        );

        const std::string parts[] = {"[1,", "2,", " 3", "]"};
        auto c0 = flatjson::feed(&parser, parts[0].data(), parts[0].size());
        assert(c0.code == flatjson::FJ_INCOMPLETE);
        assert(c0.next == parts[0].data() + 3);
        assert(c0.remaining == 0);
        auto c1 = flatjson::feed(&parser, parts[1].data(), parts[1].size());
        assert(c1.code == flatjson::FJ_INCOMPLETE);
        // A number needs the byte after it in its own chunk.
        auto c2 = flatjson::feed(&parser, parts[2].data(), parts[2].size());
        assert(c2.code == flatjson::FJ_SV_CROSS_CHUNK);
        assert(c2.next == parts[2].data() + 1);
        assert(c2.remaining == 1);
        auto c3 = flatjson::feed(&parser, parts[3].data(), parts[3].size());
        assert(c3.code == flatjson::FJ_SV_CROSS_CHUNK);

        flatjson::reset(&parser);
        assert(parser.chunks.num == 0u);
        const std::string fine[] = {"[1,", "2,", " 3]", " \n"};
        for ( const auto &part: fine ) {
            c0 = flatjson::feed(&parser, part.data(), part.size());
        }
        assert(c0.code == flatjson::FJ_OK);
        assert(flatjson::at(&parser, static_cast<std::size_t>(0)).to_int() == 1);
        assert(flatjson::at(&parser, static_cast<std::size_t>(1)).to_int() == 2);
        assert(flatjson::at(&parser, static_cast<std::size_t>(2)).to_int() == 3);
        assert(flatjson::at(&parser, static_cast<std::size_t>(2)).to_string_view().data()
            == fine[2].data() + 1);
        // The chunk table grew from the parser's memory functions.
        assert(myallocator.allocations() == 1);
        assert(myallocator.reallocations() == 0);
        c0 = flatjson::feed(&parser, nullptr, 0);
        assert(c0.code == flatjson::FJ_OK);
        // No chunk after the end of the stream.
        c0 = flatjson::feed(&parser, fine[3].data(), fine[3].size());
        assert(c0.code == flatjson::FJ_INVALID);

        // The table is reallocated past its first eight chunks.
        flatjson::reset(&parser);
        std::vector<std::string> many{"["};
        for ( int i = 0; i < 20; ++i ) {
            many.push_back(std::to_string(i) + ",");
        }
        many.push_back("20]");
        for ( const auto &part: many ) {
            c0 = flatjson::feed(&parser, part.data(), part.size());
        }
        assert(c0.code == flatjson::FJ_OK);
        assert(parser.chunks.num == many.size());
        assert(myallocator.reallocations() > 0);
        for ( std::size_t i = 0; i <= 20; ++i ) {
            auto it = flatjson::at(&parser, i);
            assert(it.to_uint() == i);
            assert(token_text(&parser, it.cur) == it.to_string_view());
        }
        // The root spans every chunk: there is no single slice for it.
        assert(token_text(&parser, parser.toks_beg).empty());

        flatjson::free_parser(&parser);
        assert(myallocator.allocations() == 0);
    };

    test += FJ_TEST(test for streaming number at the chunk end) {
        auto feed_two = [](const char *first, const char *second) -> flatjson::error_code {
            flatjson::token toks[8]{};
            flatjson::chunk chunks[2];
            auto parser = flatjson::make_parser(
                 flatjson::args.toks_beg = toks
                ,flatjson::args.toks_end = toks + 8
                ,flatjson::args.chunks_beg = chunks
                ,flatjson::args.chunks_end = chunks + 2
            );
            const std::string a{first};
            const std::string b{second};
            flatjson::feed(&parser, a.data(), a.size());

            return flatjson::feed(&parser, b.data(), b.size()).code;
        };

        assert(feed_two("[12", "3]") == flatjson::FJ_SV_CROSS_CHUNK);
        assert(feed_two("[1.", "5]") == flatjson::FJ_SV_CROSS_CHUNK);
        assert(feed_two("[1e", "5]") == flatjson::FJ_SV_CROSS_CHUNK);
        assert(feed_two("[-", "1]") == flatjson::FJ_SV_CROSS_CHUNK);
        assert(feed_two("{\"a\":4", "2}") == flatjson::FJ_SV_CROSS_CHUNK);
        assert(feed_two("[12,", "3]") == flatjson::FJ_OK);
        assert(feed_two("[12]", " ") == flatjson::FJ_OK);
        assert(feed_two("{\"a\":4}", "") == flatjson::FJ_OK);

        // A top-level number has no byte after it: the next chunk or the end of the stream
        // tells whether it is complete.
        auto top = [](const char *first, const char *second, flatjson::error_code expected) {
            flatjson::token toks[4]{};
            flatjson::chunk chunks[2];
            auto parser = flatjson::make_parser(
                 flatjson::args.toks_beg = toks
                ,flatjson::args.toks_end = toks + 4
                ,flatjson::args.chunks_beg = chunks
                ,flatjson::args.chunks_end = chunks + 2
            );
            const std::string a{first};
            auto c0 = flatjson::feed(&parser, a.data(), a.size());
            assert(c0.code == flatjson::FJ_INCOMPLETE);
            flatjson::cursor c1{};
            if ( second ) {
                const std::string b{second};
                c1 = flatjson::feed(&parser, b.data(), b.size());
                if ( c1.code == flatjson::FJ_OK ) {
                    c1 = flatjson::feed(&parser, nullptr, 0);
                }
            } else {
                c1 = flatjson::feed(&parser, nullptr, 0);
            }
            assert(c1.code == expected);
            if ( expected == flatjson::FJ_OK ) {
                assert(flatjson::begin(&parser).is_number());
                assert(flatjson::begin(&parser).to_string_view() == a.c_str());
            }
        };
        top("42", nullptr, flatjson::FJ_OK);
        top("42", " ", flatjson::FJ_OK);
        top("1.5e3", "\n", flatjson::FJ_OK);
        top("42", "3", flatjson::FJ_SV_CROSS_CHUNK);
        top("4", ".5", flatjson::FJ_SV_CROSS_CHUNK);
        top("4", "e5", flatjson::FJ_SV_CROSS_CHUNK);
        top("42", " x", flatjson::FJ_INVALID);

        // The classic API sees the whole document, so a trailing number is complete.
        static const char whole[] = "42";
        auto *wp = flatjson::alloc_parser();
        assert(flatjson::parse(
             wp
            ,flatjson::args.json_beg = std::begin(whole)
            ,flatjson::args.json_end = std::end(whole)
        ) == 1);
        assert(flatjson::is_valid(wp));
        flatjson::free_parser(wp);
    };

    test += FJ_TEST(test for chunk table limits) {
        const std::string parts[] = {"[1,", "2,", "3]"};

        // Without a table and memory functions the document is one chunk.
        flatjson::token toks[8]{};
        auto one = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 8
        );
        assert(flatjson::feed(
             &one
            ,parts[0].data()
            ,parts[0].size()
        ).code == flatjson::FJ_INCOMPLETE);
        auto c = flatjson::feed(&one, parts[1].data(), parts[1].size());
        assert(c.code == flatjson::FJ_NO_FREE_CHUNKS);
        assert(c.next == parts[1].data());
        assert(c.remaining == parts[1].size());
        assert(flatjson::get_error(&one) == flatjson::FJ_NO_FREE_CHUNKS);

        // A user table does not grow.
        flatjson::chunk chunks[2];
        auto two = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 8
            ,flatjson::args.chunks_beg = chunks
            ,flatjson::args.chunks_end = chunks + 2
        );
        assert(flatjson::feed(
             &two
            ,parts[0].data()
            ,parts[0].size()
        ).code == flatjson::FJ_INCOMPLETE);
        assert(flatjson::feed(
             &two
            ,parts[1].data()
            ,parts[1].size()
        ).code == flatjson::FJ_INCOMPLETE);
        assert(flatjson::feed(
             &two
            ,parts[2].data()
            ,parts[2].size()
        ).code == flatjson::FJ_NO_FREE_CHUNKS);

        // A streamed document packs into one buffer with the same token positions.
        flatjson::chunk chunks3[3];
        auto three = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 8
            ,flatjson::args.chunks_beg = chunks3
            ,flatjson::args.chunks_end = chunks3 + 3
        );
        for ( const auto &part: parts ) {
            c = flatjson::feed(&three, part.data(), part.size());
        }
        assert(c.code == flatjson::FJ_OK);
        std::vector<char> packed(flatjson::packed_state_size(&three));
        assert(flatjson::pack_state(packed.data(), packed.size(), &three) == packed.size());
        flatjson::token toks2[8]{};
        auto unpacked = flatjson::make_parser(
             flatjson::args.toks_beg = toks2
            ,flatjson::args.toks_end = toks2 + 8
        );
        assert(flatjson::unpack_state(&unpacked, packed.data(), packed.size()));
        for ( std::size_t i = 0; i < 3; ++i ) {
            assert(flatjson::at(&unpacked, i).to_uint() == i + 1);
        }
        assert(token_text(&unpacked, unpacked.toks_beg) == "[1,2,3]");
    };

    test += FJ_TEST(test for comments NaN and rejected trailing commas) {
        auto parse_flags_of = [](const char *json, std::uint8_t flags) -> flatjson::error_code {
            flatjson::token toks[32]{};
            auto parser = flatjson::make_parser(
                 flatjson::args.toks_beg = toks
                ,flatjson::args.toks_end = toks + 32
            );
            flatjson::error_code ec = flatjson::FJ_OK;
            flatjson::parse(
                 &parser
                ,flatjson::args.json_beg = json
                ,flatjson::args.json_end = json + std::strlen(json) + 1
                ,flatjson::args.error_code = &ec
                ,flatjson::args.flags = flags
            );

            return ec;
        };

        assert(parse_flags_of("{\"a\":1,}", flatjson::FJ_PARSE_NONE) == flatjson::FJ_INVALID);
        assert(parse_flags_of("{\"a\":1, }", flatjson::FJ_PARSE_NONE) == flatjson::FJ_INVALID);
        assert(parse_flags_of(
             "{\"a\":{\"b\":2,},\"c\":3}"
            ,flatjson::FJ_PARSE_NONE
        ) == flatjson::FJ_INVALID);
        assert(parse_flags_of(
             "{\"a\":1,//c\n}"
            ,flatjson::FJ_PARSE_ALLOW_COMMENTS
        ) == flatjson::FJ_INVALID);
        assert(parse_flags_of("{\"a\":1,\"b\":2}", flatjson::FJ_PARSE_NONE) == flatjson::FJ_OK);
        assert(parse_flags_of("{\"a\":1,,}", flatjson::FJ_PARSE_NONE) == flatjson::FJ_INVALID);
        assert(parse_flags_of("[1,]", flatjson::FJ_PARSE_NONE) == flatjson::FJ_INVALID);
        assert(parse_flags_of("[1,2,]", flatjson::FJ_PARSE_NONE) == flatjson::FJ_INVALID);

        assert(parse_flags_of("{\"a\":1}//c\n", flatjson::FJ_PARSE_NONE) == flatjson::FJ_INVALID);
        assert(parse_flags_of(
             "{\"a\":1}//c\n"
            ,flatjson::FJ_PARSE_ALLOW_COMMENTS
        ) == flatjson::FJ_OK);
        assert(parse_flags_of("[1,//c\n2]", flatjson::FJ_PARSE_ALLOW_COMMENTS) == flatjson::FJ_OK);

        assert(parse_flags_of("NaN", flatjson::FJ_PARSE_NONE) == flatjson::FJ_INVALID);
        assert(parse_flags_of("NaN", flatjson::FJ_PARSE_ALLOW_INF_NAN) == flatjson::FJ_OK);
        assert(parse_flags_of("Infinity", flatjson::FJ_PARSE_ALLOW_INF_NAN) == flatjson::FJ_OK);
        assert(parse_flags_of("-Infinity", flatjson::FJ_PARSE_ALLOW_INF_NAN) == flatjson::FJ_OK);

        // Both bits: a comment and a non-JSON number in one document. Either flag alone rejects it.
        static const char mixed[] = "// head\n[NaN,// mid\nInfinity,-Infinity]";
        assert(parse_flags_of(mixed, flatjson::FJ_PARSE_NONE) == flatjson::FJ_INVALID);
        assert(parse_flags_of(mixed, flatjson::FJ_PARSE_ALLOW_COMMENTS) == flatjson::FJ_INVALID);
        assert(parse_flags_of(mixed, flatjson::FJ_PARSE_ALLOW_INF_NAN) == flatjson::FJ_INVALID);
        assert(parse_flags_of(
             mixed
            ,flatjson::FJ_PARSE_ALLOW_COMMENTS | flatjson::FJ_PARSE_ALLOW_INF_NAN
        ) == flatjson::FJ_OK);
        flatjson::token xtoks[8]{};
        auto xparser = flatjson::make_parser(
             flatjson::args.toks_beg = xtoks
            ,flatjson::args.toks_end = xtoks + 8
        );
        flatjson::error_code xec = flatjson::FJ_OK;
        auto xn = flatjson::parse(
             &xparser
            ,flatjson::args.json_beg = std::begin(mixed)
            ,flatjson::args.json_end = std::end(mixed)
            ,flatjson::args.error_code = &xec
            ,flatjson::args.flags = flatjson::FJ_PARSE_ALLOW_COMMENTS
                | flatjson::FJ_PARSE_ALLOW_INF_NAN
        );
        assert(xec == flatjson::FJ_OK);
        assert(xn == 5);
        assert(flatjson::at(&xparser, static_cast<std::size_t>(0)).to_string_view() == "NaN");
        assert(flatjson::at(&xparser, static_cast<std::size_t>(1)).to_string_view() == "Infinity");
        assert(flatjson::at(&xparser, static_cast<std::size_t>(2)).to_string_view() == "-Infinity");

        static const char nan_json[] = "[NaN,Infinity,-Infinity]";
        flatjson::token toks[16]{};
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 16
        );
        flatjson::error_code ec = flatjson::FJ_OK;
        auto n = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(nan_json)
            ,flatjson::args.json_end = std::end(nan_json)
            ,flatjson::args.error_code = &ec
            ,flatjson::args.flags = flatjson::FJ_PARSE_ALLOW_INF_NAN
        );
        assert(ec == flatjson::FJ_OK);
        assert(n > 0);
        assert(flatjson::at(&parser, static_cast<std::size_t>(0)).to_string_view() == "NaN");
        assert(flatjson::at(&parser, static_cast<std::size_t>(1)).to_string_view() == "Infinity");
        assert(flatjson::at(&parser, static_cast<std::size_t>(2)).to_string_view() == "-Infinity");

        // A comment may be cut by a chunk; a NaN may not.
        char buf[32]{};
        flatjson::token ctoks[16]{};
        flatjson::chunk cchunks[2];
        auto streaming = flatjson::make_parser(
             flatjson::args.toks_beg = ctoks
            ,flatjson::args.toks_end = ctoks + 16
            ,flatjson::args.chunks_beg = cchunks
            ,flatjson::args.chunks_end = cchunks + 2
        );
        streaming.resume.flags = flatjson::FJ_PARSE_ALLOW_COMMENTS
            | flatjson::FJ_PARSE_ALLOW_INF_NAN;
        std::memcpy(buf, "{\"a\":1,/", 8);
        auto c0 = flatjson::feed(&streaming, buf, 8);
        assert(c0.code == flatjson::FJ_INCOMPLETE);
        std::memcpy(buf + 8, "/x\n\"b\":2}", 9);
        auto c1 = flatjson::feed(&streaming, buf + 8, 9);
        assert(c1.code == flatjson::FJ_OK);
        assert(flatjson::at(&streaming, "a").to_int() == 1);
        assert(flatjson::at(&streaming, "b").to_int() == 2);

        char nbuf[8]{};
        flatjson::token ntoks[4]{};
        flatjson::chunk nchunks[2];
        auto np = flatjson::make_parser(
             flatjson::args.toks_beg = ntoks
            ,flatjson::args.toks_end = ntoks + 4
            ,flatjson::args.chunks_beg = nchunks
            ,flatjson::args.chunks_end = nchunks + 2
        );
        np.resume.flags = flatjson::FJ_PARSE_ALLOW_INF_NAN;
        std::memcpy(nbuf, "Na", 2);
        auto n0 = flatjson::feed(&np, nbuf, 2);
        assert(n0.code == flatjson::FJ_SV_CROSS_CHUNK);
        std::memcpy(nbuf + 2, "N", 1);
        auto n1 = flatjson::feed(&np, nbuf + 2, 1);
        assert(n1.code == flatjson::FJ_SV_CROSS_CHUNK);
        flatjson::reset(&np);
        n1 = flatjson::feed(&np, nbuf, 3);
        assert(n1.code == flatjson::FJ_OK);
        assert(flatjson::begin(&np).to_string_view() == "NaN");

        // The comment is cut by the chunk, and the value after it is NaN.
        char mbuf[32]{};
        flatjson::token mtoks[8]{};
        flatjson::chunk mchunks[2];
        auto mixed_stream = flatjson::make_parser(
             flatjson::args.toks_beg = mtoks
            ,flatjson::args.toks_end = mtoks + 8
            ,flatjson::args.chunks_beg = mchunks
            ,flatjson::args.chunks_end = mchunks + 2
        );
        mixed_stream.resume.flags = flatjson::FJ_PARSE_ALLOW_COMMENTS
            | flatjson::FJ_PARSE_ALLOW_INF_NAN;
        std::memcpy(mbuf, "{\"a\":/", 6);
        auto m0 = flatjson::feed(&mixed_stream, mbuf, 6);
        assert(m0.code == flatjson::FJ_INCOMPLETE);
        std::memcpy(mbuf + 6, "/c\nNaN}", 7);
        auto m1 = flatjson::feed(&mixed_stream, mbuf + 6, 7);
        assert(m1.code == flatjson::FJ_OK);
        assert(flatjson::at(&mixed_stream, "a").to_string_view() == "NaN");
    };

    test += FJ_TEST(test for parser reuse after failed parse) {
        char json[128]{};
        std::memcpy(json, "{\"a\":]", sizeof("{\"a\":]"));
        const std::size_t bad_len = std::strlen(json);

        flatjson::token toks[32]{};
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 32
        );

        flatjson::error_code ec = flatjson::FJ_OK;
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = json
            ,flatjson::args.json_end = json + bad_len + 1
            ,flatjson::args.error_code = &ec
        );
        assert(toknum == 0);
        assert(ec == flatjson::FJ_INVALID);

        std::memcpy(json, "{\"a\":1}", sizeof("{\"a\":1}"));
        const std::size_t good_len = std::strlen(json);

        ec = flatjson::FJ_OK;
        toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = json
            ,flatjson::args.json_end = json + good_len + 1
            ,flatjson::args.error_code = &ec
        );
        assert(ec == flatjson::FJ_OK);
        assert(toknum > 0);
        assert(flatjson::is_valid(&parser));

        auto beg = flatjson::begin(&parser);
        assert(beg.is_valid());
        assert(beg.is_object());

        auto a = flatjson::at(&parser, "a");
        assert(a.is_valid());
        assert(a.is_number());
        assert(a.to_int() == 1);
    };

    test += FJ_TEST(test for parse binds a new document to the parser) {
        static const char arr[] = "[1,2,3]";
        static const char obj[] = R"({"k":true})";

        auto *parser = flatjson::alloc_parser();
        assert(parser);

        flatjson::error_code ec = flatjson::FJ_OK;
        auto toknum = flatjson::parse(parser, flatjson::args.error_code = &ec);
        assert(toknum == 0);
        assert(ec == flatjson::FJ_INVALID);

        toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(arr)
            ,flatjson::args.json_end = std::end(arr)
        );
        assert(toknum == 5);
        assert(flatjson::is_array(parser));

        toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(obj)
            ,flatjson::args.json_end = std::end(obj)
        );
        assert(toknum == 4);
        assert(flatjson::is_object(parser));
        assert(flatjson::at(parser, "k").to_bool());

        std::string big = "[";
        for ( int i = 0; i < 10000; ++i ) {
            big += "1,";
        }
        big += "1]";
        ec = flatjson::FJ_OK;
        toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = big.c_str()
            ,flatjson::args.json_end = big.c_str() + big.size() + 1
            ,flatjson::args.error_code = &ec
        );
        assert(ec == flatjson::FJ_OK);
        assert(flatjson::is_valid(parser));
        assert(toknum == 10003);

        const auto *toks = parser->toks_beg;
        toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(arr)
            ,flatjson::args.json_end = std::end(arr)
        );
        assert(toknum == 5);
        assert(parser->toks_beg == toks);
        flatjson::free_parser(parser);

        flatjson::token stack_toks[8]{};
        auto sparser = flatjson::make_parser(
             flatjson::args.toks_beg = stack_toks
            ,flatjson::args.toks_end = stack_toks + 8
        );
        toknum = flatjson::parse(
             &sparser
            ,flatjson::args.json_beg = std::begin(obj)
            ,flatjson::args.json_end = std::end(obj)
        );
        assert(toknum == 4);
        toknum = flatjson::parse(
             &sparser
            ,flatjson::args.json_beg = std::begin(arr)
            ,flatjson::args.json_end = std::end(arr)
        );
        assert(toknum == 5);
        assert(flatjson::at(&sparser, 2).to_int() == 3);

        toknum = flatjson::parse(
             arr
            ,flatjson::args.toks_beg = stack_toks
            ,flatjson::args.toks_end = stack_toks + 8
        );
        assert(toknum == 5);
        auto *lparser = flatjson::parse(obj);
        assert(lparser && flatjson::is_valid(lparser));
        assert(flatjson::num_tokens(lparser) == 4);
        flatjson::free_parser(lparser);
    };

    test += FJ_TEST(test for zero-copy string views) {
        static const char json[] = R"({"sv":"view","arr":["alpha","beta","gamma"]})";
        const char *json_begin = std::begin(json);
        const char *json_end = std::end(json) - 1;

        flatjson::token toks[64]{};
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 64
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(json)
            ,flatjson::args.json_end = std::end(json)
        );
        assert(toknum > 0);
        assert(flatjson::is_valid(&parser));

        auto sv = flatjson::at(&parser, "sv");
        assert(sv.is_string());
        auto v = sv.to_string_view();
        assert(v == "view");
        assert(v.data() >= json_begin);
        assert(v.data() + v.size() <= json_end);

        auto arr = flatjson::at(&parser, "arr");
        assert(arr.is_array());
        auto arr_end = flatjson::end(arr);
        for (auto it = flatjson::next(arr);
             flatjson::not_equal(it, arr_end);
             it = flatjson::next(it)) {
            assert(it.is_string());
            auto s = it.to_string_view();
            assert(s.data() >= json_begin);
            assert(s.data() + s.size() <= json_end);
        }
    };

    test += FJ_TEST(test for STRING) {
        static const char str[] = R"("string")";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_valid());
        assert(beg.is_string());
        assert(beg.to_string_view() == "string");

        auto end = flatjson::end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for BOOL-false) {
        static const char str[] = R"(false)";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_valid());
        assert(beg.is_bool());
        assert(beg.to_bool() == false);

        auto end = flatjson::end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for BOOL-true) {
        static const char str[] = R"(true)";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_valid());
        assert(beg.is_bool());
        assert(beg.to_bool() == true);

        auto end = flatjson::end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for NULL) {
        static const char str[] = R"(null)";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 1);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_valid());
        assert(beg.is_null());

        auto end = flatjson::end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for empty ARRAY) {
        static const char str[] = R"([])";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 2);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_valid());
        assert(beg.is_array());

        auto end = flatjson::end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for parse ARRAY) {
        static const char str[] = R"([1,0])";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 4);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_array());

        auto end = flatjson::end(parser);
        assert(end.cur == beg.end);
        assert(end.cur == end.end);

        auto i0 = flatjson::at(beg, 0);
        assert(i0.is_valid());
        assert(i0.is_number());
        assert(i0.value() == "1");
        assert(i0.cur == parser->toks_beg + 1);

        auto i1 = flatjson::at(beg, 1);
        assert(i1.is_valid());
        assert(i1.is_number());
        assert(i1.value() == "0");
        assert(i1.cur == parser->toks_beg + 2);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for empty JSON OBJECT) {
        static const char str[] = R"({})";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 2);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for parse OBJECT) {
        static const char str[] = R"({"bb":0, "b":1})";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 6);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for parse OBJECT with ARRAY as value) {
        static const char str[] = R"({"a":[1,0]})";

        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 7);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        flatjson::dump_tokens(stdout, "object with array", parser);

        flatjson::free_parser(parser);
    };

    // Phase 1+2: iterator tests ported to current API

    test += FJ_TEST(test for parse ARRAY with OBJECT as value) {
        static const char str[] = R"([{"a":0, "b":1}])";
        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 8);
        assert(parser->toks_end >= parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_array());

        (void)flatjson::end(parser);

        auto obj = flatjson::next(beg);
        assert(obj.is_valid());
        assert(obj.is_object());
        assert(obj.cur == parser->toks_beg + 1);

        auto a = flatjson::at(obj, "a");
        assert(a.is_valid());
        assert(a.is_number());
        assert(a.value() == "0");

        auto b = flatjson::at(obj, "b");
        assert(b.is_valid());
        assert(b.is_number());
        assert(b.value() == "1");

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for stack-allocated parser and tokens) {
        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        flatjson::token tokens[14];
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(&parser));
        assert(toknum == 12);
        assert(parser.toks_beg + toknum == parser.toks_cur);
        assert(parser.toks_beg->primitive_only == 1);

        const char *json = str;
        assert(tokens[0].type == flatjson::FJ_TYPE_OBJECT);
        assert(tokens[0].parent_idx == 0);

        assert(tokens[1].type == flatjson::FJ_TYPE_STRING);
        assert(flatjson::string_view(json + tokens[1].vpos, tokens[1].vlen) == "a");
        assert(tokens[2].type == flatjson::FJ_TYPE_BOOL);
        assert(flatjson::string_view(json + tokens[2].vpos, tokens[2].vlen) == "true");

        assert(tokens[3].type == flatjson::FJ_TYPE_STRING);
        assert(flatjson::string_view(json + tokens[3].vpos, tokens[3].vlen) == "b");
        assert(tokens[4].type == flatjson::FJ_TYPE_BOOL);
        assert(flatjson::string_view(json + tokens[4].vpos, tokens[4].vlen) == "false");

        assert(tokens[5].type == flatjson::FJ_TYPE_STRING);
        assert(flatjson::string_view(json + tokens[5].vpos, tokens[5].vlen) == "c");
        assert(tokens[6].type == flatjson::FJ_TYPE_NULL);
        assert(flatjson::string_view(json + tokens[6].vpos, tokens[6].vlen) == "null");

        assert(tokens[7].type == flatjson::FJ_TYPE_STRING);
        assert(flatjson::string_view(json + tokens[7].vpos, tokens[7].vlen) == "d");
        assert(tokens[8].type == flatjson::FJ_TYPE_NUMBER);
        assert(flatjson::string_view(json + tokens[8].vpos, tokens[8].vlen) == "0");

        assert(tokens[9].type == flatjson::FJ_TYPE_STRING);
        assert(flatjson::string_view(json + tokens[9].vpos, tokens[9].vlen) == "e");
        assert(tokens[10].type == flatjson::FJ_TYPE_STRING);
        assert(flatjson::string_view(json + tokens[10].vpos, tokens[10].vlen) == "e");

        assert(tokens[11].type == flatjson::FJ_TYPE_OBJECT_END);

        flatjson::free_parser(&parser);
    };

    test += FJ_TEST(test for dyn-allocated parser and stack-allocated tokens) {
        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        flatjson::token tokens[14];
        auto *parser = flatjson::alloc_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
            ,flatjson::args.mem_fns = {&my_realloc, &my_free}
        );

        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(myallocator.allocations() == 1);

        assert(flatjson::is_valid(parser));
        assert(toknum == 12);
        assert(parser->toks_beg + toknum == parser->toks_cur);
        assert(parser->toks_beg->primitive_only == 1);

        const char *json = str;
        assert(tokens[0].type == flatjson::FJ_TYPE_OBJECT);
        assert(tokens[1].type == flatjson::FJ_TYPE_STRING);
        assert(tokens[2].type == flatjson::FJ_TYPE_BOOL);
        assert(flatjson::string_view(json + tokens[2].vpos, tokens[2].vlen) == "true");
        assert(tokens[3].type == flatjson::FJ_TYPE_STRING);
        assert(tokens[4].type == flatjson::FJ_TYPE_BOOL);
        assert(flatjson::string_view(json + tokens[4].vpos, tokens[4].vlen) == "false");
        assert(tokens[5].type == flatjson::FJ_TYPE_STRING);
        assert(tokens[6].type == flatjson::FJ_TYPE_NULL);
        assert(tokens[7].type == flatjson::FJ_TYPE_STRING);
        assert(tokens[8].type == flatjson::FJ_TYPE_NUMBER);
        assert(tokens[9].type == flatjson::FJ_TYPE_STRING);
        assert(tokens[10].type == flatjson::FJ_TYPE_STRING);
        assert(tokens[11].type == flatjson::FJ_TYPE_OBJECT_END);

        flatjson::free_parser(parser);

        assert(myallocator.allocations() == 0);
        assert(myallocator.total_alloc() == 0);
    };

    test += FJ_TEST(test for dyn-allocated parser and dyn-allocated tokens) {
        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        auto *parser = flatjson::alloc_parser(flatjson::args.mem_fns = {&my_realloc, &my_free});

        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );
        assert(flatjson::is_valid(parser));
        assert(toknum == 12);
        assert(parser->toks_end >= parser->toks_beg + toknum);
        assert(parser->toks_beg->primitive_only == 1);

        assert(myallocator.allocations() == 2);

        auto *tokens = parser->toks_beg;
        assert(tokens[0].type == flatjson::FJ_TYPE_OBJECT);
        assert(tokens[1].type == flatjson::FJ_TYPE_STRING);
        assert(tokens[2].type == flatjson::FJ_TYPE_BOOL);
        assert(tokens[11].type == flatjson::FJ_TYPE_OBJECT_END);

        flatjson::free_parser(parser);

        assert(myallocator.allocations() == 0);
        assert(myallocator.total_alloc() == 0);
    };

    test += FJ_TEST(test for dyn-allocated tokens grow past the initial capacity) {
        std::string str = "[";
        for ( int i = 0; i < 40; ++i ) {
            if ( i ) {
                str += ',';
            }
            str += std::to_string(i);
        }
        str += ']';

        auto *parser = flatjson::alloc_parser(
             flatjson::args.mem_fns = {&my_realloc, &my_free}
            ,flatjson::args.init_size = std::size_t{4}
        );
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = str.c_str()
            ,flatjson::args.json_end = str.c_str() + str.size() + 1
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 42);
        assert(parser->toks_cur == parser->toks_beg + toknum);
        assert(parser->toks_end > parser->toks_beg + 4);
        assert(myallocator.allocations() == 2);

        auto beg = flatjson::begin(parser);
        auto end_it = flatjson::end(beg);
        assert(beg.is_array());
        auto idx = 0u;
        for (auto it = flatjson::next(beg);
             flatjson::not_equal(it, end_it);
             it = flatjson::next(it), ++idx) {
            assert(it.is_number());
            assert(it.to_uint() == idx);
        }
        assert(idx == 40);

        flatjson::free_parser(parser);
        assert(myallocator.allocations() == 0);
        assert(myallocator.total_alloc() == 0);
    };

    test += FJ_TEST(test for dyn-allocated tokens grow with the default allocator) {
        std::string str = "[";
        for ( int i = 0; i < 5000; ++i ) {
            if ( i ) {
                str += ',';
            }
            str += std::to_string(i % 10);
        }
        str += ']';

        auto *parser = flatjson::alloc_parser(flatjson::args.init_size = std::size_t{4});
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = str.c_str()
            ,flatjson::args.json_end = str.c_str() + str.size() + 1
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 5002);
        assert(parser->toks_cur == parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        auto end_it = flatjson::end(beg);
        auto idx = 0u;
        for (auto it = flatjson::next(beg);
             flatjson::not_equal(it, end_it);
             it = flatjson::next(it), ++idx) {
            assert(it.is_number());
            assert(it.to_uint() == idx % 10u);
        }
        assert(idx == 5000);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for dyn-allocated tokens start capacity from the JSON size) {
        std::string str = "{\"k\":[";
        for ( int i = 0; i < 1000; ++i ) {
            if ( i ) {
                str += ',';
            }
            str += "\"abcdefgh\"";
        }
        str += "]}";
        const char *data = str.c_str();

        auto *parser = flatjson::alloc_parser(flatjson::args.mem_fns = {&my_realloc, &my_free});
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = data
            ,flatjson::args.json_end = data + str.size() + 1
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 1005);
        const auto cap = static_cast<std::size_t>(
            parser->toks_end - parser->toks_beg
        );
        assert(cap == (str.size() + 1u) / 4u + FJ_TOKEN_INIT_CAP);
        assert(myallocator.allocations() == 2);

        flatjson::free_parser(parser);
        assert(myallocator.allocations() == 0);
    };

    test += FJ_TEST(test for case when lack of stack-allocated tokens) {
        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        flatjson::token tokens[4]{};
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(!flatjson::is_valid(&parser));
        assert(toknum == 0);
        assert(flatjson::num_tokens(&parser) == 4);
        assert(flatjson::get_error(&parser) == flatjson::FJ_NO_FREE_TOKENS);

        flatjson::free_parser(&parser);
    };

    test += FJ_TEST(test for end iterator pointing to the latest token which is ARRAY_END) {
        static const char str[] = R"([{"c":3, "f":5}])";
        flatjson::token tokens[10];
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(&parser));
        assert(toknum == 8);
        assert(parser.toks_beg + toknum == parser.toks_cur);

        auto beg = flatjson::begin(&parser);
        assert(beg.is_array());

        auto end_it = flatjson::end(&parser);
        assert(end_it.type() == flatjson::FJ_TYPE_ARRAY_END);

        auto nxt = flatjson::next(beg);
        assert(flatjson::not_equal(nxt, end_it));
        assert(nxt.is_object());

        flatjson::free_parser(&parser);
    };

    test += FJ_TEST(test for end iterator pointing to the latest token which is OBJECT_END) {
        static const char str[] = R"({"a":true, "b":false, "c":null})";
        flatjson::token tokens[10];
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(&parser));
        assert(toknum == 8);
        assert(parser.toks_beg + toknum == parser.toks_cur);
        assert(parser.toks_beg->primitive_only == 1);

        auto beg = flatjson::begin(&parser);
        assert(beg.is_object());

        auto end_it = flatjson::end(&parser);
        assert(end_it.type() == flatjson::FJ_TYPE_OBJECT_END);

        auto m = flatjson::members(beg);
        assert(m == 3);
    };

    test += FJ_TEST(test for iteration through the OBJECT using iterators) {
        static const char str[] = R"({"a":true, "b":false, "c":null, "d":0, "e":"e"})";
        flatjson::token tokens[14];
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(&parser));
        assert(toknum == 12);
        assert(parser.toks_beg + toknum == parser.toks_cur);
        assert(parser.toks_beg->primitive_only == 1);

        auto beg = flatjson::begin(&parser);
        assert(beg.is_object());

        auto end_it = flatjson::end(&parser);
        assert(end_it.type() == flatjson::FJ_TYPE_OBJECT_END);

        auto dist = flatjson::members(beg);
        assert(dist == 5);

        std::size_t idx = 0u;
        for (auto it = flatjson::next(beg);
             flatjson::not_equal(it, end_it);
             it = flatjson::next(it)) {
            idx = flatjson::distance(flatjson::next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.key() == "a");
                    assert(it.is_bool());
                    assert(it.value() == "true");
                    break;
                }
                case 1: {
                    assert(it.key() == "b");
                    assert(it.is_bool());
                    assert(it.value() == "false");
                    break;
                }
                case 2: {
                    assert(it.key() == "c");
                    assert(it.is_null());
                    assert(it.value() == "null");
                    break;
                }
                case 3: {
                    assert(it.key() == "d");
                    assert(it.is_number());
                    assert(it.value() == "0");
                    break;
                }
                case 4: {
                    assert(it.key() == "e");
                    assert(it.is_string());
                    assert(it.value() == "e");
                    break;
                }
                default: assert(!"unreachable!");
            }
        }
        assert(idx == 4);
    };

    test += FJ_TEST(test for iteration through the ARRAY using iterators) {
        static const char str[] = R"([4,3,2,1])";
        flatjson::token tokens[10];
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(&parser));
        assert(toknum == 6);
        assert(parser.toks_beg + toknum == parser.toks_cur);
        assert(parser.toks_beg->primitive_only == 1);

        auto beg = flatjson::begin(&parser);
        assert(beg.is_array());

        auto end_it = flatjson::end(&parser);
        assert(end_it.type() == flatjson::FJ_TYPE_ARRAY_END);

        auto dist = flatjson::members(beg);
        assert(dist == 4);

        const char *json = str;
        bool case_0 = false;
        bool case_1 = false;
        bool case_2 = false;
        bool case_3 = false;
        std::size_t idx = 0u;
        for (auto it = flatjson::next(beg);
             flatjson::not_equal(it, end_it);
             it = flatjson::next(it)) {
            idx = flatjson::distance(flatjson::next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.is_number());
                    case_0 = it.cur->type == flatjson::FJ_TYPE_NUMBER
                        && *(json + it.cur->vpos) == '4';
                    break;
                }
                case 1: {
                    assert(it.is_number());
                    case_1 = it.cur->type == flatjson::FJ_TYPE_NUMBER
                        && *(json + it.cur->vpos) == '3';
                    break;
                }
                case 2: {
                    assert(it.is_number());
                    case_2 = it.cur->type == flatjson::FJ_TYPE_NUMBER
                        && *(json + it.cur->vpos) == '2';
                    break;
                }
                case 3: {
                    assert(it.is_number());
                    case_3 = it.cur->type == flatjson::FJ_TYPE_NUMBER
                        && *(json + it.cur->vpos) == '1';
                    break;
                }
                default: assert(!"unreachable!");
            }
        }

        assert(idx == 3);
        assert(case_0);
        assert(case_1);
        assert(case_2);
        assert(case_3);

        flatjson::free_parser(&parser);
    };

    test += FJ_TEST(test for iteration through the ARRAY of ARRAYS using iterators) {
        static const char str[] = R"([[4],[3],[2],[1]])";
        flatjson::token tokens[14];
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(&parser));
        assert(toknum == 14);
        assert(parser.toks_end == parser.toks_beg + toknum);
        assert(parser.toks_beg->primitive_only == 0);

        auto beg = flatjson::begin(&parser);
        assert(beg.is_array());

        auto end_it = flatjson::end(&parser);
        assert(end_it.type() == flatjson::FJ_TYPE_ARRAY_END);

        auto dist = flatjson::members(beg);
        assert(dist == 4);

        const char *json = str;
        bool case_0 = false;
        bool case_1 = false;
        bool case_2 = false;
        bool case_3 = false;
        std::size_t idx = 0u;
        for (auto it = flatjson::next(beg);
             flatjson::not_equal(it, end_it);
             it = flatjson::next(it)) {
            idx = flatjson::distance(flatjson::next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.is_array());
                    case_0 = (it.cur + 1)->type == flatjson::FJ_TYPE_NUMBER
                        && *(json + (it.cur + 1)->vpos) == '4';
                    break;
                }
                case 1: {
                    assert(it.is_array());
                    case_1 = (it.cur + 1)->type == flatjson::FJ_TYPE_NUMBER
                        && *(json + (it.cur + 1)->vpos) == '3';
                    break;
                }
                case 2: {
                    assert(it.is_array());
                    case_2 = (it.cur + 1)->type == flatjson::FJ_TYPE_NUMBER
                        && *(json + (it.cur + 1)->vpos) == '2';
                    break;
                }
                case 3: {
                    assert(it.is_array());
                    case_3 = (it.cur + 1)->type == flatjson::FJ_TYPE_NUMBER
                        && *(json + (it.cur + 1)->vpos) == '1';
                    break;
                }
                default: assert(!"unreachable!");
            }
        }

        assert(idx == 3);
        assert(case_0);
        assert(case_1);
        assert(case_2);
        assert(case_3);

        flatjson::free_parser(&parser);
    };

    test += FJ_TEST(test for access to the ARRAY values using search by index) {
        static const char str[] = R"([0, "1", 3.14, -314])";
        flatjson::token tokens[10]{};
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(&parser));
        assert(toknum == 6);
        assert(parser.toks_beg + toknum == parser.toks_cur);
        assert(parser.toks_beg->primitive_only == 1);

        assert(flatjson::num_tokens(&parser) == 6);
        assert(!flatjson::is_object(&parser));
        assert(flatjson::is_array(&parser));
        assert(!flatjson::is_null(&parser));
        assert(!flatjson::is_string(&parser));
        assert(!flatjson::is_bool(&parser));
        assert(!flatjson::is_number(&parser));

        auto pbeg = flatjson::begin(&parser);
        assert(pbeg.is_array());
        auto dist = flatjson::members(pbeg);
        assert(dist == 4);

        auto it0 = flatjson::at(&parser, static_cast<std::size_t>(0));
        assert(it0.is_number());
        assert(it0.to_int() == 0);

        auto it1 = flatjson::at(&parser, static_cast<std::size_t>(1));
        assert(it1.is_string());
        assert(it1.to_string_view() == "1");

        auto it2 = flatjson::at(&parser, static_cast<std::size_t>(2));
        assert(it2.is_number());
        assert(it2.to_double() == 3.14);

        auto it3 = flatjson::at(&parser, static_cast<std::size_t>(3));
        assert(it3.is_number());
        assert(it3.to_int() == -314);
    };

    test += FJ_TEST(test for access to the OBJECT values using search by key) {
        static const char str[] = R"({"a":true, "b":"false", "c":null})";
        flatjson::token tokens[10];
        auto parser = flatjson::make_parser(
             flatjson::args.toks_beg = std::begin(tokens)
            ,flatjson::args.toks_end = std::end(tokens)
        );
        auto toknum = flatjson::parse(
             &parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(&parser));
        assert(toknum == 8);
        assert(parser.toks_beg + toknum == parser.toks_cur);
        assert(parser.toks_beg->primitive_only == 1);

        assert(flatjson::num_tokens(&parser) == 8);
        assert(flatjson::is_object(&parser));
        assert(!flatjson::is_array(&parser));
        assert(!flatjson::is_null(&parser));
        assert(!flatjson::is_string(&parser));
        assert(!flatjson::is_bool(&parser));
        assert(!flatjson::is_number(&parser));

        auto it0 = flatjson::at(&parser, "a");
        assert(it0.is_bool());
        assert(it0.to_bool() == true);

        auto it1 = flatjson::at(&parser, "b");
        assert(it1.is_string());
        assert(it1.to_string_view() == "false");

        auto it2 = flatjson::at(&parser, "c");
        assert(it2.is_null());
        assert(it2.to_string_view() == "null");
    };

    test += FJ_TEST(test for access to the nested OBJECT values using search by key) {
        static const char str[] = R"({"a":{"b":true, "c":1234}})";
        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 9);
        assert(parser->toks_end >= parser->toks_beg + toknum);
        assert(parser->toks_beg->primitive_only == 0);

        assert(flatjson::is_object(parser));
        assert(!flatjson::is_array(parser));
        assert(!flatjson::is_number(parser));
        assert(!flatjson::is_string(parser));
        assert(!flatjson::is_null(parser));
        assert(!flatjson::is_bool(parser));

        auto j0 = flatjson::at(parser, "a");
        assert(j0.is_valid());
        assert(j0.is_object());
        assert(!j0.is_array());
        assert(!j0.is_number());
        assert(!j0.is_string());
        assert(!j0.is_null());
        assert(!j0.is_bool());

        auto j1 = flatjson::at(j0, "b");
        assert(j1.is_valid());
        assert(j1.is_bool());
        assert(j1.to_bool() == true);
        assert(j0.parent() == parser->toks_beg);
        assert(j1.parent() == j0.cur);

        auto j2 = flatjson::at(j0, "c");
        assert(j2.is_valid());
        assert(j2.is_number());
        assert(j2.to_int() == 1234);

        auto j3 = flatjson::at(j0, "d");
        assert(flatjson::equal(j3, flatjson::end(j0)));

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for iterate through ARRAY with nested OBJECTS) {
        static const char str[] = R"({"a":[4,3,2,1], "b":[{"a":0,"b":1,"c":2},{"b":4,"a":3,"c":5},{"c":8,"b":7,"a":6}], "c":[0,1,2,3]})";
        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 43);
        assert(parser->toks_end >= parser->toks_beg + toknum);
        assert(parser->toks_beg->primitive_only == 0);

        auto pbeg = flatjson::begin(parser);
        auto pend = flatjson::end(pbeg);
        assert(pend.type() == flatjson::FJ_TYPE_OBJECT_END);
        auto pdist = flatjson::members(pbeg);
        assert(pdist == 3);

        auto a = flatjson::at(parser, "a");
        assert(a.is_valid());
        assert(a.is_array());

        (void)flatjson::begin(a);
        auto aend = flatjson::end(a);
        assert(aend.type() == flatjson::FJ_TYPE_ARRAY_END);

        auto b = flatjson::at(parser, "b");
        assert(b.is_valid());
        assert(b.is_array());

        auto bbeg = flatjson::begin(b);
        auto bend = flatjson::end(b);

        bool dist_0 = false;
        bool dist_1 = false;
        bool dist_2 = false;
        std::size_t idx = 0u;
        for (auto it = flatjson::next(bbeg);
             flatjson::not_equal(it, bend);
             it = flatjson::next(it)) {
            idx = flatjson::distance(flatjson::next(bbeg), it);
            switch ( idx ) {
                case 0: {
                    auto a0 = flatjson::at(it, "a");
                    assert(flatjson::not_equal(a0, it));
                    assert(a0.is_valid());
                    assert(a0.is_number());
                    assert(a0.value() == "0");

                    auto b0 = flatjson::at(it, "b");
                    assert(flatjson::not_equal(b0, it));
                    assert(b0.is_valid());
                    assert(b0.is_number());
                    assert(b0.value() == "1");

                    auto c0 = flatjson::at(it, "c");
                    assert(flatjson::not_equal(c0, it));
                    assert(c0.is_valid());
                    assert(c0.is_number());
                    assert(c0.value() == "2");

                    dist_0 = true;
                    break;
                }
                case 1: {
                    auto a1 = flatjson::at(it, "a");
                    assert(a1.is_valid());
                    assert(a1.is_number());
                    assert(a1.value() == "3");

                    auto b1 = flatjson::at(it, "b");
                    assert(b1.is_valid());
                    assert(b1.is_number());
                    assert(b1.value() == "4");

                    auto c1 = flatjson::at(it, "c");
                    assert(c1.is_valid());
                    assert(c1.is_number());
                    assert(c1.value() == "5");

                    dist_1 = true;
                    break;
                }
                case 2: {
                    auto a2 = flatjson::at(it, "a");
                    assert(a2.is_valid());
                    assert(a2.is_number());
                    assert(a2.value() == "6");

                    auto b2 = flatjson::at(it, "b");
                    assert(b2.is_valid());
                    assert(b2.is_number());
                    assert(b2.value() == "7");

                    auto c2 = flatjson::at(it, "c");
                    assert(c2.is_valid());
                    assert(c2.is_number());
                    assert(c2.value() == "8");

                    dist_2 = true;
                    break;
                }
                default:
                    assert(!"unreachable!");
            }
        }

        assert(idx == 2);
        assert(dist_0);
        assert(dist_1);
        assert(dist_2);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for iterate using index through ARRAY which is value of OBJECT) {
        static const char str[] = R"({"a":[0,1,2]})";
        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 8);
        assert(parser->toks_end >= parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_object());
        auto end_it = flatjson::end(parser);
        assert(end_it.type() == flatjson::FJ_TYPE_OBJECT_END);

        auto j0 = flatjson::at(beg, "a");
        assert(flatjson::not_equal(j0, end_it));
        assert(j0.is_array());

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for iterate using iterator through ARRAY which is value of OBJECT) {
        static const char str[] = R"({"a":[0,1,2]})";
        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 8);
        assert(parser->toks_end >= parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        assert(beg.is_object());
        auto end_it = flatjson::end(parser);
        assert(end_it.type() == flatjson::FJ_TYPE_OBJECT_END);

        auto j0 = flatjson::at(beg, "a");
        assert(flatjson::not_equal(j0, end_it));
        assert(j0.is_array());

        auto j0end = flatjson::end(j0);
        auto idx = 0u;
        for (auto it = flatjson::next(j0);
             flatjson::not_equal(it, j0end);
             it = flatjson::next(it), ++idx) {
            assert(it.is_simple_type());
            assert(it.to_uint() == idx);
        }

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for iterate through OBJECT with excluded nested OBJECTS) {
        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 15);
        assert(parser->toks_end >= parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        auto end_it = flatjson::end(parser);
        assert(flatjson::members(beg) == 4);

        bool case_0 = false;
        bool case_1 = false;
        bool case_2 = false;
        bool case_3 = false;
        std::size_t idx = 0u;
        for (auto it = flatjson::next(beg);
             flatjson::not_equal(it, end_it);
             it = flatjson::next(it)) {
            idx = flatjson::distance(flatjson::next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.key() == "a");
                    assert(it.is_number());
                    assert(it.value() == "0");
                    case_0 = true;
                    break;
                }
                case 1: {
                    assert(it.key() == "b");
                    assert(it.is_number());
                    assert(it.value() == "1");
                    case_1 = true;
                    break;
                }
                case 2: {
                    assert(it.key() == "c");
                    assert(it.is_object());
                    assert(it.value() == "");
                    case_2 = true;
                    break;
                }
                case 3: {
                    assert(it.key() == "f");
                    assert(it.is_number());
                    assert(it.value() == "4");
                    case_3 = true;
                    break;
                }
                default: assert(!"unreachable!");
            }
        }

        assert(idx == 3);
        assert(case_0);
        assert(case_1);
        assert(case_2);
        assert(case_3);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for iterate through OBJECT including nested OBJECT) {
        static const char str[] = R"({"a":0, "b":1, "c":{"d":2, "e":3}, "f":4})";
        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );

        assert(flatjson::is_valid(parser));
        assert(toknum == 15);
        assert(parser->toks_end >= parser->toks_beg + toknum);

        auto beg = flatjson::begin(parser);
        auto end_it = flatjson::end(parser);
        assert(flatjson::members(beg) == 4);

        bool case_0 = false;
        bool case_1 = false;
        bool case_2 = false;
        bool case_2_0 = false;
        bool case_2_1 = false;
        bool case_3 = false;
        std::size_t idx = 0u;
        for (auto it = flatjson::next(beg);
             flatjson::not_equal(it, end_it);
             it = flatjson::next(it)) {
            idx = flatjson::distance(flatjson::next(beg), it);
            switch ( idx ) {
                case 0: {
                    assert(it.key() == "a");
                    assert(it.value() == "0");
                    case_0 = true;
                    break;
                }
                case 1: {
                    assert(it.key() == "b");
                    assert(it.value() == "1");
                    case_1 = true;
                    break;
                }
                case 2: {
                    assert(it.key() == "c");
                    assert(it.is_object());
                    auto cbeg = flatjson::begin(it);
                    auto cend = flatjson::end(it);
                    std::size_t idx2 = 0u;
                    for (auto cit = flatjson::next(cbeg);
                         flatjson::not_equal(cit, cend);
                         cit = flatjson::next(cit)) {
                        idx2 = flatjson::distance(flatjson::next(cbeg), cit);
                        switch ( idx2 ) {
                            case 0: {
                                assert(cit.key() == "d");
                                assert(cit.value() == "2");
                                case_2_0 = true;
                                break;
                            }
                            case 1: {
                                assert(cit.key() == "e");
                                assert(cit.value() == "3");
                                case_2_1 = true;
                                break;
                            }
                            default: assert(!"unreachable");
                        }
                    }

                    assert(idx2 == 1);
                    case_2 = true;
                    break;
                }
                case 3: {
                    assert(it.key() == "f");
                    assert(it.value() == "4");
                    case_3 = true;
                    break;
                }
                default: assert(!"unreachable");
            }
        }

        assert(idx == 3);
        assert(case_0);
        assert(case_1);
        assert(case_2);
        assert(case_2_0);
        assert(case_2_1);
        assert(case_3);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for serialize APIs) {
        static const char str[] = R"({"a":[1,2],"b":{"c":"x\n\"\u0041"},"d":[],"e":{},"f":[true,false,null,-1.5e+3]})";
        static const char pretty_str[] =
            "{\n"
            "   \"a\": [\n"
            "      1,\n"
            "      2\n"
            "   ],\n"
            "   \"b\": {\n"
            "      \"c\": \"x\\n\\\"\\u0041\"\n"
            "   },\n"
            "   \"d\": [],\n"
            "   \"e\": {},\n"
            "   \"f\": [\n"
            "      true,\n"
            "      false,\n"
            "      null,\n"
            "      -1.5e+3\n"
            "   ]\n"
            "}";
        auto *parser = flatjson::alloc_parser();
        auto toknum = flatjson::parse(
             parser
            ,flatjson::args.json_beg = std::begin(str)
            ,flatjson::args.json_end = std::end(str)
        );
        assert(flatjson::is_valid(parser));
        assert(toknum > 0);

        // The string tokens keep the escapes of the source.
        const std::string compact = flatjson::to_string(parser);
        assert(compact == str);
        assert(flatjson::required_bytes(parser) == compact.size());

        const std::string pretty = flatjson::to_string(parser, true);
        assert(pretty == pretty_str);
        assert(flatjson::required_bytes(parser, true) == pretty.size());

        std::vector<char> buf(pretty.size() + 1, '#');
        assert(flatjson::to_buffer(buf.data(), parser) == buf.data() + compact.size());
        assert(std::string(buf.data(), compact.size()) == compact);
        assert(buf[compact.size()] == '#');
        assert(flatjson::to_buffer(buf.data(), parser, true) == buf.data() + pretty.size());
        assert(std::string(buf.data(), pretty.size()) == pretty);
        assert(buf[pretty.size()] == '#');

        const flatjson::parser *cparser = parser;
        assert(flatjson::to_string(cparser) == compact);
        flatjson::fjson whole{parser};
        assert(flatjson::to_string(whole) == compact);
        assert(flatjson::to_string(flatjson::at(parser, "a")) == "[1,2]");
        assert(flatjson::to_string(flatjson::at(parser, "b"), true)
            == "{\n   \"c\": \"x\\n\\\"\\u0041\"\n}");
        assert(flatjson::to_string(flatjson::at(
             flatjson::at(parser, "f")
            ,std::size_t{3}
        )) == "-1.5e+3");
        assert(flatjson::to_string(flatjson::begin(parser)) == compact);

        // Nothing to write: an end iterator, an unfinished parser, an empty fjson.
        assert(flatjson::required_bytes(flatjson::end(parser)) == 0u);
        assert(flatjson::to_string(flatjson::end(parser)).empty());
        assert(flatjson::to_buffer(buf.data(), flatjson::end(parser)) == buf.data());
        auto *unfinished = flatjson::alloc_parser();
        static const char part[] = R"({"a":[1,)";
        flatjson::feed(unfinished, part, sizeof(part) - 1);
        assert(flatjson::required_bytes(unfinished) == 0u);
        assert(flatjson::to_string(unfinished).empty());
        assert(flatjson::to_string(flatjson::fjson{}).empty());
        const auto sink = [](const void *, std::size_t) { return true; };
        assert(!flatjson::to_chunked_buffer(buf.data(), buf.size(), unfinished, sink));
        flatjson::free_parser(unfinished);

        // Top-level leaves.
        auto *leaf = flatjson::alloc_parser();
        static const char num[] = "42";
        flatjson::parse(
             leaf
            ,flatjson::args.json_beg = std::begin(num)
            ,flatjson::args.json_end = std::end(num)
        );
        assert(flatjson::to_string(leaf) == "42");
        assert(flatjson::to_string(leaf, true) == "42");
        static const char text[] = R"("a\tb")";
        flatjson::parse(
             leaf
            ,flatjson::args.json_beg = std::begin(text)
            ,flatjson::args.json_end = std::end(text)
        );
        assert(flatjson::to_string(leaf) == text);
        flatjson::free_parser(leaf);

        // NaN and Infinity are not JSON.
        auto *inf_nan = flatjson::alloc_parser();
        static const char inf_nan_str[] = "[NaN,Infinity,-Infinity,-1]";
        flatjson::parse(
             inf_nan
            ,flatjson::args.json_beg = std::begin(inf_nan_str)
            ,flatjson::args.json_end = std::end(inf_nan_str)
            ,flatjson::args.flags = flatjson::FJ_PARSE_ALLOW_INF_NAN
        );
        assert(flatjson::is_valid(inf_nan));
        assert(flatjson::to_string(inf_nan) == "[null,null,null,-1]");
        assert(flatjson::required_bytes(inf_nan) == std::strlen("[null,null,null,-1]"));
        flatjson::free_parser(inf_nan);

        // The deepest nesting the parser accepts.
        std::string deep;
        for ( std::size_t depth = flatjson::details::k_parse_stack_max; depth != 0u; --depth ) {
            deep.assign(depth, '[');
            deep.append(depth, ']');
            auto *p = flatjson::alloc_parser();
            flatjson::parse(
                 p
                ,flatjson::args.json_beg = deep.c_str()
                ,flatjson::args.json_end = deep.c_str() + deep.size() + 1
            );
            const bool valid = flatjson::is_valid(p);
            if ( valid ) {
                assert(flatjson::to_string(p) == deep);
                assert(flatjson::to_string(p, true).size() == flatjson::required_bytes(p, true));
            }
            flatjson::free_parser(p);
            if ( valid ) {
                break;
            }
        }

        // Any buffer size gives the same text, and fn sees no empty part.
        for ( const bool pretty_on: {false, true} ) {
            const std::string &expected = pretty_on ? pretty : compact;
            for ( std::size_t size = 1; size <= expected.size() + 1; ++size ) {
                std::string out;
                const bool ok = flatjson::to_chunked_buffer(
                     buf.data()
                    ,size
                    ,parser
                    ,[&out, size](const void *data, std::size_t len) {
                        assert(len != 0u && len <= size);
                        out.append(static_cast<const char *>(data), len);

                        return true;
                    }
                    ,pretty_on
                );
                assert(ok);
                assert(out == expected);
            }
        }

        // A buffer that holds the longest leaf with the byte after it gives parts for feed().
        const std::size_t longest_leaf = std::strlen(R"("x\n\"\u0041")") + 1u;
        for ( const bool pretty_on: {false, true} ) {
            for ( std::size_t size = longest_leaf; size <= compact.size(); ++size ) {
                std::deque<std::string> parts;
                assert(flatjson::to_chunked_buffer(
                     buf.data()
                    ,size
                    ,parser
                    ,[&parts](const void *data, std::size_t len) {
                        parts.emplace_back(static_cast<const char *>(data), len);

                        return true;
                    }
                    ,pretty_on
                ));
                auto *streamed = flatjson::alloc_parser();
                for ( std::size_t i = 0; i + 1 < parts.size(); ++i ) {
                    assert(flatjson::feed(
                         streamed
                        ,parts[i].data()
                        ,parts[i].size()
                    ).code == flatjson::FJ_INCOMPLETE);
                }
                const auto last = flatjson::feed(
                     streamed
                    ,parts.back().data()
                    ,parts.back().size()
                ).code;
                assert(last == flatjson::FJ_OK || last == flatjson::FJ_INCOMPLETE);
                assert(flatjson::feed(streamed, nullptr, 0).code == flatjson::FJ_OK);
                assert(flatjson::to_string(streamed) == compact);
                flatjson::free_parser(streamed);
            }
        }

        // fn stops the writing.
        std::size_t calls = 0;
        assert(!flatjson::to_chunked_buffer(
             buf.data()
            ,4
            ,parser
            ,[&calls](const void *, std::size_t) { ++calls; return false; }
        ));
        assert(calls == 1u);

        // An owned buffer comes from alloc_fn and goes back to free_fn.
        static std::size_t allocs, frees;
        allocs = frees = 0;
        const auto counted_alloc = [](void *ptr, std::size_t size) {
            ++allocs;

            return std::realloc(ptr, size);
        };
        const auto counted_free = [](void *ptr) { ++frees; std::free(ptr); };
        std::string owned_out;
        assert(flatjson::to_chunked_buffer(
             nullptr
            ,7
            ,parser
            ,[&owned_out](const void *data, std::size_t len) {
                owned_out.append(static_cast<const char *>(data), len);

                return true;
            }
            ,true
            ,counted_alloc
            ,counted_free
            ,true
        ));
        assert(owned_out == pretty);
        assert(allocs == 1u && frees == 1u);
        assert(!flatjson::to_chunked_buffer(
             nullptr
            ,7
            ,parser
            ,[](const void *, std::size_t) { return false; }
            ,false
            ,counted_alloc
            ,counted_free
            ,true
        ));
        assert(allocs == 2u && frees == 2u);

        flatjson::free_parser(parser);
    };

    test += FJ_TEST(test for pack and unpack state) {
        auto parse_doc = [](
             const std::string &json
            ,std::vector<flatjson::token> &toks
            ,std::size_t flags)
        {
            toks.assign(json.size() + 2, flatjson::token{});
            auto p = flatjson::make_parser(
                 flatjson::args.toks_beg = toks.data()
                ,flatjson::args.toks_end = toks.data() + toks.size()
            );
            flatjson::parse(
                 &p
                ,flatjson::args.json_beg = json.c_str()
                ,flatjson::args.json_end = json.c_str() + json.size() + 1
                ,flatjson::args.flags = static_cast<std::uint8_t>(flags)
            );
            assert(flatjson::is_valid(&p));

            return p;
        };
        auto same_tokens = [](const flatjson::parser *a, const flatjson::parser *b) {
            const auto n = static_cast<std::size_t>(a->toks_cur - a->toks_beg);
            return n == static_cast<std::size_t>(b->toks_cur - b->toks_beg)
                && std::memcmp(a->toks_beg, b->toks_beg, n * sizeof(flatjson::token))
                    == 0;
        };
        auto check = [&](
             const std::string &json
            ,std::size_t parse_flags = flatjson::FJ_PARSE_NONE)
        {
            std::vector<flatjson::token> toks;
            auto src = parse_doc(json, toks, parse_flags);

            std::vector<char> packed(flatjson::packed_state_size(&src));
            assert(!packed.empty());
            assert(flatjson::pack_state(packed.data(), packed.size(), &src) == packed.size());
            assert(std::memcmp(packed.data(), "FJS0", 4) == 0);
            // One pass into a buffer of the upper bound gives the same bytes.
            std::vector<char> roomy(flatjson::packed_state_size_max(&src));
            assert(roomy.size() >= packed.size());
            assert(flatjson::pack_state(roomy.data(), roomy.size(), &src) == packed.size());
            assert(std::memcmp(roomy.data(), packed.data(), packed.size()) == 0);
            assert(flatjson::pack_state(packed.data(), packed.size() - 1, &src) == 0);

            std::uint32_t json_len{};
            std::memcpy(&json_len, packed.data() + 4, sizeof(json_len));
            const std::string text{packed.data() + 16, json_len};
            std::vector<flatjson::token> mtoks;
            auto expected = parse_doc(text, mtoks, flatjson::FJ_PARSE_NONE);
            assert(flatjson::to_string(&expected) == flatjson::to_string(&src));
            std::vector<flatjson::token> toks2(
                static_cast<std::size_t>(src.toks_cur - src.toks_beg)
            );
            auto dst = flatjson::make_parser(
                 flatjson::args.toks_beg = toks2.data()
                ,flatjson::args.toks_end = toks2.data() + toks2.size()
            );
            assert(flatjson::unpack_state(&dst, packed.data(), packed.size()));
            assert(flatjson::is_valid(&dst));
            assert(same_tokens(&expected, &dst));
            assert(flatjson::begin(&dst).to_string_view()
                == flatjson::begin(&expected).to_string_view());
            auto *grown = flatjson::alloc_parser(flatjson::args.init_size = 1);
            assert(flatjson::unpack_state(grown, packed.data(), packed.size()));
            assert(flatjson::is_valid(grown));
            assert(same_tokens(&expected, grown));
            flatjson::free_parser(grown);

            // A damaged state is rejected or decodes within its buffer.
            const std::size_t step = packed.size() / 2048u + 1u;
            for ( std::size_t n = 0; n < packed.size(); n += step ) {
                std::vector<char> cut(packed.begin(), packed.begin() + n);
                assert(!flatjson::unpack_state(&dst, cut.data(), cut.size()));
            }
            for ( std::size_t i = 0; i < packed.size(); i += step ) {
                for ( unsigned bit: {0x01u, 0x08u, 0x80u} ) {
                    std::vector<char> bad(packed);
                    bad[i] = static_cast<char>(bad[i] ^ bit);
                    if ( flatjson::unpack_state(&dst, bad.data(), bad.size()) ) {
                        const auto *t = dst.toks_beg;
                        const auto ntoks = static_cast<std::size_t>(
                            dst.toks_cur - dst.toks_beg
                        );
                        for ( std::size_t k = 0; k < ntoks; ++k ) {
                            assert(std::size_t{t[k].vpos} + t[k].vlen <= bad.size());
                        }
                    }
                }
            }
        };

        check(R"({"a":[1,2],"b":{"c":"x"}})");
        check("42");
        check("-1.5e3");
        check("\"s\"");
        check("\"\"");
        check("true");
        check("false");
        check("null");
        check("[]");
        check("{}");
        check("[[],{},[[{}]],\"\",0]");
        check(" \n\t{ \"key\" :\n\n  [ true , false , null ,\"v\\\"\\u00e9\" ]  ,"
            " \"k2\" : { } } \n");
        check("[" + std::string(40, ' ') + "1," + std::string(200, '\n') + "\""
            + std::string(300, 'x') + "\"]");
        check("{\"" + std::string(70000, 'k') + "\":" + std::string(5000, '7') + "}");
        check("// head\n[1, // one\n 2, // tail\n 3]", flatjson::FJ_PARSE_ALLOW_COMMENTS);
        check(read_file("jsonchecker/pass01.json"));

        std::string deep;
        for ( int i = 0; i < 60; ++i ) {
            deep += "{\"a\":[";
        }
        deep += "1";
        for ( int i = 0; i < 60; ++i ) {
            deep += "]}";
        }
        check(deep);

        // Only a complete document is packed.
        flatjson::token toks[8]{};
        auto partial = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 8
        );
        static const char half[] = "[1,";
        assert(flatjson::feed(&partial, half, 3).code == flatjson::FJ_INCOMPLETE);
        assert(flatjson::packed_state_size(&partial) == 0);
        assert(flatjson::packed_state_size_max(&partial) == 0);
        char buf[64];
        assert(flatjson::pack_state(buf, sizeof(buf), &partial) == 0);

        // Fixed tokens that are too few are not grown.
        const std::string three{"[1,2,3]"};
        std::vector<flatjson::token> src_toks;
        auto src = parse_doc(three, src_toks, flatjson::FJ_PARSE_NONE);
        std::vector<char> packed(flatjson::packed_state_size(&src));
        flatjson::pack_state(packed.data(), packed.size(), &src);
        auto small = flatjson::make_parser(
             flatjson::args.toks_beg = toks
            ,flatjson::args.toks_end = toks + 2
        );
        assert(!flatjson::unpack_state(&small, packed.data(), packed.size()));
    };

    test.run();

    /*********************************************************************************************/
#if 1
    auto res = test_conformance();
    if ( res.expected != res.ec ) {
        const char *expstr = (res.expected == 0) ? "SUCCESS" : "FAIL";
        const char *retstr = (res.ec == flatjson::FJ_OK) ? "SUCCESS" : "FAIL";
        std::cout
            << "test \"" << res.path << '/' << res.fname << "\" FAILED because \"" << expstr << "\" expected but \"" << retstr << "\" was received:\n"
            << res.emsg
        << std::endl;

        return EXIT_FAILURE;
    } else {
        std::cout
            << "conformance tests PASSED!\n"
            << "**************************************************************************************"
        << std::endl;
    }
#endif // 1
    return EXIT_SUCCESS;
}

/*************************************************************************************************/

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER
