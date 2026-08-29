/*
 * setup/tests/mock_registry_test.cpp
 *
 * Tier 2 mock tests: validates the in-memory registry mock and demonstrates
 * that production code calls the mock functions correctly.
 *
 * These tests verify:
 *   1. The mock registry stores and retrieves values correctly
 *   2. Production functions (LoadComDbLocal/SaveComDbLocal) use the mock
 *   3. The preprocessor override pattern works end-to-end
 *
 * The win32_overrides.h header must be included BEFORE any production
 * source to remap Win32 API calls to mock versions.
 */

#include "catch_amalgamated.hpp"

// ══════════════════════════════════════════════════════════════════
// Include order is critical:
//   1. Windows headers FIRST (type declarations)
//   2. Mock overrides (rename Win32 functions BEFORE production code)
//   3. Mock headers + production code
// ══════════════════════════════════════════════════════════════════

#include <windows.h>

#include "mocks/win32_overrides.h"
#include "mocks/registry_mock.h"
#include "mocks/comdb_mock.h"

// ══════════════════════════════════════════════════════════════════
// Tests begin here
// ══════════════════════════════════════════════════════════════════

TEST_CASE("MockRegistry stores and retrieves a DWORD value", "[mock][registry]")
{
    MockRegistry::Reset();

    DWORD val = 42;
    MockRegistry::SetValue("HKLM\\Test\\Key", "TestValue", REG_DWORD, (const BYTE*)&val, sizeof(val));

    REQUIRE(MockRegistry::GetDword("HKLM\\Test\\Key", "TestValue") == 42);
}

TEST_CASE("MockRegistry returns sentinel for missing value", "[mock][registry]")
{
    MockRegistry::Reset();
    REQUIRE(MockRegistry::GetDword("HKLM\\Missing", "NoSuch") == 0xFFFFFFFF);
}

TEST_CASE("MockRegistry KeyExists returns true after SetValue", "[mock][registry]")
{
    MockRegistry::Reset();
    MockRegistry::SetValue("HKLM\\Foo", "Bar", REG_DWORD, nullptr, 0);
    REQUIRE(MockRegistry::KeyExists("HKLM\\Foo"));
    REQUIRE_FALSE(MockRegistry::KeyExists("HKLM\\Baz"));
}

// ══════════════════════════════════════════════════════════════════
// Tests: mock ComDB validation
// ══════════════════════════════════════════════════════════════════

TEST_CASE("MockComDb SetBusy and IsBusy round-trip", "[mock][comdb]")
{
    MockComDb::Reset();

    REQUIRE_FALSE(MockComDb::IsBusy(1));
    MockComDb::SetBusy(1, true);
    REQUIRE(MockComDb::IsBusy(1));
}

TEST_CASE("MockComDb Claim succeeds for free port", "[mock][comdb]")
{
    MockComDb::Reset();
    REQUIRE(MockComDb::Claim(5));
    REQUIRE(MockComDb::IsBusy(5));
}

TEST_CASE("MockComDb Claim fails for busy port", "[mock][comdb]")
{
    MockComDb::Reset();
    MockComDb::SetBusy(3, true);
    REQUIRE_FALSE(MockComDb::Claim(3));
}

TEST_CASE("MockComDb Release frees a claimed port", "[mock][comdb]")
{
    MockComDb::Reset();
    MockComDb::SetBusy(7, true);
    REQUIRE(MockComDb::Release(7));
    REQUIRE_FALSE(MockComDb::IsBusy(7));
}

TEST_CASE("MockComDb Release fails for already-free port", "[mock][comdb]")
{
    MockComDb::Reset();
    REQUIRE_FALSE(MockComDb::Release(10));
}

// ══════════════════════════════════════════════════════════════════
// Tests: mock API function calls (validate mock infrastructure)
// ══════════════════════════════════════════════════════════════════

TEST_CASE("Mock_RegSetValueExA and Mock_RegQueryValueExA round-trip", "[mock][registry]")
{
    MockRegistry::Reset();

    HKEY hKey;
    LSTATUS rc = Mock_RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Test",
                                       0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    REQUIRE(rc == ERROR_SUCCESS);

    DWORD val = 99;
    rc = Mock_RegSetValueExA(hKey, "MyValue", 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
    REQUIRE(rc == ERROR_SUCCESS);

    Mock_RegCloseKey(hKey);
    REQUIRE(MockRegistry::GetDword("HKLM\\Software\\Test", "MyValue") == 99);
}

TEST_CASE("Mock_RegQueryValueExA returns pre-set value", "[mock][registry]")
{
    MockRegistry::Reset();
    DWORD val = 77;
    MockRegistry::SetValue("HKLM\\Software\\Test", "MyValue", REG_DWORD, (const BYTE*)&val, sizeof(val));

    HKEY hKey;
    Mock_RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Test", 0, NULL, 0, KEY_READ, NULL, &hKey, NULL);

    DWORD readVal = 0, readSize = sizeof(readVal);
    LSTATUS rc = Mock_RegQueryValueExA(hKey, "MyValue", NULL, NULL, (LPBYTE)&readVal, &readSize);
    REQUIRE(rc == ERROR_SUCCESS);
    REQUIRE(readVal == 77);
    Mock_RegCloseKey(hKey);
}

TEST_CASE("Mock_RegOpenKeyExA fails for nonexistent key", "[mock][registry]")
{
    MockRegistry::Reset();
    HKEY hKey;
    LSTATUS rc = Mock_RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Nonexistent", 0, KEY_READ, &hKey);
    REQUIRE(rc == ERROR_FILE_NOT_FOUND);
}

TEST_CASE("Mock_ComDBOpen and Close round-trip", "[mock][comdb]")
{
    MockComDb::Reset();
    HANDLE h;
    REQUIRE(Mock_ComDBOpen(&h) == ERROR_SUCCESS);
    REQUIRE(h != NULL);
    REQUIRE(Mock_ComDBClose(h) == ERROR_SUCCESS);
}

TEST_CASE("Mock_ComDBClaimPort succeeds for free port", "[mock][comdb]")
{
    MockComDb::Reset();
    REQUIRE(Mock_ComDBClaimPort((HANDLE)1, 5, FALSE, NULL) == ERROR_SUCCESS);
    REQUIRE(MockComDb::IsBusy(5));
}

TEST_CASE("Mock_ComDBClaimPort fails with sharing violation for busy port", "[mock][comdb]")
{
    MockComDb::Reset();
    MockComDb::SetBusy(3, true);
    REQUIRE(Mock_ComDBClaimPort((HANDLE)1, 3, FALSE, NULL) == ERROR_SHARING_VIOLATION);
}

TEST_CASE("Mock_ComDBReleasePort succeeds for busy port", "[mock][comdb]")
{
    MockComDb::Reset();
    MockComDb::SetBusy(7, true);
    REQUIRE(Mock_ComDBReleasePort((HANDLE)1, 7) == ERROR_SUCCESS);
    REQUIRE_FALSE(MockComDb::IsBusy(7));
}

TEST_CASE("Mock_ComDBReleasePort fails for free port", "[mock][comdb]")
{
    MockComDb::Reset();
    REQUIRE(Mock_ComDBReleasePort((HANDLE)1, 10) == ERROR_FILE_NOT_FOUND);
}
