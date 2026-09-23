#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdint>
#include <cstring>
#include <windows.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

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

    // Fix: guard the >> 32 so it is not UB on 32-bit size_t.
    s[0] ^= (uint32_t)len;
    s[1] ^= (uint32_t)((uint64_t)len >> 32);

    // suspaudimas norint pasiekt reikiama ilgi
    for (size_t i = 0; i < len; ++i) {
        uint32_t b = data[i];
        int slot = i % 8;

        s[slot] = mix32(s[slot] ^ (b + (uint32_t)i));
        s[(slot + 3) & 7] += b;
        s[(slot + 5) & 7] ^= mix32(b + (uint32_t)(i * 31));
    }

    // maisa
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

// 256 i 64 hex
string toHex(const hash& h) {
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) os << std::setw(8) << h.w[i];
    return os.str();
}

// Read the entire contents of a binary file. Returns false on open failure.
static bool read_file_binary(const char* path, string& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    std::streamsize n = f.tellg();
    if (n < 0) return false;
    f.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(n));
    if (n > 0) {
        f.read(out.data(), n);
        if (!f) return false;
    }
    return true;
}

// Read all of stdin in binary mode
static bool read_stdin_binary(string& out) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    if (std::cin.bad()) return false;
    out = ss.str();
    return true;
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    string data;

    // Aiškios vėliavėlės:
    //   -f <kelias>   skaityti baitus iš failo (saugus dvejetainis režimas, leidžiami NUL baitai)
    //   -s <tekstas>  skaičiuoti nurodytos eilutės maišos reikšmę (per argv NUL baitų perduoti negalima)
    //   (be argumentų) skaityti iš standartinės įvesties (stdin) dvejetainiu režimu
    // Senoji elgsena: argv[1] laikomas failu, jei jį pavyksta atidaryti;
    //                 priešingu atveju – tiesiogine teksto eilute.
    if (argc >= 3 && std::strcmp(argv[1], "-f") == 0) {
        if (!read_file_binary(argv[2], data)) {
            std::cerr << "cannot open " << argv[2] << "\n";
            return 1;
        }
    }
    else if (argc >= 3 && std::strcmp(argv[1], "-s") == 0) {
        data = argv[2];
    }
    else if (argc == 1) {
        if (!read_stdin_binary(data)) {
            std::cerr << "failed to read stdin\n";
            return 1;
        }
    }
    else {
        if (!read_file_binary(argv[1], data)) {
            data = argv[1];
        }
    }

    hash h = myHash(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    std::cout << toHex(h) << "\n";
    return 0;
}