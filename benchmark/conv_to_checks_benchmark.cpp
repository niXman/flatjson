#include <flatjson/conv.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr std::size_t k_numbers = 4096;
constexpr int k_warmup = 200;
constexpr int k_repeats = 2000;
constexpr int k_runs = 11;

// res > (uint64 max - digit) / 10, written so the limit is a constant.
constexpr std::uint64_t k_u64_div10 = std::numeric_limits<std::uint64_t>::max() / 10u;
constexpr unsigned k_u64_mod10 =
    static_cast<unsigned>(std::numeric_limits<std::uint64_t>::max() % 10u);

volatile std::uint64_t g_sink = 0;

struct number {
    const char *ptr;
    std::size_t len;
};

template<typename To>
using conv_fn = To (*)(const char *, std::size_t);

template<typename To>
__attribute__((noinline))
To plain_conv(const char *ptr, std::size_t len) noexcept
{ return flatjson::details::conv_to(ptr, len, To{}); }

// Length, digit, uint64 overflow, then narrowing to To. Failure returns stay off the hot path.
#define FJ_ACCUMULATE(res, ch)                                                      \
    do {                                                                            \
        const unsigned fj_digit = static_cast<unsigned>((ch) - '0');                \
        if ( __FJ__UNLIKELY(fj_digit > 9u) ) {                                      \
            assert(!"not a digit");                                                 \
            return To{};                                                            \
        }                                                                           \
        if ( __FJ__UNLIKELY(                                                        \
            (res) > k_u64_div10                                                     \
            || ((res) == k_u64_div10 && fj_digit > k_u64_mod10)) ) {                \
            assert(!"overflow detected!");                                          \
            return std::numeric_limits<To>::max();                                  \
        }                                                                           \
        (res) = ((res) << 1) + ((res) << 3) + fj_digit;                             \
    } while ( false )

template<typename To>
__attribute__((noinline))
To checked_conv(const char *ptr, std::size_t len) noexcept {
    if ( __FJ__UNLIKELY(len == 0 || len > 20) ) {
        assert(!"invalid length");

        return To{};
    }

    const auto *str = reinterpret_cast<const std::uint8_t *>(ptr);
    std::uint64_t res = 0;
    switch ( len ) {
        case 20: FJ_ACCUMULATE(res, str[len - 20]); __FJ__FALLTHROUGH;
        case 19: FJ_ACCUMULATE(res, str[len - 19]); __FJ__FALLTHROUGH;
        case 18: FJ_ACCUMULATE(res, str[len - 18]); __FJ__FALLTHROUGH;
        case 17: FJ_ACCUMULATE(res, str[len - 17]); __FJ__FALLTHROUGH;
        case 16: FJ_ACCUMULATE(res, str[len - 16]); __FJ__FALLTHROUGH;
        case 15: FJ_ACCUMULATE(res, str[len - 15]); __FJ__FALLTHROUGH;
        case 14: FJ_ACCUMULATE(res, str[len - 14]); __FJ__FALLTHROUGH;
        case 13: FJ_ACCUMULATE(res, str[len - 13]); __FJ__FALLTHROUGH;
        case 12: FJ_ACCUMULATE(res, str[len - 12]); __FJ__FALLTHROUGH;
        case 11: FJ_ACCUMULATE(res, str[len - 11]); __FJ__FALLTHROUGH;
        case 10: FJ_ACCUMULATE(res, str[len - 10]); __FJ__FALLTHROUGH;
        case 9: FJ_ACCUMULATE(res, str[len - 9]); __FJ__FALLTHROUGH;
        case 8: FJ_ACCUMULATE(res, str[len - 8]); __FJ__FALLTHROUGH;
        case 7: FJ_ACCUMULATE(res, str[len - 7]); __FJ__FALLTHROUGH;
        case 6: FJ_ACCUMULATE(res, str[len - 6]); __FJ__FALLTHROUGH;
        case 5: FJ_ACCUMULATE(res, str[len - 5]); __FJ__FALLTHROUGH;
        case 4: FJ_ACCUMULATE(res, str[len - 4]); __FJ__FALLTHROUGH;
        case 3: FJ_ACCUMULATE(res, str[len - 3]); __FJ__FALLTHROUGH;
        case 2: FJ_ACCUMULATE(res, str[len - 2]); __FJ__FALLTHROUGH;
        case 1: FJ_ACCUMULATE(res, str[len - 1]); break;
        default: {
            assert(!"unreachable!");

            return To{};
        }
    }
    if ( res > static_cast<std::uint64_t>(std::numeric_limits<To>::max()) ) {
        assert(!"overflow detected!");

        return std::numeric_limits<To>::max();
    }

    return static_cast<To>(res);
}

#undef FJ_ACCUMULATE

template<typename To>
conv_fn<To> cloak(conv_fn<To> fn) {
    asm volatile("" : "+r"(fn));

    return fn;
}

template<typename To>
__attribute__((noinline))
std::uint64_t accumulate(
    conv_fn<To> fn
    ,const number *items
    ,std::size_t count
    ,int repeats)
{
    std::uint64_t sum = 0;
    for ( int rep = 0; rep < repeats; ++rep ) {
        for ( std::size_t i = 0; i < count; ++i ) {
            sum += static_cast<std::uint64_t>(fn(items[i].ptr, items[i].len));
        }
    }

    return sum;
}

std::uint64_t mix(std::uint64_t state) noexcept
{ return state * 6364136223846793005ull + 1u; }

void fill_digits(char *at, std::size_t len, std::uint64_t &state) {
    for ( std::size_t d = 0; d < len; ++d ) {
        state = mix(state);
        const auto digit = static_cast<unsigned>((state >> 33) % 10u);
        if ( d == 0 && len > 1 ) {
            at[d] = static_cast<char>('1' + digit % 9u);
        } else {
            at[d] = static_cast<char>('0' + digit);
        }
    }
}

std::vector<number> make_fixed(std::string &storage, std::size_t len) {
    storage.assign(k_numbers * len, '0');
    std::uint64_t state = 0x123456789abcdefull ^ (len << 17);
    std::vector<number> out;
    out.reserve(k_numbers);
    for ( std::size_t i = 0; i < k_numbers; ++i ) {
        char *at = &storage[i * len];
        fill_digits(at, len, state);
        out.push_back(number{at, len});
    }

    return out;
}

std::vector<number> make_mixed(std::string &storage, std::size_t max_len) {
    storage.assign(k_numbers * max_len, '0');
    std::uint64_t state = 0x9abcdef123456789ull ^ (max_len << 9);
    std::vector<number> out;
    out.reserve(k_numbers);
    for ( std::size_t i = 0; i < k_numbers; ++i ) {
        const std::size_t len = 1u + (i % max_len);
        char *at = &storage[i * max_len];
        fill_digits(at, len, state);
        out.push_back(number{at, len});
    }

    return out;
}

template<typename To>
double median_ns(conv_fn<To> fn, const std::vector<number> &items) {
    fn = cloak(fn);
    g_sink += accumulate(fn, items.data(), items.size(), k_warmup);
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(k_runs));
    const double scale = static_cast<double>(
        items.size() * static_cast<std::size_t>(k_repeats)
    );
    for ( int run = 0; run < k_runs; ++run ) {
        const auto t0 = std::chrono::steady_clock::now();
        g_sink += accumulate(fn, items.data(), items.size(), k_repeats);
        const auto t1 = std::chrono::steady_clock::now();
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        samples.push_back(static_cast<double>(ns) / scale);
    }
    std::sort(samples.begin(), samples.end());

    return samples[samples.size() / 2];
}

template<typename To>
bool bench_one(
    const char *type_name
    ,const char *kind
    ,const std::vector<number> &items)
{
    const auto plain_once = accumulate(
        cloak(&plain_conv<To>), items.data(), items.size(), 1
    );
    const auto checked_once = accumulate(
        cloak(&checked_conv<To>), items.data(), items.size(), 1
    );
    if ( plain_once != checked_once ) {
        std::cout << "checksum mismatch for " << type_name << ' ' << kind << '\n';

        return false;
    }
    g_sink += plain_once;

    const double plain_ns = median_ns<To>(&plain_conv<To>, items);
    const double checked_ns = median_ns<To>(&checked_conv<To>, items);
    const double delta = (checked_ns - plain_ns) / plain_ns * 100.0;
    std::cout
        << std::left << std::setw(6) << type_name
        << std::setw(12) << kind
        << std::right << std::fixed << std::setprecision(2)
        << std::setw(8) << plain_ns
        << std::setw(8) << checked_ns
        << std::setw(8) << delta << "%\n";

    return true;
}

template<typename To>
bool bench_lengths(const char *type_name, const std::size_t *lens, std::size_t nlens) {
    for ( std::size_t i = 0; i < nlens; ++i ) {
        std::string storage;
        const auto items = make_fixed(storage, lens[i]);
        char kind[16];
        ::snprintf(kind, sizeof(kind), "len=%u", static_cast<unsigned>(lens[i]));
        if ( !bench_one<To>(type_name, kind, items) ) {
            return false;
        }
    }
    std::string storage;
    const auto mixed = make_mixed(storage, lens[nlens - 1]);

    return bench_one<To>(type_name, "mixed", mixed);
}

template<typename To>
bool expect_same(const char *text, To expected) {
    const auto len = ::strlen(text);
    const To plain = plain_conv<To>(text, len);
    const To checked = checked_conv<To>(text, len);
    if ( plain != expected || checked != expected ) {
        std::cout << "mismatch \"" << text << "\"\n";

        return false;
    }

    return true;
}

bool expect_reject(const char *text, std::size_t len, std::uint64_t expected) {
    const auto got = checked_conv<std::uint64_t>(text, len);
    if ( got != expected ) {
        std::cout << "reject mismatch \"" << text << "\"\n";

        return false;
    }

    return true;
}

bool self_check() {
    if ( !expect_same<std::uint64_t>("0", 0) ) {
        return false;
    }
    if ( !expect_same<std::uint64_t>("7", 7) ) {
        return false;
    }
    if ( !expect_same<std::uint64_t>("42", 42) ) {
        return false;
    }
    if ( !expect_same<std::uint64_t>("0003", 3) ) {
        return false;
    }
    if ( !expect_same<std::uint32_t>("4294967295", 4294967295u) ) {
        return false;
    }
    if ( !expect_same<std::uint64_t>("4294967295", 4294967295ull) ) {
        return false;
    }
    if ( !expect_same<std::uint64_t>("18446744073709551615",
        std::numeric_limits<std::uint64_t>::max()) ) {
        return false;
    }
    if ( !expect_reject("", 0, 0) ) {
        return false;
    }
    if ( !expect_reject("12x", 3, 0) ) {
        return false;
    }
    if ( !expect_reject("111111111111111111111", 21, 0) ) {
        return false;
    }
    if ( !expect_reject(
        "18446744073709551616", 20, std::numeric_limits<std::uint64_t>::max()) ) {
        return false;
    }
    const auto narrowed = checked_conv<std::uint32_t>("4294967296", 10);
    if ( narrowed != std::numeric_limits<std::uint32_t>::max() ) {
        std::cout << "uint32 overflow mismatch\n";

        return false;
    }

    return true;
}

} // anon ns

int main() {
    std::cout
        << std::left << std::setw(6) << "type"
        << std::setw(12) << "input"
        << std::right << std::setw(8) << "plain"
        << std::setw(8) << "checked"
        << std::setw(9) << "delta"
        << "   ns/call, median of " << k_runs << "\n";
    if ( !self_check() ) {
        return EXIT_FAILURE;
    }

    const std::size_t u64_lens[] = {1u, 4u, 8u, 16u, 19u};
    const std::size_t u32_lens[] = {1u, 4u, 8u, 9u};
    if ( !bench_lengths<std::uint64_t>("u64", u64_lens, 5) ) {
        return EXIT_FAILURE;
    }
    if ( !bench_lengths<std::uint32_t>("u32", u32_lens, 4) ) {
        return EXIT_FAILURE;
    }
    std::cout << "sink " << g_sink << '\n';

    return EXIT_SUCCESS;
}
