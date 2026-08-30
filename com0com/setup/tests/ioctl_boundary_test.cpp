#include "catch_amalgamated.hpp"
#include <windows.h>
#include <ntddser.h>
#include <cstdio>
#include <cstring>

#include "../../include/cncext.h"

static const char *PORT_A = "\\\\.\\CNCA0";
static const char *PORT_B = "\\\\.\\CNCB0";

static const ULONG lengths[] = {
    0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12,
    15, 16, 31, 32, 63, 64, 127, 128, 255, 256
};

static HANDLE OpenPortRaw(const char *pName)
{
    HANDLE h = CreateFileA(pName, GENERIC_READ | GENERIC_WRITE, 0,
                           NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (h == INVALID_HANDLE_VALUE)
        FAIL("CreateFile(" << pName << ") failed: " << GetLastError());
    return h;
}

static void CheckIoResult(DWORD code, BOOL ok, DWORD err, DWORD returned, DWORD outLen)
{
    INFO("ioctl 0x" << std::hex << code << " returned=" << std::dec << returned
         << " err=" << err);
    if (ok) {
        REQUIRE(returned <= outLen);
    } else {
        REQUIRE((err == ERROR_INSUFFICIENT_BUFFER || err == ERROR_INVALID_PARAMETER));
    }
}

TEST_CASE("IOCTL boundary sweep does not overrun or hang", "[driver]")
{
    HANDLE hA = OpenPortRaw(PORT_A);
    HANDLE hB = OpenPortRaw(PORT_B);

    // Snapshot current state to restore after the sweep.
    SERIAL_BAUD_RATE savedBaud = {0};
    SERIAL_LINE_CONTROL savedLc = {0};
    SERIAL_HANDFLOW savedHf = {0};
    SERIAL_CHARS savedChars = {0};
    DWORD returned = 0;
    DeviceIoControl(hA, IOCTL_SERIAL_GET_BAUD_RATE, NULL, 0, &savedBaud, sizeof(savedBaud), &returned, NULL);
    DeviceIoControl(hA, IOCTL_SERIAL_GET_LINE_CONTROL, NULL, 0, &savedLc, sizeof(savedLc), &returned, NULL);
    DeviceIoControl(hA, IOCTL_SERIAL_GET_HANDFLOW, NULL, 0, &savedHf, sizeof(savedHf), &returned, NULL);
    DeviceIoControl(hA, IOCTL_SERIAL_GET_CHARS, NULL, 0, &savedChars, sizeof(savedChars), &returned, NULL);

    // Valid values used for the mutation IOCTLs during the sweep.
    SERIAL_BAUD_RATE baud = {0}; baud.BaudRate = 9600;
    SERIAL_LINE_CONTROL lc = {0}; lc.WordLength = 8; lc.Parity = NO_PARITY; lc.StopBits = STOP_BIT_1;
    SERIAL_HANDFLOW hf = {0};
    SERIAL_CHARS chars = {0}; chars.XonChar = 0x11; chars.XoffChar = 0x13;
    ULONG modem = 0;
    ULONG waitMask = SERIAL_EV_RXCHAR;
    UCHAR imm = 'X';
    ULONG purge = SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_RXCLEAR;
    SERIAL_QUEUE_SIZE qs = {0}; qs.InSize = 64; qs.OutSize = 64;
    SERIAL_TIMEOUTS to = {0};

    BYTE inBuf[256];
    BYTE outBuf[256];

    struct Io { DWORD code; const void *valid; DWORD validSize; };
    const Io inputIo[] = {
        {IOCTL_SERIAL_SET_BAUD_RATE, &baud, sizeof(baud)},
        {IOCTL_SERIAL_SET_LINE_CONTROL, &lc, sizeof(lc)},
        {IOCTL_SERIAL_SET_CHARS, &chars, sizeof(chars)},
        {IOCTL_SERIAL_SET_HANDFLOW, &hf, sizeof(hf)},
        {IOCTL_SERIAL_SET_MODEM_CONTROL, &modem, sizeof(modem)},
        {IOCTL_SERIAL_SET_WAIT_MASK, &waitMask, sizeof(waitMask)},
        {IOCTL_SERIAL_IMMEDIATE_CHAR, &imm, sizeof(imm)},
        {IOCTL_SERIAL_PURGE, &purge, sizeof(purge)},
        {IOCTL_SERIAL_SET_QUEUE_SIZE, &qs, sizeof(qs)},
        {IOCTL_SERIAL_LSRMST_INSERT, &imm, sizeof(imm)},
        {IOCTL_SERIAL_SET_TIMEOUTS, &to, sizeof(to)},
    };
    const DWORD outputIo[] = {
        IOCTL_SERIAL_GET_BAUD_RATE,
        IOCTL_SERIAL_GET_LINE_CONTROL,
        IOCTL_SERIAL_GET_CHARS,
        IOCTL_SERIAL_GET_HANDFLOW,
        IOCTL_SERIAL_GET_MODEM_CONTROL,
        IOCTL_SERIAL_GET_MODEMSTATUS,
        IOCTL_SERIAL_GET_WAIT_MASK,
        IOCTL_SERIAL_XOFF_COUNTER,
        IOCTL_SERIAL_GET_COMMSTATUS,
        IOCTL_SERIAL_GET_DTRRTS,
        IOCTL_SERIAL_CONFIG_SIZE,
        IOCTL_SERIAL_GET_STATS,
        IOCTL_SERIAL_GET_TIMEOUTS,
        IOCTL_SERIAL_GET_PROPERTIES,
    };
    const DWORD noBufIo[] = {
        IOCTL_SERIAL_SET_DTR,
        IOCTL_SERIAL_CLR_DTR,
        IOCTL_SERIAL_SET_RTS,
        IOCTL_SERIAL_CLR_RTS,
        IOCTL_SERIAL_SET_XON,
        IOCTL_SERIAL_SET_XOFF,
        IOCTL_SERIAL_SET_BREAK_ON,
        IOCTL_SERIAL_SET_BREAK_OFF,
        IOCTL_SERIAL_CLEAR_STATS,
    };

    // Sweep input lengths for every input IOCTL with valid payloads.
    for (const Io &io : inputIo) {
        memcpy(inBuf, io.valid, io.validSize);
        for (ULONG len : lengths) {
            CAPTURE(io.code, len);
            returned = 0;
            BOOL ok = DeviceIoControl(hA, io.code, inBuf, len, outBuf, sizeof(outBuf), &returned, NULL);
            CheckIoResult(io.code, ok, GetLastError(), returned, sizeof(outBuf));
        }
    }

    // Sweep output lengths for every output IOCTL.
    for (DWORD code : outputIo) {
        for (ULONG len : lengths) {
            CAPTURE(code, len);
            memset(outBuf, 0xCC, sizeof(outBuf));
            returned = 0;
            BOOL ok = DeviceIoControl(hA, code, NULL, 0, outBuf, len, &returned, NULL);
            CheckIoResult(code, ok, GetLastError(), returned, len);
        }
    }

    // No-buffer IOCTLs with zero-length buffers.
    for (DWORD code : noBufIo) {
        CAPTURE(code);
        returned = 0;
        BOOL ok = DeviceIoControl(hA, code, NULL, 0, NULL, 0, &returned, NULL);
        CheckIoResult(code, ok, GetLastError(), returned, 0);
    }

    // Restore the port state and flush stray data produced by the sweep.
    DeviceIoControl(hA, IOCTL_SERIAL_SET_BAUD_RATE, &savedBaud, sizeof(savedBaud), NULL, 0, &returned, NULL);
    DeviceIoControl(hA, IOCTL_SERIAL_SET_LINE_CONTROL, &savedLc, sizeof(savedLc), NULL, 0, &returned, NULL);
    DeviceIoControl(hA, IOCTL_SERIAL_SET_HANDFLOW, &savedHf, sizeof(savedHf), NULL, 0, &returned, NULL);
    DeviceIoControl(hA, IOCTL_SERIAL_SET_CHARS, &savedChars, sizeof(savedChars), NULL, 0, &returned, NULL);
    ULONG purgeAll = SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT |
                     SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_RXCLEAR;
    DeviceIoControl(hA, IOCTL_SERIAL_PURGE, &purgeAll, sizeof(purgeAll), NULL, 0, &returned, NULL);
    DeviceIoControl(hB, IOCTL_SERIAL_PURGE, &purgeAll, sizeof(purgeAll), NULL, 0, &returned, NULL);

    CloseHandle(hA);
    CloseHandle(hB);
}
