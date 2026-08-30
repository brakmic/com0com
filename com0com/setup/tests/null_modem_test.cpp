#include "catch_amalgamated.hpp"
#include <windows.h>
#include <ntddser.h>
#include <cstdio>
#include <cstring>

#include "../../include/cncext.h"

static const char *PORT_A = "\\\\.\\CNCA0";
static const char *PORT_B = "\\\\.\\CNCB0";

static HANDLE OpenPort(const char *pName)
{
    HANDLE h = CreateFileA(pName, GENERIC_READ|GENERIC_WRITE, 0,
                           NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        FAIL("CreateFile(" << pName << ") failed: " << GetLastError());
        return h;
    }

    // Driver now defaults to 8-N-1 (8 data bits, no parity, 1 stop bit).
    // Keep the DCB query as documentation — verify the default is correct.
    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        FAIL("GetCommState failed: " << GetLastError());
        CloseHandle(h); return INVALID_HANDLE_VALUE;
    }
    REQUIRE(dcb.ByteSize == 8);
    REQUIRE(dcb.Parity == NOPARITY);
    REQUIRE(dcb.StopBits == ONESTOPBIT);
    REQUIRE(dcb.BaudRate == 9600);

    COMMTIMEOUTS to = {0};
    to.ReadIntervalTimeout = MAXDWORD;
    to.ReadTotalTimeoutMultiplier = MAXDWORD;
    to.ReadTotalTimeoutConstant = MAXDWORD - 1;
    if (h != INVALID_HANDLE_VALUE && !SetCommTimeouts(h, &to)) {
        FAIL("SetCommTimeouts failed: " << GetLastError());
        CloseHandle(h); return INVALID_HANDLE_VALUE;
    }
    return h;
}

static bool WritePort(HANDLE h, const BYTE *p, DWORD sz)
{
    OVERLAPPED ov = {0};
    ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (!ov.hEvent) return false;
    DWORD w=0; BOOL ok=WriteFile(h,p,sz,&w,&ov);
    DWORD err=GetLastError();
    if (!ok && err==ERROR_IO_PENDING) {
        if (WaitForSingleObject(ov.hEvent,5000)==WAIT_OBJECT_0) ok=GetOverlappedResult(h,&ov,&w,FALSE);
        else { CancelIo(h); ok=FALSE; }
    }
    CloseHandle(ov.hEvent);
    INFO("WritePort ok=" << (int)ok << " written=" << w << " expected=" << sz << " err=" << err);
    return ok && w==sz;
}

static int ReadPort(HANDLE h, BYTE *b, DWORD sz, DWORD ms)
{
    // Loop until all requested bytes are read or timeout.
    DWORD total = 0;
    DWORD start = GetTickCount();
    while (total < sz) {
        DWORD remaining = ms;
        DWORD elapsed = GetTickCount() - start;
        if (elapsed >= ms) break;
        remaining = ms - elapsed;

        OVERLAPPED ov = {0};
        ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
        if (!ov.hEvent) return -1;
        DWORD r=0; BOOL ok=ReadFile(h,b+total,sz-total,&r,&ov);
        if (!ok && GetLastError()==ERROR_IO_PENDING) {
            if (WaitForSingleObject(ov.hEvent,remaining)==WAIT_OBJECT_0) ok=GetOverlappedResult(h,&ov,&r,FALSE);
            else { CancelIo(h); ok=FALSE; r=0; }
        }
        CloseHandle(ov.hEvent);
        if (!ok || r==0) break;
        total += r;
    }
    return (int)total;
}

TEST_CASE("Null-modem CNCA0↔CNCB0 passes all integrity checks", "[integration][driver]")
{
    HANDLE hA = OpenPort(PORT_A);
    REQUIRE(hA != INVALID_HANDLE_VALUE);
    HANDLE hB = OpenPort(PORT_B);
    REQUIRE(hB != INVALID_HANDLE_VALUE);

    // A→B text
    const char *a2b = "Hello from A"; DWORD la = (DWORD)strlen(a2b);
    REQUIRE(WritePort(hA,(const BYTE*)a2b,la));
    BYTE ba[64]={0}; int na=ReadPort(hB,ba,sizeof(ba),2000);
    REQUIRE(na==(int)la); REQUIRE(memcmp(ba,a2b,la)==0);

    // B→A text
    const char *b2a = "Hello from B"; DWORD lb = (DWORD)strlen(b2a);
    REQUIRE(WritePort(hB,(const BYTE*)b2a,lb));
    BYTE bb[64]={0}; int nb=ReadPort(hA,bb,sizeof(bb),2000);
    REQUIRE(nb==(int)lb); REQUIRE(memcmp(bb,b2a,lb)==0);

    // Bidirectional
    const char *x="A->B",*y="B->A"; DWORD lx=(DWORD)strlen(x),ly=(DWORD)strlen(y);
    REQUIRE(WritePort(hA,(const BYTE*)x,lx));
    REQUIRE(WritePort(hB,(const BYTE*)y,ly));
    BYTE bx[64]={0},by[64]={0};
    int nx=ReadPort(hB,bx,sizeof(bx),2000);
    REQUIRE(nx==(int)lx); REQUIRE(memcmp(bx,x,lx)==0);
    int ny=ReadPort(hA,by,sizeof(by),2000);
    REQUIRE(ny==(int)ly); REQUIRE(memcmp(by,y,ly)==0);

    // Binary with zeros
    BYTE bin[]={0x00,0xFF,0x7F,0x80,0x01,0x00,0xFE,0x55,0xAA};
    DWORD bl=sizeof(bin);
    REQUIRE(WritePort(hA,bin,bl));
    BYTE bz[64]={0}; int nz=ReadPort(hB,bz,sizeof(bz),3000);
    REQUIRE(nz==(int)bl);
    bool match = (memcmp(bz,bin,bl)==0);
    if (!match) {
        for(DWORD ii=0; ii<(DWORD)nz && ii<bl; ii++)
            WARN("byte[" << ii << "] exp=0x" << std::hex << (int)bin[ii] << " got=0x" << (int)bz[ii]);
    }
    REQUIRE(match);

    // Three cycles with small delays
    for(int i=0;i<3;i++){
        char m[32]; _snprintf(m,sizeof(m),"Cycle %d",i);
        DWORD lm=(DWORD)strlen(m);
        REQUIRE(WritePort(hA,(const BYTE*)m,lm));
        BYTE bm[64]={0}; int nm=ReadPort(hB,bm,sizeof(bm),3000);
        INFO("Cycle " << i << " read " << nm << " bytes, expected " << lm);
        REQUIRE(nm==(int)lm); REQUIRE(memcmp(bm,m,lm)==0);
        Sleep(10); // small gap between cycles
    }

    CancelIo(hA); CancelIo(hB);
    FlushFileBuffers(hA); FlushFileBuffers(hB);
    CloseHandle(hA); CloseHandle(hB);
}

TEST_CASE("Rapid open/close does not cause ACCESS_DENIED", "[driver]")
{
    // Regression test for DEFECT-002: rapid CreateFile/CloseHandle cycles
    // on a port pair used to return error 5 because the close path
    // queued the IRP and openCount was decremented asynchronously.
    // The fix moves the openCount decrement to FdoPortClose (synchronous).

    for (int cycle = 0; cycle < 20; cycle++) {
        HANDLE hA = OpenPort(PORT_A);
        HANDLE hB = OpenPort(PORT_B);

        // Quick data check to verify ports are functional
        const char *msg = "OK";
        REQUIRE(WritePort(hA, (const BYTE*)msg, 2));
        BYTE buf[4] = {0};
        int nr = ReadPort(hB, buf, sizeof(buf), 3000);
        REQUIRE(nr == 2);
        REQUIRE(memcmp(buf, "OK", 2) == 0);

        FlushFileBuffers(hA); FlushFileBuffers(hB);
        CloseHandle(hA);
        CloseHandle(hB);
        // Immediate re-open happens at top of next iteration
    }
    SUCCEED("20 rapid open/close cycles completed without ACCESS_DENIED");
}

TEST_CASE("Extended GET_MODEM_CONTROL places signature at protocol offset", "[driver]")
{
    // Regression test for the x64 buffer overrun where the extended
    // IOCTL_SERIAL_GET_MODEM_CONTROL response copied the signature at
    // sizeof(PULONG) instead of sizeof(ULONG). The wire format is
    // ULONG modem control followed by the c0c signature.
    static const ULONG sizes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16, 32, 64};

    HANDLE h = OpenPort(PORT_A);

    BYTE inBuf[C0CE_SIGNATURE_SIZE];
    memcpy(inBuf, C0CE_SIGNATURE, C0CE_SIGNATURE_SIZE);

    for (ULONG outSize : sizes) {
        CAPTURE(outSize);

        BYTE outBuf[64];
        memset(outBuf, 0xCC, sizeof(outBuf));
        DWORD returned = 0;
        BOOL ok = DeviceIoControl(h, IOCTL_SERIAL_GET_MODEM_CONTROL,
                                  inBuf, sizeof(inBuf),
                                  outBuf, outSize, &returned, NULL);
        if (outSize < sizeof(ULONG)) {
            REQUIRE(!ok);
            REQUIRE(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
            continue;
        }

        REQUIRE(ok);
        if (outSize >= sizeof(ULONG) + C0CE_SIGNATURE_SIZE) {
            REQUIRE(returned == outSize);
            REQUIRE(memcmp(outBuf + sizeof(ULONG), C0CE_SIGNATURE,
                           C0CE_SIGNATURE_SIZE) == 0);
            for (DWORD i = sizeof(ULONG) + C0CE_SIGNATURE_SIZE; i < outSize; i++)
                REQUIRE(outBuf[i] == 0);
        } else {
            REQUIRE(returned == sizeof(ULONG));
        }
    }

    CloseHandle(h);
}

static BYTE *PutULONG(BYTE *p, ULONG v)
{
    memcpy(p, &v, sizeof(v));
    return p + sizeof(v);
}

TEST_CASE("Extended LSRMST_INSERT protocol preserves byte layout", "[driver]")
{
    // Byte-level protocol checks for the extended LSRMST_INSERT exchange:
    // input is UCHAR escape + c0c signature + ULONG options, output is
    // either c0c + ULONG capabilities or a stream of insert records.
    HANDLE h = OpenPort(PORT_A);

    // CAPS query.
    BYTE capsIn[1 + C0CE_SIGNATURE_SIZE + sizeof(ULONG)];
    capsIn[0] = 0;
    memcpy(capsIn + 1, C0CE_SIGNATURE, C0CE_SIGNATURE_SIZE);
    PutULONG(capsIn + 1 + C0CE_SIGNATURE_SIZE, C0CE_INSERT_IOCTL_CAPS);

    BYTE out[64];
    DWORD returned = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_SERIAL_LSRMST_INSERT,
                              capsIn, sizeof(capsIn), out, sizeof(out),
                              &returned, NULL);
    REQUIRE(ok);
    REQUIRE(returned == C0CE_SIGNATURE_SIZE + sizeof(ULONG));
    REQUIRE(memcmp(out, C0CE_SIGNATURE, C0CE_SIGNATURE_SIZE) == 0);
    ULONG caps = 0;
    memcpy(&caps, out + C0CE_SIGNATURE_SIZE, sizeof(caps));
    REQUIRE(caps == (C0CE_INSERT_IOCTL_GET | C0CE_INSERT_IOCTL_RXCLEAR |
                     C0CE_INSERT_ENABLE_LSR | C0CE_INSERT_ENABLE_MST |
                     C0CE_INSERT_ENABLE_RBR | C0CE_INSERT_ENABLE_RLC |
                     C0CE_INSERT_ENABLE_LSR_BI));

    // GET with RBR only. Layout: escape, C0CE_INSERT_RBR, ULONG baud.
    BYTE getIn[1 + C0CE_SIGNATURE_SIZE + sizeof(ULONG)];
    getIn[0] = 0;
    memcpy(getIn + 1, C0CE_SIGNATURE, C0CE_SIGNATURE_SIZE);
    PutULONG(getIn + 1 + C0CE_SIGNATURE_SIZE,
             C0CE_INSERT_IOCTL_GET | C0CE_INSERT_ENABLE_RBR);

    memset(out, 0xCC, sizeof(out));
    returned = 0;
    ok = DeviceIoControl(h, IOCTL_SERIAL_LSRMST_INSERT,
                         getIn, sizeof(getIn), out, sizeof(out),
                         &returned, NULL);
    REQUIRE(ok);
    REQUIRE(returned == sizeof(UCHAR) * 2 + sizeof(ULONG));
    REQUIRE(out[0] == 0);
    REQUIRE(out[1] == C0CE_INSERT_RBR);
    ULONG baud = 0;
    memcpy(&baud, out + 2, sizeof(baud));
    REQUIRE(baud == 9600);

    // GET with all insertions. Layout:
    //   LSR: escape, SERIAL_LSRMST_LSR_NODATA, lsr byte
    //   MST: escape, SERIAL_LSRMST_MST, modem status byte
    //   RBR: escape, C0CE_INSERT_RBR, ULONG baud
    //   RLC: escape, C0CE_INSERT_RLC, word length, parity, stop bits
    PutULONG(getIn + 1 + C0CE_SIGNATURE_SIZE,
             C0CE_INSERT_IOCTL_GET | C0CE_INSERT_ENABLE_LSR |
             C0CE_INSERT_ENABLE_MST | C0CE_INSERT_ENABLE_RBR |
             C0CE_INSERT_ENABLE_RLC | C0CE_INSERT_ENABLE_LSR_BI);

    memset(out, 0xCC, sizeof(out));
    returned = 0;
    ok = DeviceIoControl(h, IOCTL_SERIAL_LSRMST_INSERT,
                         getIn, sizeof(getIn), out, sizeof(out),
                         &returned, NULL);
    REQUIRE(ok);
    REQUIRE(returned == sizeof(UCHAR) * 2 + sizeof(UCHAR) +
                        sizeof(UCHAR) * 2 + sizeof(UCHAR) +
                        sizeof(UCHAR) * 2 + sizeof(ULONG) +
                        sizeof(UCHAR) * 2 + sizeof(UCHAR) * 3);
    BYTE *p = out;
    REQUIRE(*p++ == 0);
    REQUIRE(*p++ == SERIAL_LSRMST_LSR_NODATA);
    p++; // lsr byte varies with TX state
    REQUIRE(*p++ == 0);
    REQUIRE(*p++ == SERIAL_LSRMST_MST);
    p++; // modem status byte varies
    REQUIRE(*p++ == 0);
    REQUIRE(*p++ == C0CE_INSERT_RBR);
    baud = 0;
    memcpy(&baud, p, sizeof(baud));
    p += sizeof(baud);
    REQUIRE(baud == 9600);
    REQUIRE(*p++ == 0);
    REQUIRE(*p++ == C0CE_INSERT_RLC);
    REQUIRE(*p++ == 8);          // word length
    REQUIRE(*p++ == NO_PARITY);  // parity
    REQUIRE(*p++ == STOP_BIT_1); // stop bits

    // Unknown option bits are rejected.
    PutULONG(getIn + 1 + C0CE_SIGNATURE_SIZE, C0CE_INSERT_ENABLE_RBR | 0x80000000);
    returned = 0;
    ok = DeviceIoControl(h, IOCTL_SERIAL_LSRMST_INSERT,
                         getIn, sizeof(getIn), out, sizeof(out),
                         &returned, NULL);
    REQUIRE(!ok);
    REQUIRE(GetLastError() == ERROR_INVALID_PARAMETER);

    CloseHandle(h);
}
