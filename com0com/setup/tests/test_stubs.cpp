/*
 * Test stubs for setup.dll dependencies.
 *
 * Provides minimal implementations of functions that params.cpp and comdb.cpp
 * call, so they can be compiled into the test binary without linking against
 * the full setup.dll. Each stub documents which production function it replaces.
 */

#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include "stubs/busy_mask.h"

// ── from utils.h ───────────────────────────────────────────────────

// Replacement for SNPRINTF/VSNPRINTF in utils.cpp.
// Wraps _vsnprintf with the same return-value semantics as the real one:
// returns -1 on truncation, otherwise the number of characters written.
// NOTE: No extern "C" — the production headers declare these as C++ linkage.
int VSNPRINTF(char *pBuf, int size, const char *pFmt, va_list va)
{
    if (size <= 0) return -1;
    int n = _vsnprintf(pBuf, size, pFmt, va);
    if (n < 0 || n >= size) {
        pBuf[size - 1] = 0;
        return -1;
    }
    return n;
}

int SNPRINTF(char *pBuf, int size, const char *pFmt, ...)
{
    va_list va;
    va_start(va, pFmt);
    int n = VSNPRINTF(pBuf, size, pFmt, va);
    va_end(va);
    return n;
}

// StrToInt is used by name2num in comdb.cpp.
// <shlwapi.h> #defines StrToInt → StrToIntA (ANSI build).
// Our stub provides both names with bool return (matching utils.h).
bool StrToInt(const char *pStr, int *pNum)
{
    if (!pStr || !*pStr) return false;
    char *end = nullptr;
    long val = strtol(pStr, &end, 10);
    if (end == pStr || *end != 0) return false;
    if (val < INT_MIN || val > INT_MAX) return false;
    *pNum = (int)val;
    return true;
}

extern "C" bool StrToIntA(const char *pStr, int *pNum) {
    return StrToInt(pStr, pNum);
}

// Replacement for STRTOK_R in utils.cpp.
// Thread-safe strtok that uses a caller-provided save pointer.
char *STRTOK_R(char *pStr, const char *pDelims, char **ppSave)
{
    if (pStr)
        *ppSave = pStr;
    if (!*ppSave)
        return nullptr;

    // Skip leading delimiters
    while (**ppSave && strchr(pDelims, **ppSave))
        (*ppSave)++;

    if (!**ppSave)
        return nullptr;

    char *pToken = *ppSave;

    // Find end of token
    while (**ppSave && !strchr(pDelims, **ppSave))
        (*ppSave)++;

    if (**ppSave)
    {
        **ppSave = 0;
        (*ppSave)++;
    }
    else
        *ppSave = nullptr;

    return pToken;
}

// ── from msg.h ─────────────────────────────────────────────────────

// Trace writes formatted output to the console (or --output file).
// In tests, we capture it to a buffer for verification.
static char g_traceBuffer[4096];
static size_t g_traceLen = 0;

void Trace(const char *pFmt, ...)
{
    va_list va;
    va_start(va, pFmt);
    int remaining = (int)(sizeof(g_traceBuffer) - g_traceLen - 1);
    if (remaining > 0) {
        int n = _vsnprintf(g_traceBuffer + g_traceLen, remaining, pFmt, va);
        if (n > 0) g_traceLen += (n < remaining ? n : remaining);
    }
    va_end(va);
}

const char *TestGetTrace() { return g_traceBuffer; }
void TestClearTrace() { g_traceBuffer[0] = 0; g_traceLen = 0; }

// ShowError/ShowMsg are UI functions not needed for unit tests.
int ShowError(unsigned int type, unsigned long err, const char *pFmt, ...) { return 0; }
int ShowLastError(unsigned int type, const char *pFmt, ...) { return 0; }
int ShowMsg(unsigned int type, const char *pFmt, ...) { return 0; }

// ── from devutils.h ────────────────────────────────────────────────

// Stubs moved to comdb_stubs.cpp (needs production headers) and
// stubs/busy_mask.h (inline, used by tests that don't link comdb.cpp).
void BusyMask_Clear(void *) {}
bool BusyMask_AddNum(void *, int) { return true; }
void BusyMask_DelNum(void *, int) {}
bool BusyMask_IsFreeNum(void *, int) { return true; }

// ── from portnum.h ─────────────────────────────────────────────────