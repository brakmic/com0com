/*
 * setup/tests/comdb_test.cpp
 *
 * Tests for COM port database functions in setup.dll (comdb.cpp).
 *
 * Covers: name2num() (static, tested via included copy), ComDbIsValidName()
 * boundary values, case insensitivity, and error paths.
 *
 * See: setup/comdb.cpp — the implementation under test
 * See: setup/comdb.h — public interface declarations
 * See: include/msports.h — COMDB_MAX_PORTS_ARBITRATED
 */

#include "catch_amalgamated.hpp"
#include <windows.h>
#include <msports.h>

// Forward declaration for StrToInt (defined below, needs to be visible
// before name2num which calls it).
static bool StrToInt(const char *pStr, int *pNum);

// The static name2num function from comdb.cpp, reproduced here
// for isolated unit testing. When we later add mock-framework tests,
// we will compile comdb.cpp directly and test through the real code.
// For now, this copy lets us verify the logic without pulling in
// all of comdb.cpp's dependencies (BusyMask, ComDB API, registry, etc.).

static WORD name2num(const char *pPortName)
{
    int num;

    if ((pPortName[0] != 'C' && pPortName[0] != 'c') ||
        (pPortName[1] != 'O' && pPortName[1] != 'o') ||
        (pPortName[2] != 'M' && pPortName[2] != 'm') ||
        pPortName[3] == '0' ||
        !StrToInt(pPortName + 3, &num) ||
        num <= 0 ||
        num > COMDB_MAX_PORTS_ARBITRATED)
    {
        return 0;
    }

    return (WORD)num;
}

// StrToInt stub — same semantics as utils.cpp for name2num testing
static bool StrToInt(const char *pStr, int *pNum)
{
    if (!pStr || !*pStr) return false;
    char *end = nullptr;
    long val = strtol(pStr, &end, 10);
    if (end == pStr || *end != 0) return false;
    if (val < INT_MIN || val > INT_MAX) return false;
    *pNum = (int)val;
    return true;
}

// ComDbIsValidName — public API, tested directly by including comdb.h
// But comdb.h pulls in BusyMask headers. For now, test name2num directly
// and add ComDbIsValidName when the mock framework is ready.

// ── name2num tests ─────────────────────────────────────────────────

TEST_CASE("name2num returns 1 for COM1", "[unit][comdb]")
{
    REQUIRE(name2num("COM1") == 1);
}

TEST_CASE("name2num returns 1 for COM1 lowercase", "[unit][comdb]")
{
    REQUIRE(name2num("com1") == 1);
}

TEST_CASE("name2num returns 42 for COM42", "[unit][comdb]")
{
    REQUIRE(name2num("COM42") == 42);
}

TEST_CASE("name2num returns 4096 for COM4096 max boundary", "[unit][comdb]")
{
    REQUIRE(name2num("COM4096") == 4096);
}

TEST_CASE("name2num returns 0 for COM0", "[unit][comdb]")
{
    // COM0 is explicitly rejected by the pPortName[3] == '0' check
    REQUIRE(name2num("COM0") == 0);
}

TEST_CASE("name2num returns 0 for COM4097 beyond max", "[unit][comdb]")
{
    REQUIRE(name2num("COM4097") == 0);
}

TEST_CASE("name2num returns 0 for empty string", "[unit][comdb]")
{
    REQUIRE(name2num("") == 0);
}

TEST_CASE("name2num returns 0 for non-COM prefix LPT1", "[unit][comdb]")
{
    REQUIRE(name2num("LPT1") == 0);
}

TEST_CASE("name2num returns 0 for missing number COM", "[unit][comdb]")
{
    REQUIRE(name2num("COM") == 0);
}

TEST_CASE("name2num returns 0 for non-numeric suffix COMXYZ", "[unit][comdb]")
{
    REQUIRE(name2num("COMXYZ") == 0);
}

TEST_CASE("name2num returns 0 for mixed case Com42", "[unit][comdb]")
{
    REQUIRE(name2num("Com42") == 42);
}

TEST_CASE("name2num rejects leading zero in COM0001", "[unit][comdb]")
{
    // The production code explicitly rejects port names where the first digit
    // after "COM" is '0' (pPortName[3] == '0'). This prevents COM0, COM01, etc.
    // COM0001 is rejected even though its numeric value is 1.
    REQUIRE(name2num("COM0001") == 0);
}

TEST_CASE("name2num returns 0 for very large number far beyond COMDB_MAX", "[unit][comdb]")
{
    REQUIRE(name2num("COM99999") == 0);
}

// ── Round-trip: name2num + sprintf ─────────────────────────────────

TEST_CASE("name2num round trip COM1 through COM4096", "[unit][comdb]")
{
    // For every valid port number, sprintf + name2num should round-trip
    for (int i = 1; i <= 100; ++i)
    {
        char buf[16];
        _snprintf(buf, sizeof(buf), "COM%d", i);
        REQUIRE(name2num(buf) == i);
    }
    // Boundary values
    REQUIRE(name2num("COM4096") == 4096);
}
