/*
 * setup/tests/mocks/comdb_mock.h
 *
 * In-memory ComDB for Tier 2 mock tests.
 *
 * Replaces msports.dll ComDB API with an in-memory bitmask.
 * The mock functions are prefixed with Mock_ and mapped via
 * win32_overrides.h preprocessor defines.
 */

#pragma once
#include <windows.h>

extern "C" {
LONG APIENTRY Mock_ComDBOpen(HANDLE *phComDB);
LONG APIENTRY Mock_ComDBClose(HANDLE hComDB);
LONG APIENTRY Mock_ComDBGetCurrentPortUsage(HANDLE hComDB, PBYTE pBuf, DWORD bufSize, ULONG flags, LPDWORD pMaxPorts);
LONG APIENTRY Mock_ComDBClaimPort(HANDLE hComDB, DWORD portNumber, BOOL force, PBYTE pReserved);
LONG APIENTRY Mock_ComDBReleasePort(HANDLE hComDB, DWORD portNumber);
}

class MockComDb
{
public:
    // Reset all state (call before each test)
    static void Reset();

    // Set which COM ports are currently "in use" in the system ComDB.
    // portNumbers are 1-based (COM1=1, COM2=2, etc.)
    static void SetBusy(int portNumber, bool busy);

    // Check if a port is marked busy
    static bool IsBusy(int portNumber);

    // Claim a port (marks it busy)
    static bool Claim(int portNumber);

    // Release a port (marks it free)
    static bool Release(int portNumber);

private:
    static bool s_openCalled;
};
