// test_blockchain.cpp
// Compile:  cl /std:c++17 /EHsc test_blockchain.cpp
//      or:  g++ -std=c++17 -O2 test.cpp -o test.exe
//
// Runs C:\Users\Matas\Documents\programavimas\c++\blockchain\cmake-build-debug\blockchain.exe
// and checks that it is binary-safe on file, argv, and stdin inputs.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

static const wchar_t* kExePath =
    L"C:\\Users\\Matas\\Documents\\programavimas\\c++\\blockchain\\cmake-build-debug\\blockchain.exe";

// ---------------------------------------------------------------------------
// Test bookkeeping
// ---------------------------------------------------------------------------

static int g_pass = 0;
static int g_fail = 0;

static void check(const std::string& label, bool ok, const std::string& detail = "") {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label;
    if (!detail.empty()) std::cout << "  -- " << detail;
    std::cout << "\n";
    if (ok) ++g_pass; else ++g_fail;
}

// ---------------------------------------------------------------------------
// Process runner: spawn exe with given args and optional stdin bytes,
// capture stdout + stderr, return exit code.
// ---------------------------------------------------------------------------

struct RunResult {
    DWORD  exit_code = 0;
    std::string out;
    std::string err;
    bool   launched = false;
    std::string launch_error;
};

static std::wstring widen_ascii(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

// Escape an argument for the Windows command line (argv[0] rules).
// Wraps in quotes and backslash-escapes any embedded quotes.
static std::wstring quote_arg(const std::wstring& arg) {
    std::wstring out = L"\"";
    size_t backslashes = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') {
            ++backslashes;
        } else if (c == L'"') {
            out.append(backslashes * 2 + 1, L'\\');
            backslashes = 0;
            out.push_back(L'"');
        } else {
            out.append(backslashes, L'\\');
            backslashes = 0;
            out.push_back(c);
        }
    }
    out.append(backslashes * 2, L'\\');
    out.push_back(L'"');
    return out;
}

static RunResult run_exe(const std::vector<std::string>& args,
                         const std::string& stdin_bytes = std::string()) {
    RunResult r;

    // Build command line:  "exe" "arg1" "arg2" ...
    std::wstring cmd = quote_arg(kExePath);
    for (const auto& a : args) {
        cmd.push_back(L' ');
        cmd.append(quote_arg(widen_ascii(a)));
    }

    // Two pipes: one for child's stdin (we write), one for child's stdout+stderr.
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE in_r = nullptr, in_w = nullptr;
    HANDLE out_r = nullptr, out_w = nullptr;

    if (!CreatePipe(&in_r, &in_w, &sa, 0)) { r.launch_error = "CreatePipe(stdin)"; return r; }
    if (!SetHandleInformation(in_w, HANDLE_FLAG_INHERIT, 0)) { r.launch_error = "SetHandleInformation(in_w)"; return r; }
    if (!CreatePipe(&out_r, &out_w, &sa, 0)) { r.launch_error = "CreatePipe(stdout)"; return r; }
    if (!SetHandleInformation(out_r, HANDLE_FLAG_INHERIT, 0)) { r.launch_error = "SetHandleInformation(out_r)"; return r; }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = in_r;
    si.hStdOutput = out_w;
    si.hStdError  = out_w;   // merge stderr into stdout for simplicity

    PROCESS_INFORMATION pi{};

    std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back(L'\0');

    BOOL ok = CreateProcessW(
        nullptr, cmd_buf.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    // Parent no longer needs the child-side ends.
    CloseHandle(in_r);
    CloseHandle(out_w);

    if (!ok) {
        r.launch_error = "CreateProcess failed, GetLastError=" + std::to_string(GetLastError());
        CloseHandle(in_w);
        CloseHandle(out_r);
        return r;
    }

    // Write stdin, then close the write end so the child sees EOF.
    if (!stdin_bytes.empty()) {
        DWORD written = 0;
        const char* p = stdin_bytes.data();
        size_t remaining = stdin_bytes.size();
        while (remaining > 0) {
            DWORD chunk = (DWORD)((remaining > 0x10000) ? 0x10000 : remaining);
            if (!WriteFile(in_w, p, chunk, &written, nullptr) || written == 0) break;
            p += written;
            remaining -= written;
        }
    }
    CloseHandle(in_w);

    // Read all stdout.
    char buf[4096];
    DWORD read_n = 0;
    while (ReadFile(out_r, buf, sizeof(buf), &read_n, nullptr) && read_n > 0) {
        r.out.append(buf, read_n);
    }
    CloseHandle(out_r);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    r.exit_code = code;
    r.launched = true;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return r;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::string write_temp_file(const std::string& bytes) {
    char tmpdir[MAX_PATH];
    GetTempPathA(MAX_PATH, tmpdir);
    char tmppath[MAX_PATH];
    GetTempFileNameA(tmpdir, "btc", 0, tmppath);

    std::ofstream f(tmppath, std::ios::binary | std::ios::trunc);
    f.write(bytes.data(), (std::streamsize)bytes.size());
    f.close();
    return std::string(tmppath);
}

static std::string digest_of_file_bytes(const std::string& bytes) {
    std::string path = write_temp_file(bytes);
    RunResult r = run_exe({"-f", path});
    DeleteFileA(path.c_str());
    if (!r.launched || r.exit_code != 0) return "";
    return trim(r.out);
}

// Count nibbles that differ between two equal-length hex strings.
static int hex_nibble_diff(const std::string& a, const std::string& b) {
    int d = 0;
    size_t n = (a.size() < b.size()) ? a.size() : b.size();
    for (size_t i = 0; i < n; ++i) if (a[i] != b[i]) ++d;
    d += (int)((a.size() > n) ? a.size() - n : 0);
    d += (int)((b.size() > n) ? b.size() - n : 0);
    return d;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 0) existence check
    DWORD attrs = GetFileAttributesW(kExePath);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        std::wcerr << L"ERROR: executable not found at " << kExePath << L"\n";
        return 2;
    }

    std::wcout << L"Testing: " << kExePath << L"\n\n";

    // 1) -s "abc"
    {
        RunResult r = run_exe({"-s", "abc"});
        if (!r.launched) { check("-s 'abc' launches", false, r.launch_error); }
        else {
            check("-s 'abc' exits 0", r.exit_code == 0,
                  "rc=" + std::to_string(r.exit_code) + " out=" + trim(r.out));
            check("-s 'abc' returns 64 hex chars", trim(r.out).size() == 64,
                  "got '" + trim(r.out) + "'");
        }
    }

    // 2) file with NUL bytes
    const std::string payload_nul("a\0b\0c", 5);
    std::string d_nul = digest_of_file_bytes(payload_nul);
    check("file with NUL bytes -> 64 hex chars", d_nul.size() == 64, d_nul);

    // 3) NUL-containing input must differ from truncated input
    std::string d_just_a = digest_of_file_bytes("a");
    if (!d_nul.empty() && !d_just_a.empty()) {
        check("NUL-containing input != truncated input", d_nul != d_just_a,
              d_nul + " vs " + d_just_a);
    }

    // 4) stdin binary safety: same bytes, same digest
    {
        RunResult r = run_exe({}, payload_nul);
        if (!r.launched) check("stdin launch", false, r.launch_error);
        else {
            std::string out = trim(r.out);
            check("stdin accepts NUL bytes (rc==0)", r.exit_code == 0,
                  "rc=" + std::to_string(r.exit_code));
            check("stdin digest == -f digest", out == d_nul,
                  "stdin=" + out + " file=" + d_nul);
        }
    }

    // 5) Ctrl-Z (0x1A) in stdin should not terminate
    {
        const std::string payload_ctrlz("abc\x1a" "def", 7);
        std::string d_file = digest_of_file_bytes(payload_ctrlz);
        RunResult r = run_exe({}, payload_ctrlz);
        if (r.launched) {
            check("0x1A in stdin is not EOF", trim(r.out) == d_file,
                  "stdin=" + trim(r.out) + " file=" + d_file);
        }
    }

    // 6) CRLF vs LF
    {
        std::string d_crlf = digest_of_file_bytes("a\r\nb");
        std::string d_lf   = digest_of_file_bytes("a\nb");
        check("CRLF and LF hash differently", d_crlf != d_lf,
              "crlf=" + d_crlf + " lf=" + d_lf);
    }

    // 7) determinism
    {
        std::string d1 = digest_of_file_bytes("hello world");
        std::string d2 = digest_of_file_bytes("hello world");
        check("same input -> same digest", !d1.empty() && d1 == d2, d1);
    }

    // 8) avalanche (rough)
    {
        std::string d_a = digest_of_file_bytes("a");
        std::string d_b = digest_of_file_bytes("b");
        int diff = hex_nibble_diff(d_a, d_b);
        check("1-byte change alters many output nibbles", diff >= 16,
              std::to_string(diff) + "/64 nibbles differ");
    }

    // 9) long argument via -s (100 a's)
    {
        std::string long_arg(100, 'a');
        RunResult r = run_exe({"-s", long_arg});
        if (r.launched) {
            check("100-byte argument via -s", r.exit_code == 0 && trim(r.out).size() == 64,
                  trim(r.out));
        }
    }

    std::cout << "\n" << g_pass << " passed, " << g_fail << " failed.\n";
    return g_fail == 0 ? 0 : 1;
}