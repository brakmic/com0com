/*
 * setup/tests/mocks/setupapi_mock.cpp
 *
 * Minimal stub implementations for SetupAPI functions.
 * These return "no devices found" by default.
 * Tests can inject specific device information via MockRegistry.
 */

#include <windows.h>
#include <setupapi.h>

extern "C" {

HDEVINFO APIENTRY Mock_SetupDiGetClassDevsA(
    const GUID *ClassGuid, PCSTR Enumerator, HWND hwndParent, DWORD Flags)
{
    (void)ClassGuid; (void)Enumerator; (void)hwndParent; (void)Flags;
    // Return a dummy handle — non-NULL and not INVALID_HANDLE_VALUE
    return (HDEVINFO)(ULONG_PTR)1;
}

BOOL APIENTRY Mock_SetupDiEnumDeviceInfo(
    HDEVINFO DeviceInfoSet, DWORD MemberIndex, PSP_DEVINFO_DATA DeviceInfoData)
{
    (void)DeviceInfoSet; (void)MemberIndex; (void)DeviceInfoData;
    // No devices enumerated
    SetLastError(ERROR_NO_MORE_ITEMS);
    return FALSE;
}

BOOL APIENTRY Mock_SetupDiDestroyDeviceInfoList(HDEVINFO DeviceInfoSet)
{
    (void)DeviceInfoSet;
    return TRUE;
}

HKEY APIENTRY Mock_SetupDiOpenDevRegKey(
    HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
    DWORD Scope, DWORD HwProfile, DWORD KeyType, REGSAM samDesired)
{
    (void)DeviceInfoSet; (void)DeviceInfoData; (void)Scope;
    (void)HwProfile; (void)KeyType; (void)samDesired;
    SetLastError(ERROR_FILE_NOT_FOUND);
    return (HKEY)INVALID_HANDLE_VALUE;
}

BOOL APIENTRY Mock_SetupDiGetDeviceRegistryPropertyA(
    HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
    DWORD Property, PDWORD PropertyRegDataType, PBYTE PropertyBuffer,
    DWORD PropertyBufferSize, PDWORD RequiredSize)
{
    (void)DeviceInfoSet; (void)DeviceInfoData; (void)Property;
    (void)PropertyRegDataType; (void)PropertyBuffer;
    (void)PropertyBufferSize; (void)RequiredSize;
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

BOOL APIENTRY Mock_SetupDiCreateDevRegKeyA(
    HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
    DWORD Scope, DWORD HwProfile, DWORD KeyType, HINF InfHandle,
    PCSTR InfSectionName)
{
    (void)DeviceInfoSet; (void)DeviceInfoData; (void)Scope;
    (void)HwProfile; (void)KeyType; (void)InfHandle; (void)InfSectionName;
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

BOOL APIENTRY Mock_SetupDiCallClassInstaller(
    DWORD InstallFunction, HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData)
{
    (void)InstallFunction; (void)DeviceInfoSet; (void)DeviceInfoData;
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL APIENTRY Mock_SetupDiGetDeviceInstanceIdA(
    HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
    PSTR DeviceInstanceId, DWORD DeviceInstanceIdSize, PDWORD RequiredSize)
{
    (void)DeviceInfoSet; (void)DeviceInfoData;
    (void)DeviceInstanceId; (void)DeviceInstanceIdSize; (void)RequiredSize;
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

} // extern "C"
