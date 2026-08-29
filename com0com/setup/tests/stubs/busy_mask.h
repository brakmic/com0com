/*
 * setup/tests/stubs/busy_mask.h
 *
 * Minimal BusyMask stub for test compilation.
 * Matches the interface from utils.h/devutils.h.
 */

#pragma once
#include <windows.h>

class BusyMask {
public:
    BusyMask() : pBusyMask(nullptr), busyMaskLen(0) {}
    ~BusyMask() { Clear(); }

    void Clear() { delete[] pBusyMask; pBusyMask = nullptr; busyMaskLen = 0; }

    bool AddNum(int num) {
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

    void DelNum(int num) {
        int byteIdx = num / 8;
        if (byteIdx < (int)busyMaskLen)
            pBusyMask[byteIdx] &= ~(1 << (num % 8));
    }

    bool IsFreeNum(int num) const {
        int byteIdx = num / 8;
        if (byteIdx >= (int)busyMaskLen) return true;
        return (pBusyMask[byteIdx] & (1 << (num % 8))) == 0;
    }

    int GetFirstFreeNum() const { return -1; }

private:
    PBYTE pBusyMask;
    SIZE_T busyMaskLen;
};
