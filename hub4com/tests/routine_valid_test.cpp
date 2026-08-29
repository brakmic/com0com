/*
 * hub4com/tests/routine_valid_test.cpp
 *
 * Tests for ROUTINE_IS_VALID and ITEM_IS_VALID macros (plugins_api.h).
 *
 * These macros perform bounds-checking on function pointer tables to enable
 * backward-compatible plugin API extension. A plugin struct has a `size` field
 * declaring its byte size. ROUTINE_IS_VALID checks whether a function pointer
 * field address falls within the struct bounds, protecting against version
 * mismatches.
 *
 * See: hub4com/plugins/plugins_api.h — ITEM_IS_VALID, ROUTINE_GET, ROUTINE_IS_VALID
 */

#include "catch_amalgamated.hpp"
#include <windows.h>

#include "../plugins/plugins_api.h"

// Test struct simulating a plugin routines table
struct TestRoutines {
    size_t size;
    int (*pFunc1)(void);
    int (*pFunc2)(int);
    int (*pFunc3)(int, int);
};

// ══════════════════════════════════════════════════════════════════
// ITEM_IS_VALID tests
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ITEM_IS_VALID returns true for first field when size is full struct", "[unit][routine]")
{
    TestRoutines tbl;
    tbl.size = sizeof(TestRoutines);
    REQUIRE(ITEM_IS_VALID(&tbl, pFunc1));
}

TEST_CASE("ITEM_IS_VALID returns true for last field when size is full struct", "[unit][routine]")
{
    TestRoutines tbl;
    tbl.size = sizeof(TestRoutines);
    REQUIRE(ITEM_IS_VALID(&tbl, pFunc3));
}

TEST_CASE("ITEM_IS_VALID returns false when size is too small for the field", "[unit][routine]")
{
    TestRoutines tbl;
    // Declare size as only enough for pFunc1, not pFunc2
    tbl.size = offsetof(TestRoutines, pFunc2);
    REQUIRE_FALSE(ITEM_IS_VALID(&tbl, pFunc2));
}

TEST_CASE("ITEM_IS_VALID returns false when size is zero", "[unit][routine]")
{
    TestRoutines tbl;
    tbl.size = 0;
    REQUIRE_FALSE(ITEM_IS_VALID(&tbl, pFunc1));
}

TEST_CASE("ITEM_IS_VALID returns false for field past declared size", "[unit][routine]")
{
    TestRoutines tbl;
    tbl.size = sizeof(size_t) + sizeof(void*); // size + pFunc1 only
    // pFunc3 is at offset 24 (size_t + two pointers on 64-bit)
    REQUIRE_FALSE(ITEM_IS_VALID(&tbl, pFunc3));
}

// ══════════════════════════════════════════════════════════════════
// ROUTINE_GET tests
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ROUTINE_GET returns pointer when field is valid", "[unit][routine]")
{
    TestRoutines tbl;
    tbl.size = sizeof(TestRoutines);
    tbl.pFunc2 = (int(*)(int))0x1234;
    REQUIRE(ROUTINE_GET(&tbl, pFunc2) == (int(*)(int))0x1234);
}

TEST_CASE("ROUTINE_GET returns NULL when field is out of bounds", "[unit][routine]")
{
    TestRoutines tbl;
    tbl.size = offsetof(TestRoutines, pFunc2);
    tbl.pFunc2 = (int(*)(int))0x1234;
    REQUIRE(ROUTINE_GET(&tbl, pFunc2) == nullptr);
}

// ══════════════════════════════════════════════════════════════════
// ROUTINE_IS_VALID tests
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ROUTINE_IS_VALID returns true for valid routine pointer", "[unit][routine]")
{
    TestRoutines tbl;
    tbl.size = sizeof(TestRoutines);
    REQUIRE(ROUTINE_IS_VALID(&tbl, pFunc1));
}

TEST_CASE("ROUTINE_IS_VALID returns false when routine out of bounds", "[unit][routine]")
{
    TestRoutines tbl;
    tbl.size = offsetof(TestRoutines, pFunc2);
    REQUIRE_FALSE(ROUTINE_IS_VALID(&tbl, pFunc2));
}

TEST_CASE("ROUTINE_IS_VALID returns false when size is zero for any field", "[unit][routine]")
{
    TestRoutines tbl;
    tbl.size = 0;
    REQUIRE_FALSE(ROUTINE_IS_VALID(&tbl, pFunc1));
}

// ══════════════════════════════════════════════════════════════════
// Boundary: exactly-at-boundary
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ITEM_IS_VALID returns false when field starts at declared size", "[unit][routine]")
{
    // ITEM_IS_VALID checks: (addr of (field+1)) <= (base + size)
    // If field starts exactly at base+size, then field+1 > base+size → false
    TestRoutines tbl;
    tbl.size = sizeof(size_t); // only enough for the `size` field itself
    REQUIRE_FALSE(ITEM_IS_VALID(&tbl, pFunc1));
}
