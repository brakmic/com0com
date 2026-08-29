/*
 * setup/tests/mocks/fileio_mock.cpp
 *
 * Minimal stub implementations for file I/O functions.
 * All operations return failure by default.
 */

#include <windows.h>

extern "C" {

HANDLE APIENTRY Mock_CreateFileA(
    LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    (void)lpFileName; (void)dwDesiredAccess; (void)dwShareMode;
    (void)lpSecurityAttributes; (void)dwCreationDisposition;
    (void)dwFlagsAndAttributes; (void)hTemplateFile;
    SetLastError(ERROR_FILE_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

BOOL APIENTRY Mock_WriteFile(
    HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    (void)hFile; (void)lpBuffer; (void)nNumberOfBytesToWrite;
    (void)lpNumberOfBytesWritten; (void)lpOverlapped;
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}

BOOL APIENTRY Mock_ReadFile(
    HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
    (void)hFile; (void)lpBuffer; (void)nNumberOfBytesToRead;
    (void)lpNumberOfBytesRead; (void)lpOverlapped;
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}

BOOL APIENTRY Mock_CloseHandle_File(HANDLE hObject)
{
    (void)hObject;
    return TRUE;
}

DWORD APIENTRY Mock_SetFilePointer(
    HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
    (void)hFile; (void)lDistanceToMove; (void)lpDistanceToMoveHigh; (void)dwMoveMethod;
    SetLastError(ERROR_INVALID_HANDLE);
    return INVALID_SET_FILE_POINTER;
}

DWORD APIENTRY Mock_GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
    (void)hFile; (void)lpFileSizeHigh;
    SetLastError(ERROR_INVALID_HANDLE);
    return INVALID_FILE_SIZE;
}

BOOL APIENTRY Mock_DeleteFileA(LPCSTR lpFileName)
{
    (void)lpFileName;
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

} // extern "C"

// Memory stubs
extern "C" {

HLOCAL APIENTRY Mock_LocalAlloc(UINT uFlags, SIZE_T uBytes)
{
    (void)uFlags;
    return LocalAlloc(LPTR, uBytes); // Use real LocalAlloc (kernel32)
}

HLOCAL APIENTRY Mock_LocalFree(HLOCAL hMem)
{
    return LocalFree(hMem); // Use real LocalFree (kernel32)
}

}
