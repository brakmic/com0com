#include "catch_amalgamated.hpp"
#include <windows.h>
#include <ntddser.h>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <cstdlib>
#include <thread>

static const char *PORT_A = "\\\\.\\CNCA0";
static const char *PORT_B = "\\\\.\\CNCB0";

// Fixed-size framed stream protocol: 4-byte magic, sequence number,
// deterministic 16-byte payload derived from the sequence number.
static const BYTE FRAME_MAGIC[4] = {'S', 'T', 'R', '1'};
static const DWORD FRAME_SIZE = 4 + sizeof(ULONG) + 16;

static ULONG XorShift(ULONG &state)
{
    ULONG x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

static void FillPayload(BYTE *p, ULONG seq)
{
    ULONG state = seq * 2654435761u + 1;
    for (int i = 0; i < 16; i++)
        p[i] = (BYTE)XorShift(state);
}

static bool PayloadMatches(const BYTE *p, ULONG seq)
{
    BYTE expect[16];
    FillPayload(expect, seq);
    return memcmp(p, expect, sizeof(expect)) == 0;
}

static HANDLE OpenStressPort(const char *pName)
{
    HANDLE h = CreateFileA(pName, GENERIC_READ | GENERIC_WRITE, 0,
                           NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        FAIL("CreateFile(" << pName << ") failed: " << GetLastError());

    COMMTIMEOUTS to = {0};
    to.ReadIntervalTimeout = MAXDWORD;
    to.ReadTotalTimeoutMultiplier = 0;
    to.ReadTotalTimeoutConstant = 3000;
    to.WriteTotalTimeoutMultiplier = 0;
    to.WriteTotalTimeoutConstant = 3000;
    if (!SetCommTimeouts(h, &to))
        FAIL("SetCommTimeouts failed: " << GetLastError());
    return h;
}

static DWORD ReadFull(HANDLE h, BYTE *buf, DWORD want)
{
    DWORD got = 0;
    while (got < want) {
        DWORD n = 0;
        if (!ReadFile(h, buf + got, want - got, &n, NULL) || n == 0)
            break;
        got += n;
    }
    return got;
}

static COMMTIMEOUTS TransferTimeouts = {
    MAXDWORD, 0, 3000, // read: immediate return when data exists, 3 s stall cap
    0, 3000            // write: 3 s stall cap
};

static void WriterThread(HANDLE h, ULONG frames, std::atomic<bool> &stop)
{
    for (ULONG seq = 0; seq < frames && !stop; seq++) {
        BYTE frame[FRAME_SIZE];
        memcpy(frame, FRAME_MAGIC, sizeof(FRAME_MAGIC));
        memcpy(frame + 4, &seq, sizeof(seq));
        FillPayload(frame + 4 + sizeof(ULONG), seq);

        // The settings workers modify timeouts on this port.
        SetCommTimeouts(h, &TransferTimeouts);
        DWORD written = 0;
        if (!WriteFile(h, frame, sizeof(frame), &written, NULL) || written != sizeof(frame)) {
            stop = true;
            return;
        }
    }
}

static void ReaderThread(HANDLE h, ULONG frames, std::atomic<bool> &stop,
                         std::atomic<int> &readErrors, std::atomic<int> &seqErrors,
                         std::atomic<int> &payloadErrors)
{
    // Stream parser: reads chunks, scans for the frame magic, and only
    // consumes a frame when its deterministic payload validates. Random
    // payload bytes can contain the magic sequence by chance.
    BYTE window[4096];
    DWORD have = 0;
    ULONG nextSeq = 0;
    DWORD idleStart = GetTickCount();

    while (!stop && nextSeq < frames) {
        // Top up the window.
        DWORD room = sizeof(window) - have;
        if (room < FRAME_SIZE * 2) {
            memmove(window, window + have - FRAME_SIZE, FRAME_SIZE);
            have = FRAME_SIZE;
            room = sizeof(window) - have;
        }

        DWORD n = 0;
        SetCommTimeouts(h, &TransferTimeouts);
        if (!ReadFile(h, window + have, room, &n, NULL)) {
            readErrors++;
            return;
        }
        if (n == 0) {
            // The settings workers may have made the port non-blocking for
            // an instant. Treat an empty read as polling and only give up
            // after a long stall.
            if (GetTickCount() - idleStart > 10000) {
                readErrors++;
                return;
            }
            Sleep(1);
            continue;
        }
        idleStart = GetTickCount();
        have += n;

        // Scan for a validated frame boundary.
        bool progress = true;
        while (progress && have >= FRAME_SIZE) {
            progress = false;
            for (DWORD i = 0; i + FRAME_SIZE <= have; i++) {
                if (memcmp(window + i, FRAME_MAGIC, sizeof(FRAME_MAGIC)) != 0)
                    continue;
                ULONG seq = 0;
                memcpy(&seq, window + i + 4, sizeof(seq));
                if (!PayloadMatches(window + i + 4 + sizeof(ULONG), seq))
                    continue; // false magic inside random payload
                if (seq != nextSeq) {
                    seqErrors++;
                    nextSeq = seq;
                }
                nextSeq++;
                have -= i + FRAME_SIZE;
                if (have)
                    memmove(window, window + i + FRAME_SIZE, have);
                progress = true;
                break;
            }
        }
    }
}

static void SettingsWorkerThread(HANDLE hA, HANDLE hB, std::atomic<bool> &stop)
{
    ULONG rng = 0x12345678;
    const ULONG bauds[] = {9600, 19200, 38400, 57600, 115200};

    while (!stop) {
        DCB dcb = {0};
        dcb.DCBlength = sizeof(dcb);

        switch (XorShift(rng) % 6) {
        case 0:
            GetCommState(hA, &dcb);
            break;
        case 1:
            dcb.BaudRate = bauds[XorShift(rng) % (sizeof(bauds) / sizeof(bauds[0]))];
            dcb.ByteSize = 8;
            dcb.Parity = NOPARITY;
            dcb.StopBits = ONESTOPBIT;
            SetCommState(hA, &dcb);
            break;
        case 2: {
            COMMTIMEOUTS to = {0};
            GetCommTimeouts(hA, &to);
            to.ReadIntervalTimeout = MAXDWORD;
            to.WriteTotalTimeoutConstant = 3000;
            SetCommTimeouts(hA, &to);
            break;
        }
        case 3: {
            DWORD errs = 0;
            COMSTAT st = {0};
            ClearCommError(hA, &errs, &st);
            break;
        }
        case 4:
            EscapeCommFunction(hA, (XorShift(rng) & 1) ? SETDTR : CLRDTR);
            EscapeCommFunction(hA, (XorShift(rng) & 1) ? SETRTS : CLRRTS);
            break;
        case 5:
            SetupComm(hA, 4096, 4096);
            SetupComm(hB, 4096, 4096);
            break;
        }
    }
}

TEST_CASE("Full-duplex stress transfer with concurrent settings traffic", "[driver][stress]")
{
    const char *envFrames = std::getenv("COM0COM_STRESS_FRAMES");
    ULONG frames = envFrames ? (ULONG)std::strtoul(envFrames, NULL, 10) : 2000;
    if (frames == 0)
        frames = 2000;

    HANDLE hA = OpenStressPort(PORT_A);
    HANDLE hB = OpenStressPort(PORT_B);

    // Clear both directions before measuring.
    DWORD purgeAll = SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT |
                     SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_RXCLEAR;
    DWORD returned = 0;
    DeviceIoControl(hA, IOCTL_SERIAL_PURGE, &purgeAll, sizeof(purgeAll), NULL, 0, &returned, NULL);
    DeviceIoControl(hB, IOCTL_SERIAL_PURGE, &purgeAll, sizeof(purgeAll), NULL, 0, &returned, NULL);

    // Disable modem handshake for the duration of the run. The settings
    // workers toggle DTR and RTS, which would otherwise throttle or block
    // the opposite writer through the default DTR/RTS handshake.
    SERIAL_HANDFLOW savedHandflow = {0};
    SERIAL_HANDFLOW noHandflow = {0};
    DeviceIoControl(hA, IOCTL_SERIAL_GET_HANDFLOW, NULL, 0,
                    &savedHandflow, sizeof(savedHandflow), &returned, NULL);
    DeviceIoControl(hA, IOCTL_SERIAL_SET_HANDFLOW, &noHandflow, sizeof(noHandflow),
                    NULL, 0, &returned, NULL);

    std::atomic<bool> stop(false);
    std::atomic<int> readErrorsA(0), seqErrorsA(0), payloadErrorsA(0);
    std::atomic<int> readErrorsB(0), seqErrorsB(0), payloadErrorsB(0);

    std::thread writerA(WriterThread, hA, frames, std::ref(stop));
    std::thread writerB(WriterThread, hB, frames, std::ref(stop));
    std::thread readerB(ReaderThread, hB, frames, std::ref(stop),
                        std::ref(readErrorsA), std::ref(seqErrorsA), std::ref(payloadErrorsA));
    std::thread readerA(ReaderThread, hA, frames, std::ref(stop),
                        std::ref(readErrorsB), std::ref(seqErrorsB), std::ref(payloadErrorsB));
    std::thread workerA(SettingsWorkerThread, hA, hB, std::ref(stop));
    std::thread workerB(SettingsWorkerThread, hB, hA, std::ref(stop));

    writerA.join();
    writerB.join();
    readerB.join();
    readerA.join();
    stop = true;
    workerA.join();
    workerB.join();

    // Restore a known state after the workers changed settings on both ports.
    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    dcb.BaudRate = 9600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(hA, &dcb);
    SetCommState(hB, &dcb);
    DeviceIoControl(hA, IOCTL_SERIAL_SET_HANDFLOW, &savedHandflow, sizeof(savedHandflow),
                    NULL, 0, &returned, NULL);

    DeviceIoControl(hA, IOCTL_SERIAL_PURGE, &purgeAll, sizeof(purgeAll), NULL, 0, &returned, NULL);
    DeviceIoControl(hB, IOCTL_SERIAL_PURGE, &purgeAll, sizeof(purgeAll), NULL, 0, &returned, NULL);

    CloseHandle(hA);
    CloseHandle(hB);

    INFO("A->B read errors " << readErrorsA << " seq errors " << seqErrorsA
         << " payload errors " << payloadErrorsA);
    INFO("B->A read errors " << readErrorsB << " seq errors " << seqErrorsB
         << " payload errors " << payloadErrorsB);
    REQUIRE(readErrorsA + seqErrorsA + payloadErrorsA == 0);
    REQUIRE(readErrorsB + seqErrorsB + payloadErrorsB == 0);
}
