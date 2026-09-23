/*
 * myhash.cpp — a strengthened 256-bit non-cryptographic hash.
 *
 * See readme.md for the full list of changes relative to the previous
 * revision and for a description of the algorithm.
 *
 * Build (C++17):
 *     c++ -O2 -std=c++17 -o myhash myhash.cpp
 */

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#  include <io.h>
#  include <fcntl.h>
#  include <windows.h>
#endif

// ============================================================================
//  Public types
// ============================================================================

struct hash256 {
    uint32_t w[8];   // 8 * 32 = 256 bits
};

// ============================================================================
//  Primitives
// ============================================================================

namespace {

// 32-bit rotate-left.  The mask keeps n == 0 well-defined.
inline uint32_t rotl32(uint32_t x, int n) noexcept {
    return (x << n) | (x >> ((32 - n) & 31));
}

// Pelle Evensen's "lowbias32" finalizer.
// Source: https://mostlymangling.blogspot.com/2019/12/stronger-better-morer-moremur-better.html
// Replaces the previous ad-hoc mix32() which used non-standard shift and
// multiplier constants.  lowbias32 has excellent avalanche and a well-
// documented track record in SMHasher-style tests.
inline uint32_t mix32(uint32_t x) noexcept {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

// Well-known constants, each documented at its source.
constexpr uint32_t GOLDEN = 0x9e3779b9U;   // floor(2^32 / phi), Weyl sequence
constexpr uint32_t PRIME1 = 0x85ebca6bU;   // MurmurHash3
constexpr uint32_t PRIME2 = 0xc2b2ae35U;   // MurmurHash3

// Domain-separated IV.  Derived from the SHA-256 IV (fractional parts of
// the square roots of the first eight primes), XOR'd with a version tag in
// the first word so that digests produced by this revision are not
// interchangeable with the previous, weaker revision.
constexpr uint32_t IV[8] = {
    0x6a09e667U ^ 0x01U,   // version tag: rev 2
    0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
    0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
};

} // namespace

// ============================================================================
//  Core hash function
// ============================================================================

hash256 myHash(const uint8_t* data, size_t len) noexcept {
    uint32_t s[8];

    // ---- 1. Initialise the state ------------------------------------------
    for (int i = 0; i < 8; ++i)
        s[i] = IV[i] ^ (GOLDEN * static_cast<uint32_t>(i + 1));

    // ---- 2. Absorb length, in both bytes and bits -------------------------
    const uint64_t bits = static_cast<uint64_t>(len) * 8u;
    s[0] ^= static_cast<uint32_t>(len);
    s[1] ^= static_cast<uint32_t>(static_cast<uint64_t>(len) >> 32);
    s[2] ^= static_cast<uint32_t>(bits);
    s[3] ^= static_cast<uint32_t>(bits >> 32);

    // ---- 3. Absorb the message -------------------------------------------
    // Each byte perturbs three different words with three different
    // combining operations (non-linear, additive, XOR) so that influence
    // spreads quickly.
    for (size_t i = 0; i < len; ++i) {
        const uint32_t b    = data[i];
        const uint32_t idx  = static_cast<uint32_t>(i);
        const int      slot = static_cast<int>(i & 7u);

        s[slot]              = mix32(s[slot] ^ (b + GOLDEN * (idx + 1u)));
        s[(slot + 3) & 7]   += b + PRIME1 * idx;
        s[(slot + 5) & 7]   ^= mix32(b + PRIME2 * (idx + 1u));
    }

    // ---- 4. Sponge terminator (0x80 byte) --------------------------------
    // Guarantees that "abc" and "abc\0" reach different internal states,
    // even ignoring the length field absorbed in step 2.
    {
        const uint32_t idx  = static_cast<uint32_t>(len);
        const int      slot = static_cast<int>(len & 7u);
        constexpr uint32_t pad = 0x80u;

        s[slot]              = mix32(s[slot] ^ (pad + GOLDEN * (idx + 1u)));
        s[(slot + 3) & 7]   += pad + PRIME1 * idx;
        s[(slot + 5) & 7]   ^= mix32(pad + PRIME2 * (idx + 1u));
    }

    // ---- 5. Finalize ------------------------------------------------------
    // 16 rounds (was 8).  Each round mixes every word with its neighbour,
    // then the whole state is rotated by one word so that the fixed
    // neighbour pattern does not create weak word pairs.
    for (int r = 0; r < 16; ++r) {
        const uint32_t rc = GOLDEN * static_cast<uint32_t>(r + 1);

        for (int i = 0; i < 8; ++i) {
            s[i]            = mix32(s[i] + s[(i + 1) & 7] + rc);
            s[(i + 3) & 7] ^= s[i];
            s[(i + 5) & 7] += rotl32(s[i], (i * 3 + 5) & 31);
        }

        const uint32_t t = s[0];
        for (int i = 0; i < 7; ++i) s[i] = s[i + 1];
        s[7] = t;
    }

    hash256 out{};
    for (int i = 0; i < 8; ++i) out.w[i] = s[i];
    return out;
}

// ============================================================================
//  I/O helpers
// ============================================================================

static std::string toHex(const hash256& h) {
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i)
        os << std::setw(8) << h.w[i];
    return os.str();
}

static bool read_file_binary(const char* path, std::string& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;

    const std::streamsize n = f.tellg();
    if (n < 0) return false;

    f.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(n));
    if (n > 0) {
        f.read(&out[0], n);
        if (!f) return false;
    }
    return true;
}

static bool read_stdin_binary(std::string& out) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    if (std::cin.bad()) return false;
    out = ss.str();
    return true;
}

static void print_usage(const char* prog) {
    std::cout <<
        "Usage: " << prog << " [options] [<string>]\n"
        "\n"
        "Compute the 256-bit myhash digest of the input.\n"
        "\n"
        "Options:\n"
        "  -f, --file <path>    hash the contents of <path> (binary-safe)\n"
        "  -s, --string <text>  hash the literal string <text>\n"
        "  -h, --help           show this help and exit\n"
        "      --version        show version information and exit\n"
        "\n"
        "With no arguments, reads binary data from standard input.\n"
        "A bare positional argument is treated as a file path if it can be\n"
        "opened, otherwise as a literal string (legacy behaviour).\n";
}

// ============================================================================
//  Entry point
// ============================================================================

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::string data;

    if (argc >= 2) {
        const char* a1 = argv[1];

        if (std::strcmp(a1, "-h") == 0 || std::strcmp(a1, "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (std::strcmp(a1, "--version") == 0) {
            std::cout << "myhash 2.0\n";
            return 0;
        }
        if (std::strcmp(a1, "-f") == 0 || std::strcmp(a1, "--file") == 0) {
            if (argc < 3) { print_usage(argv[0]); return 2; }
            if (!read_file_binary(argv[2], data)) {
                std::cerr << "myhash: cannot open " << argv[2] << "\n";
                return 1;
            }
        }
        else if (std::strcmp(a1, "-s") == 0 || std::strcmp(a1, "--string") == 0) {
            if (argc < 3) { print_usage(argv[0]); return 2; }
            data = argv[2];
        }
        else {
            // Legacy behaviour: try as a file path first, else literal string.
            if (!read_file_binary(a1, data))
                data = a1;
        }
    }
    else {
        if (!read_stdin_binary(data)) {
            std::cerr << "myhash: failed to read stdin\n";
            return 1;
        }
    }

    const hash256 h = myHash(
        reinterpret_cast<const uint8_t*>(data.data()), data.size());
    std::cout << toHex(h) << "\n";
    return 0;
}