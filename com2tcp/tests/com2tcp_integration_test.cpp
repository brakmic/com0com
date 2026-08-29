/*
 * com2tcp/tests/com2tcp_integration_test.cpp
 *
 * Tier 3 integration tests for com2tcp.exe.
 *
 * Launches com2tcp.exe as a subprocess and captures stdout/stderr.
 * Tests --help output and basic command-line validation.
 * No driver or COM ports required.
 */

#include "catch_amalgamated.hpp"
#include <windows.h>
#include <cstdio>
#include <string>

// Helper: run com2tcp.exe with arguments, capture stdout, return output + exit code
static std::string RunCom2tcp(const char *pArgs, int *pExitCode)
{
    // Find com2tcp.exe relative to the test executable
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    char *pLastSlash = strrchr(exePath, '\\');
    if (pLastSlash) *(pLastSlash + 1) = 0;
    strcat(exePath, "com2tcp.exe");

    // Build full command line
    char cmdLine[1024];
    _snprintf(cmdLine, sizeof(cmdLine), "\"%s\" %s", exePath, pArgs);

    // Create pipe for stdout
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

TEST_CASE("com2tcp --help lists all documented options", "[integration][com2tcp]")
{
    int exitCode = -1;
    std::string output = RunCom2tcp("--help", &exitCode);

    REQUIRE(exitCode == 1);
    REQUIRE_FALSE(output.empty());

    // Help text should mention key options from the documentation
    REQUIRE(output.find("telnet") != std::string::npos);
    REQUIRE(output.find("baud") != std::string::npos);
    REQUIRE(output.find("data") != std::string::npos);
    REQUIRE(output.find("parity") != std::string::npos);
    REQUIRE(output.find("stop") != std::string::npos);
    REQUIRE(output.find("ignore-dsr") != std::string::npos);
    REQUIRE(output.find("connect-dtr") != std::string::npos);
    REQUIRE(output.find("help") != std::string::npos);
}

// ══════════════════════════════════════════════════════════════════
// No arguments — should show usage
// ══════════════════════════════════════════════════════════════════

TEST_CASE("com2tcp with no arguments shows usage", "[integration][com2tcp]")
{
    int exitCode = -1;
    std::string output = RunCom2tcp("", &exitCode);

    // com2tcp returns 1 when no arguments (shows usage then exits)
    REQUIRE(exitCode == 1);
    REQUIRE_FALSE(output.empty());
}

// ══════════════════════════════════════════════════════════════════
// Invalid port — should report error
// ══════════════════════════════════════════════════════════════════

TEST_CASE("com2tcp with nonexistent port reports error", "[integration][com2tcp]")
{
    int exitCode = -1;
    std::string output = RunCom2tcp("\\\\.\\COM_NONEXISTENT 9999", &exitCode);

    // Should fail to open the nonexistent port but not crash
    REQUIRE(exitCode == 2);
    REQUIRE_FALSE(output.empty());
}
