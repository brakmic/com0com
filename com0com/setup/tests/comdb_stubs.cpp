/*
 * setup/tests/comdb_stubs.cpp
 * Provides implementations of functions that comdb.cpp calls
 * but whose implementations are in other translation units.
 */
#include "precomp.h"
#include "../utils.h"     // BusyMask, SNPRINTF, StrToInt, etc.
#include "../devutils.h"  // DevProperties, EnumDevices
#include "../portnum.h"   // GetPortNum
#include "../params.h"    // PortParameters
#include <cstring>

// BusyMask (from utils.h). Constructor and destructor are inline in utils.h.
// Only non-inline methods need definitions.
void BusyMask::Clear() { delete[] pBusyMask; pBusyMask = nullptr; busyMaskLen = 0; }
bool BusyMask::AddNum(int num) {
    int byteIdx = num / 8;
    if (byteIdx >= (int)busyMaskLen) {
        SIZE_T newLen = byteIdx + 16;
        PBYTE pNew = new BYTE[newLen];
        memset(pNew, 0, newLen);
        if (pBusyMask) { memcpy(pNew, pBusyMask, busyMaskLen); delete[] pBusyMask; }
        pBusyMask = pNew;
        busyMaskLen = newLen;
    }
    pBusyMask[byteIdx] |= (1 << (num % 8));
    return true;
}
void BusyMask::DelNum(int num) {
    int byteIdx = num / 8;
    if (byteIdx < (int)busyMaskLen) pBusyMask[byteIdx] &= ~(1 << (num % 8));
}
bool BusyMask::IsFreeNum(int num) const {
    int byteIdx = num / 8;
    if (byteIdx >= (int)busyMaskLen) return true;
    return (pBusyMask[byteIdx] & (1 << (num % 8))) == 0;
}

// DevProperties (from devutils.h)
const char *DevProperties::DevId(const char *_pDevId) {
    if (_pDevId) { delete[] pDevId; pDevId = _strdup(_pDevId); return pDevId; }
    delete[] pDevId; pDevId = nullptr; return nullptr;
}
const char *DevProperties::PhObjName(const char *_p) {
    if (_p) { delete[] pPhObjName; pPhObjName = _strdup(_p); return pPhObjName; }
    delete[] pPhObjName; pPhObjName = nullptr; return nullptr;
}
const char *DevProperties::Location(const char *_p) {
    if (_p) { delete[] pLocation; pLocation = _strdup(_p); return pLocation; }
    delete[] pLocation; pLocation = nullptr; return nullptr;
}

int EnumDevices(PCNC_ENUM_FILTER, PCDevProperties, BOOL *, PCNC_DEV_CALLBACK, void *) { return 0; }
int GetPortNum(HDEVINFO, PSP_DEVINFO_DATA) { return -1; }

// <shlwapi.h> #defines StrToInt→StrToIntA. When utils.h is processed,
// its `bool StrToInt(const char*,int*)` becomes `bool StrToIntA(const char*,int*)`
// with C++ linkage. We need to provide this exact symbol.
bool StrToIntA(const char *pStr, int *pNum) {
    if (!pStr || !*pStr) return false;
    char *end = nullptr;
    long val = strtol(pStr, &end, 10);
    if (end == pStr || *end) return false;
    *pNum = (int)val;
    return true;
}
