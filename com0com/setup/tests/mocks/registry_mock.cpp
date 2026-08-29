/*
 * setup/tests/mocks/registry_mock.cpp
 *
 * In-memory registry mock — replaces advapi32.dll registry functions.
 *
 * These functions have the EXACT same signatures as the real Win32 API.
 * The linker prefers object files over .lib files, so linking this .obj
 * before advapi32.lib causes the mock to be used instead of the real API.
 */

#include "registry_mock.h"
#include <cstring>

// ── mock state ────────────────────────────────────────────────────
std::map<std::string, std::map<std::string, MockRegistry::ValueEntry>> MockRegistry::s_store;
int MockRegistry::s_nextHandle = 1;

void MockRegistry::Reset()
{
    s_store.clear();
    s_nextHandle = 1;
}

void MockRegistry::SetValue(const char *pKeyPath, const char *pValueName,
                            DWORD type, const BYTE *pData, DWORD size)
{
    ValueEntry entry;
    entry.type = type;
    entry.data.assign(pData, pData + size);
    s_store[pKeyPath][pValueName] = entry;
}

bool MockRegistry::GetValue(const char *pKeyPath, const char *pValueName,
                            DWORD *pType, std::vector<BYTE> &data)
{
    auto keyIt = s_store.find(pKeyPath);
    if (keyIt == s_store.end()) return false;

    auto valIt = keyIt->second.find(pValueName);
    if (valIt == keyIt->second.end()) return false;

    if (pType) *pType = valIt->second.type;
    data = valIt->second.data;
    return true;
}

DWORD MockRegistry::GetDword(const char *pKeyPath, const char *pValueName)
{
    std::vector<BYTE> data;
    DWORD type;
    if (GetValue(pKeyPath, pValueName, &type, data) && type == REG_DWORD && data.size() >= 4) {
        return *(DWORD*)data.data();
    }
    return 0xFFFFFFFF; // sentinel
}

std::string MockRegistry::GetString(const char *pKeyPath, const char *pValueName)
{
    std::vector<BYTE> data;
    DWORD type;
    if (GetValue(pKeyPath, pValueName, &type, data) && type == REG_SZ) {
        return std::string((const char*)data.data(), data.size());
    }
    return "";
}

bool MockRegistry::KeyExists(const char *pKeyPath)
{
    return s_store.find(pKeyPath) != s_store.end();
}

void MockRegistry::DeleteKey(const char *pKeyPath)
{
    s_store.erase(pKeyPath);
}

// ── mock API functions (same signatures as Win32) ─────────────────

// We use a simple numeric handle scheme: each "open key" is identified
// by a unique integer cast to HKEY. The real HKEY_LOCAL_MACHINE etc.
// are also valid handles — we recognize them by their pointer values.

// Map from HKEY (as integer) to key path string
static std::map<HKEY, std::string> g_openKeys;

static std::string HkeyToPath(HKEY hKey)
{
    // Predefined handles
    if (hKey == HKEY_LOCAL_MACHINE)  return "HKLM";
    if (hKey == HKEY_CURRENT_USER)   return "HKCU";
    if (hKey == HKEY_CLASSES_ROOT)   return "HKCR";

    auto it = g_openKeys.find(hKey);
    if (it != g_openKeys.end()) return it->second;
    return "";
}

static std::string MakeFullPath(HKEY hKey, LPCSTR lpSubKey)
{
    std::string base = HkeyToPath(hKey);
    if (base.empty()) return "";
    if (!lpSubKey || !*lpSubKey) return base;
    return base + "\\" + lpSubKey;
}

// ──────────────────────────────────────────────────────────────────
// These functions replace the real Win32 registry API for test builds.
// They have C linkage to match the real API exports.
// ──────────────────────────────────────────────────────────────────

extern "C" {

LSTATUS APIENTRY Mock_RegOpenKeyExA(
    HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    std::string fullPath = MakeFullPath(hKey, lpSubKey);
    if (fullPath.empty()) return ERROR_FILE_NOT_FOUND;

    if (!MockRegistry::KeyExists(fullPath.c_str()))
        return ERROR_FILE_NOT_FOUND;

    HKEY handle = (HKEY)(ULONG_PTR)MockRegistry::s_nextHandle++;
    g_openKeys[handle] = fullPath;
    *phkResult = handle;
    return ERROR_SUCCESS;
}

LSTATUS APIENTRY Mock_RegQueryValueExA(
    HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType,
    LPBYTE lpData, LPDWORD lpcbData)
{
    std::string path = HkeyToPath(hKey);
    if (path.empty()) return ERROR_FILE_NOT_FOUND;

    std::vector<BYTE> data;
    DWORD type;
    if (!MockRegistry::GetValue(path.c_str(), lpValueName ? lpValueName : "", &type, data))
        return ERROR_FILE_NOT_FOUND;

    if (lpType) *lpType = type;

    if (lpData && lpcbData) {
        DWORD copySize = (*lpcbData < (DWORD)data.size()) ? *lpcbData : (DWORD)data.size();
        memcpy(lpData, data.data(), copySize);
    }
    if (lpcbData) *lpcbData = (DWORD)data.size();

    return ERROR_SUCCESS;
}

LSTATUS APIENTRY Mock_RegSetValueExA(
    HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType,
    const BYTE *lpData, DWORD cbData)
{
    std::string path = HkeyToPath(hKey);
    if (path.empty()) return ERROR_FILE_NOT_FOUND;

    MockRegistry::s_store[path]; // ensure key exists
    MockRegistry::SetValue(path.c_str(), lpValueName ? lpValueName : "", dwType, lpData, cbData);
    return ERROR_SUCCESS;
}

LSTATUS APIENTRY Mock_RegCloseKey(HKEY hKey)
{
    g_openKeys.erase(hKey);
    return ERROR_SUCCESS;
}

LSTATUS APIENTRY Mock_RegCreateKeyExA(
    HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions,
    REGSAM samDesired, const LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult, LPDWORD lpdwDisposition)
{
    std::string fullPath = MakeFullPath(hKey, lpSubKey);
    if (fullPath.empty()) return ERROR_INVALID_PARAMETER;

    bool existed = MockRegistry::KeyExists(fullPath.c_str());
    MockRegistry::s_store[fullPath]; // create empty key if not exists

    HKEY handle = (HKEY)(ULONG_PTR)MockRegistry::s_nextHandle++;
    g_openKeys[handle] = fullPath;
    *phkResult = handle;
    if (lpdwDisposition) *lpdwDisposition = existed ? REG_OPENED_EXISTING_KEY : REG_CREATED_NEW_KEY;
    return ERROR_SUCCESS;
}

LSTATUS APIENTRY Mock_RegDeleteValueA(HKEY hKey, LPCSTR lpValueName)
{
    std::string path = HkeyToPath(hKey);
    if (path.empty()) return ERROR_FILE_NOT_FOUND;

    auto keyIt = MockRegistry::s_store.find(path);
    if (keyIt == MockRegistry::s_store.end()) return ERROR_FILE_NOT_FOUND;

    keyIt->second.erase(lpValueName ? lpValueName : "");
    return ERROR_SUCCESS;
}

LSTATUS APIENTRY Mock_RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
{
    std::string fullPath = MakeFullPath(hKey, lpSubKey);
    if (fullPath.empty()) return ERROR_FILE_NOT_FOUND;

    MockRegistry::DeleteKey(fullPath.c_str());
    return ERROR_SUCCESS;
}

} // extern "C"

// ── Stub for Mock_RegEnumKeyExA (not fully implemented — returns no subkeys) ─
extern "C" {
LSTATUS APIENTRY Mock_RegEnumKeyExA(
    HKEY, DWORD, LPSTR, LPDWORD, LPDWORD, LPSTR, LPDWORD, PFILETIME)
{
    return ERROR_NO_MORE_ITEMS;
}
}
