/*
 * setup/tests/main_a_test.cpp
 *
 * Tier 3 integration tests for setup.dll via the MainA entry point.
 *
 * These tests load setup.dll dynamically and call MainA with various
 * command strings. They verify that the DLL loads, the export exists,
 * and common commands execute without crashing. No driver required.
 *
 * Output capture via --output is unreliable because setup.dll's
 * Trace() writes to the original stdout handle, not the redirected one.
 * These tests focus on load/call/crash safety rather than output content.
 *
 * See: setup/setup.cpp — MainA implementation
 */

#include "catch_amalgamated.hpp"
#include <windows.h>
#include <cstdio>
#include <string>

typedef int (CALLBACK *MainA_t)(const char *, const char *);

// Helper: find and load setup.dll, get MainA, call it, return exit code.
// Returns -1 if DLL load or proc resolution fails.
static int CallMainA(const char *pArgs)
{
    // Find setup.dll next to the test executable
    char dllPath[MAX_PATH];
    GetModuleFileNameA(NULL, dllPath, sizeof(dllPath));
    char *pLastSlash = strrchr(dllPath, '\\');
    if (pLastSlash) *(pLastSlash + 1) = 0;
    strcat(dllPath, "setup.dll");

    HMODULE hDll = LoadLibraryA(dllPath);
    if (!hDll) {
        FAIL("LoadLibrary(" << dllPath << ") failed: " << GetLastError());
        return -1;
    }

    auto pMainA = (MainA_t)GetProcAddress(hDll, "MainA");
    if (!pMainA) {
        FreeLibrary(hDll);
        FAIL("GetProcAddress(MainA) failed: " << GetLastError());
        return -1;
    }

    int result = pMainA("setup", pArgs);
    FreeLibrary(hDll);
    return result;
}

// ══════════════════════════════════════════════════════════════════
// DLL load and basic command safety
// ══════════════════════════════════════════════════════════════════

TEST_CASE("MainA help command loads DLL and returns success", "[integration][setup]")
{
    int result = CallMainA("--silent help");
    // Main returns 0 on successful help display
    REQUIRE(result == 0);
}

TEST_CASE("MainA list command loads DLL and runs without crashing", "[integration][setup]")
{
    int result = CallMainA("--silent --detail-prms list");
    // list may fail without driver installed — the key test is no crash
    REQUIRE((result == 0 || result == 1));
}

TEST_CASE("MainA busynames command loads DLL and runs without crashing", "[integration][setup]")
{
    int result = CallMainA("--silent busynames *");
    // busynames queries system ComDB which may fail without driver installed.
    // The key assertion is that the call completes without crashing.
    REQUIRE((result == 0 || result == 1));
}

TEST_CASE("MainA invalid command returns failure without crashing", "[integration][setup]")
{
    int result = CallMainA("--silent nonexistent_command");
    // Main returns 1 for invalid command
    REQUIRE(result == 1);
}

TEST_CASE("MainA handles quit command gracefully", "[integration][setup]")
{
    // "quit" immediately returns 0 without any interactive I/O
    int result = CallMainA("quit");
    REQUIRE(result == 0);
}

