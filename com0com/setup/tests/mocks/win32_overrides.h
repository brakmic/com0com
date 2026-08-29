/*
 * setup/tests/mocks/win32_overrides.h
 *
 * Preprocessor overrides for Win32 API functions used by setup.dll.
 *
 * Include this header BEFORE any production source or Windows headers.
 * It redefines Win32 API function names to mock versions, so production
 * code calls the mock without any source changes.
 *
 * The mock functions have identical signatures but use in-memory state
 * instead of the real Windows registry, ComDB, and SetupAPI.
 *
 * Usage:
 *   #include "mocks/win32_overrides.h"
 *   #include "../comdb.cpp"        // now calls Mock_RegOpenKeyExA etc.
 *   #include "mocks/registry_mock.h" // access mock state from tests
 */

#pragma once

// ══════════════════════════════════════════════════════════════════
// Registry API (advapi32.dll)
// ══════════════════════════════════════════════════════════════════
#define RegOpenKeyExA       Mock_RegOpenKeyExA
#define RegQueryValueExA    Mock_RegQueryValueExA
#define RegSetValueExA      Mock_RegSetValueExA
#define RegCloseKey         Mock_RegCloseKey
#define RegCreateKeyExA     Mock_RegCreateKeyExA
#define RegDeleteValueA     Mock_RegDeleteValueA
#define RegDeleteKeyA       Mock_RegDeleteKeyA
#define RegEnumKeyExA       Mock_RegEnumKeyExA

// ══════════════════════════════════════════════════════════════════
// ComDB API (msports.dll)
// ══════════════════════════════════════════════════════════════════
#define ComDBOpen               Mock_ComDBOpen
#define ComDBClose              Mock_ComDBClose
#define ComDBGetCurrentPortUsage Mock_ComDBGetCurrentPortUsage
#define ComDBClaimPort          Mock_ComDBClaimPort
#define ComDBReleasePort        Mock_ComDBReleasePort

// ══════════════════════════════════════════════════════════════════
// SetupAPI (setupapi.dll)
// ══════════════════════════════════════════════════════════════════
#define SetupDiGetClassDevsA        Mock_SetupDiGetClassDevsA
#define SetupDiEnumDeviceInfo       Mock_SetupDiEnumDeviceInfo
#define SetupDiDestroyDeviceInfoList Mock_SetupDiDestroyDeviceInfoList
#define SetupDiOpenDevRegKey        Mock_SetupDiOpenDevRegKey
#define SetupDiGetDeviceRegistryPropertyA Mock_SetupDiGetDeviceRegistryPropertyA
#define SetupDiCreateDevRegKeyA     Mock_SetupDiCreateDevRegKeyA
#define SetupDiCallClassInstaller   Mock_SetupDiCallClassInstaller
#define SetupDiGetDeviceInstanceIdA Mock_SetupDiGetDeviceInstanceIdA

// ══════════════════════════════════════════════════════════════════
// File I/O (kernel32.dll)
// ══════════════════════════════════════════════════════════════════
#define CreateFileA             Mock_CreateFileA
#define WriteFile               Mock_WriteFile
#define ReadFile                Mock_ReadFile
#define CloseHandle             Mock_CloseHandle_File
#define SetFilePointer          Mock_SetFilePointer
#define GetFileSize             Mock_GetFileSize
#define DeleteFileA             Mock_DeleteFileA

// ══════════════════════════════════════════════════════════════════
// Memory (kernel32.dll) — use real implementations, not mocked
// ══════════════════════════════════════════════════════════════════
// LocalAlloc and LocalFree are intentionally NOT overridden.
// They call into kernel32.dll and do not need mocking for tests.
