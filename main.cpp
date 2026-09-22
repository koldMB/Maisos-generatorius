#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdint>
#include <windows.h>

/*
 * info man
 * https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 * https://en.wikipedia.org/wiki/Hash_function
 * https://cryptii.com/ lyginimui su kitais hash algo
 * https://www.geeksforgeeks.org/cpp/cpp-bitwise-operators/
 */

using std::string;

struct hash { uint32_t w[8]; };

static uint32_t mix32(uint32_t x) {
    x ^= x >> 7;
    x *= 2146121517;
    x ^= x >> 11;
    x *= 2220872331;
    x ^= x >> 17;
    return x;
}

hash myHash(const uint8_t* data, size_t len) {
    // prime skaiciai is http://compoasso.free.fr/primelistweb/page/prime/liste_online_en.php nuo 1530000
    uint32_t s[8] = {
        1530559,
        1531487,
        1532611,
        1532723,
        1532903,
        1533083,
        1533211,
        1533397
    };

    s[0] ^= (uint32_t)len;
    s[1] ^= (uint32_t)(len >> 32);

    // suspaudimas norint pasiekt reikiama ilgi
    for (size_t i = 0; i < len; ++i) {
        uint32_t b = data[i];
        int slot = i % 8;

        s[slot] = mix32(s[slot] ^ (b + (uint32_t)i));
        s[(slot + 3) & 7] += b;
        s[(slot + 5) & 7] ^= mix32(b + (uint32_t)(i * 31));
    }

    // maiša
    for (int r = 0; r < 8; ++r) {
        for (int i = 0; i < 8; ++i) {
            s[i] = mix32(s[i] + s[(i + 1) & 7]);
            s[(i + 3) & 7] ^= s[i];
        }
    }

    hash out;
    for (int i = 0; i < 8; ++i) out.w[i] = s[i];
    return out;
}

// 256 į 64 hex
string toHex(const hash& h) {
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) os << std::setw(8) << h.w[i];
    return os.str();
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    string data;
    if (argc > 1) {
        std::ifstream f(argv[1], std::ios::binary);
        if (f) { std::ostringstream ss; ss << f.rdbuf(); data = ss.str(); }
        else   { data = argv[1]; }
    } else {
        std::ostringstream ss; ss << std::cin.rdbuf(); data = ss.str();
    }
    hash h = myHash(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    std::cout << toHex(h) << "\n";
}