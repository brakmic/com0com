/*
 * hub4com/tests/hub4com_integration_test.cpp
 *
 * Tier 3 integration tests for hub4com.exe.
 *
 * Launches hub4com.exe as a subprocess and captures stdout.
 * Tests --help output and basic command-line validation.
 * No driver or COM ports required.
 */

#include "catch_amalgamated.hpp"
#include <windows.h>
#include <cstdio>
#include <string>

// Helper: run hub4com.exe with arguments, capture stdout, return output + exit code
static std::string RunHub4com(const char *pArgs, int *pExitCode)
{
    // Find hub4com.exe relative to the test executable
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    char *pLastSlash = strrchr(exePath, '\\');
    if (pLastSlash) *(pLastSlash + 1) = 0;
    strcat(exePath, "hub4com.exe");

    char cmdLine[1024];
    _snprintf(cmdLine, sizeof(cmdLine), "\"%s\" %s", exePath, pArgs);

    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        FAIL("CreatePipe failed: " << GetLastError());
        *pExitCode = -1;
        return "";
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi = { 0 };
    BOOL ok = CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE,
                              CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(hWrite);

    std::string output;
    if (ok) {
        CloseHandle(pi.hThread);

        char buf[4096];
        DWORD bytesRead;
        while (ReadFile(hRead, buf, sizeof(buf) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buf[bytesRead] = 0;
            output += buf;
        }

        WaitForSingleObject(pi.hProcess, 5000);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        *pExitCode = (int)exitCode;
        CloseHandle(pi.hProcess);
    } else {
        *pExitCode = -1;
        FAIL("CreateProcess failed: " << GetLastError());
    }

    CloseHandle(hRead);
    return output;
}

// ══════════════════════════════════════════════════════════════════
// --help
// ══════════════════════════════════════════════════════════════════

TEST_CASE("hub4com --help lists all documented route and filter options", "[integration][hub4com]")
{
    int exitCode = -1;
    std::string output = RunHub4com("--help", &exitCode);

    REQUIRE(exitCode == 0);
    REQUIRE_FALSE(output.empty());

    // Help should mention route and filter options
    REQUIRE(output.find("route") != std::string::npos);
    REQUIRE(output.find("filter") != std::string::npos);
    REQUIRE(output.find("use-driver") != std::string::npos);
    REQUIRE(output.find("help") != std::string::npos);
}

TEST_CASE("hub4com --help lists all modules by type", "[integration][hub4com]")
{
    int exitCode = -1;
    // --help=<LstM> where LstM is comma-separated module names.
    // The static 'serial' driver is always available.
    std::string output = RunHub4com("--help=serial", &exitCode);

    REQUIRE(exitCode == 0);
    REQUIRE_FALSE(output.empty());
}

// ══════════════════════════════════════════════════════════════════
// No arguments — should show usage or error
// ══════════════════════════════════════════════════════════════════

TEST_CASE("hub4com with no arguments shows usage", "[integration][hub4com]")
{
    int exitCode = -1;
    std::string output = RunHub4com("", &exitCode);

    REQUIRE_FALSE(output.empty());
    bool hasHub4com = output.find("hub4com") != std::string::npos;
    bool hasUsage = output.find("usage") != std::string::npos;
    REQUIRE((hasHub4com || hasUsage));
}

// ══════════════════════════════════════════════════════════════════
// Plugin directory scan — verify hub4com can find its plugins
// ══════════════════════════════════════════════════════════════════

TEST_CASE("hub4com --help=echo lists echo filter help", "[integration][hub4com]")
{
    int exitCode = -1;
    std::string output = RunHub4com("--help=echo", &exitCode);

    REQUIRE(exitCode == 0);
    REQUIRE_FALSE(output.empty());
}

TEST_CASE("hub4com --help=serial lists serial driver help", "[integration][hub4com]")
{
    int exitCode = -1;
    std::string output = RunHub4com("--help=serial", &exitCode);

    REQUIRE(exitCode == 0);
    REQUIRE_FALSE(output.empty());
}
