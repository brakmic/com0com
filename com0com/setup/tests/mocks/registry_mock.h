/*
 * setup/tests/mocks/registry_mock.h
 *
 * In-memory registry for Tier 2 mock tests.
 *
 * Replaces the real Windows registry with an in-memory store. The mock
 * functions are prefixed with Mock_ to avoid linker conflicts. The
 * win32_overrides.h header maps real API names to these mock versions
 * via preprocessor defines.
 *
 * Usage from tests:
 *   MockRegistry::Reset();
 *   MockRegistry::SetValue("HKLM\\...", "PortNum", REG_DWORD, &val, 4);
 *   // ... call production function (which now hits mock registry) ...
 *   DWORD actual = MockRegistry::GetDword("HKLM\\...", "PortNum");
 */

#pragma once
#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include <cstring>

// Mock API function declarations (C linkage to match real Win32 API)
extern "C" {
LSTATUS APIENTRY Mock_RegOpenKeyExA(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
LSTATUS APIENTRY Mock_RegQueryValueExA(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
LSTATUS APIENTRY Mock_RegSetValueExA(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD);
LSTATUS APIENTRY Mock_RegCloseKey(HKEY);
LSTATUS APIENTRY Mock_RegCreateKeyExA(HKEY, LPCSTR, DWORD, LPSTR, DWORD, REGSAM, const LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
LSTATUS APIENTRY Mock_RegDeleteValueA(HKEY, LPCSTR);
LSTATUS APIENTRY Mock_RegDeleteKeyA(HKEY, LPCSTR);
LSTATUS APIENTRY Mock_RegEnumKeyExA(HKEY, DWORD, LPSTR, LPDWORD, LPDWORD, LPSTR, LPDWORD, PFILETIME);
}

class MockRegistry
{
public:
    static void Reset();
    static void SetValue(const char *pKeyPath, const char *pValueName,
                         DWORD type, const BYTE *pData, DWORD size);
    static bool GetValue(const char *pKeyPath, const char *pValueName,
                         DWORD *pType, std::vector<BYTE> &data);
    static DWORD GetDword(const char *pKeyPath, const char *pValueName);
    static std::string GetString(const char *pKeyPath, const char *pValueName);
    static bool KeyExists(const char *pKeyPath);
    static void DeleteKey(const char *pKeyPath);

    // Public for extern "C" mock functions (MSVC friend + extern "C" limitation)
    struct ValueEntry { DWORD type; std::vector<BYTE> data; };
    static std::map<std::string, std::map<std::string, ValueEntry>> s_store;
    static int s_nextHandle;
};
