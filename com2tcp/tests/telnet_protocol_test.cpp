/*
 * com2tcp/tests/telnet_protocol_test.cpp
 *
 * Tests for the TelnetProtocol state machine (telnet.cpp).
 *
 * Covers: IAC state transitions (stData→stCode→stOption→stSubParams→stSubCode),
 * IAC escaping in Send(), option negotiation for Echo and Terminal Type,
 * sub-negotiation response, unknown option handling, Clean() reset.
 *
 * See: com2tcp/telnet.cpp — the implementation under test
 * See: com2tcp/telnet.h — TelnetProtocol class declaration
 * See: com2tcp/utils.h — Protocol base class, DataStream
 */

#include "catch_amalgamated.hpp"
#include "precomp.h"
#include "../telnet.h"

#include <cstring>

// Telnet protocol constants from telnet.cpp
enum {
    cdSE   = 240,
    cdSB   = 250,
    cdWILL = 251,
    cdWONT = 252,
    cdDO   = 253,
    cdDONT = 254,
    cdIAC  = 255,
};

enum {
    opEcho         = 1,
    opTerminalType = 24,
};

// Helper: read all available data from the send side of a Protocol.
// This reads from streamSendRead (what the protocol outputs).
static int ReadAllSend(Protocol &proto, BYTE *buf, int maxSize)
{
    return proto.Read(buf, maxSize);
}

// Helper: read all available data from the write side of a Protocol.
static int ReadAllWrite(Protocol &proto, BYTE *buf, int maxSize)
{
    return proto.Recv(buf, maxSize);
}

// ══════════════════════════════════════════════════════════════════
// IAC State Machine Tests (Write side: TCP → COM direction)
// ══════════════════════════════════════════════════════════════════

TEST_CASE("TelnetProtocol Write passes plain data through unchanged", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    BYTE data[] = { 'H', 'e', 'l', 'l', 'o' };
    REQUIRE(proto.Write(data, sizeof(data)) == 5);

    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 5);
    REQUIRE(memcmp(out, "Hello", 5) == 0);
}

TEST_CASE("TelnetProtocol Write passes data bytes that are not IAC", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    BYTE data[] = { 0x01, 0x7F, 0x80, 0xFE };
    REQUIRE(proto.Write(data, sizeof(data)) == 4);

    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 4);
    REQUIRE(out[0] == 0x01);
    REQUIRE(out[1] == 0x7F);
    REQUIRE(out[2] == 0x80);
    REQUIRE(out[3] == 0xFE);
}

TEST_CASE("TelnetProtocol Write enters stCode on IAC byte", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // Send IAC to enter stCode state
    BYTE data[] = { cdIAC };
    REQUIRE(proto.Write(data, 1) == 1);

    // No output should be produced yet (IAC consumed, waiting for code)
    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 0);
}

TEST_CASE("TelnetProtocol Write handles IAC IAC escaped byte", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // IAC IAC should produce a single 0xFF in output
    BYTE data[] = { cdIAC, cdIAC };
    REQUIRE(proto.Write(data, 2) == 2);

    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 1);
    REQUIRE(out[0] == cdIAC);
}

TEST_CASE("TelnetProtocol Write returns to stData after IAC IAC", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // IAC IAC 'A' — escaped IAC then plain data
    BYTE data[] = { cdIAC, cdIAC, 'A' };
    REQUIRE(proto.Write(data, 3) == 3);

    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 2);
    REQUIRE(out[0] == cdIAC);
    REQUIRE(out[1] == 'A');
}

TEST_CASE("TelnetProtocol Write handles IAC WILL known option", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // IAC WILL ECHO — remote wants to do echo
    BYTE data[] = { cdIAC, cdWILL, opEcho };
    REQUIRE(proto.Write(data, 3) == 3);

    // No data output expected, but a response should be on the Send side
    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 0);

    // The protocol should respond with DO ECHO (since remoteOptionState was osNo → osYes)
    BYTE send[64];
    n = ReadAllSend(proto, send, sizeof(send));
    REQUIRE(n == 3);
    REQUIRE(send[0] == cdIAC);
    REQUIRE(send[1] == cdDO);
    REQUIRE(send[2] == opEcho);
}

TEST_CASE("TelnetProtocol WILL then WONT sequence completes without error", "[unit][telnet]")
{
    // Verify that a WILL→WONT sequence for the same option doesn't crash.
    TelnetProtocol proto(10, 10);

    BYTE will[] = { cdIAC, cdWILL, opEcho };
    REQUIRE(proto.Write(will, 3) == 3);

    BYTE wont[] = { cdIAC, cdWONT, opEcho };
    REQUIRE(proto.Write(wont, 3) == 3);

    // After WONT, plain data should still pass through
    BYTE data[] = { 'X' };
    REQUIRE(proto.Write(data, 1) == 1);

    BYTE out[64];
    int m = proto.Recv(out, sizeof(out));
    REQUIRE(m == 1);
    REQUIRE(out[0] == 'X');
}

TEST_CASE("TelnetProtocol Write handles IAC DO known option", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // IAC DO Terminal Type — remote asks us to do terminal type
    BYTE data[] = { cdIAC, cdDO, opTerminalType };
    REQUIRE(proto.Write(data, 3) == 3);

    // Should respond with WILL Terminal Type
    BYTE send[64];
    int n = ReadAllSend(proto, send, sizeof(send));
    REQUIRE(n == 3);
    REQUIRE(send[0] == cdIAC);
    REQUIRE(send[1] == cdWILL);
    REQUIRE(send[2] == opTerminalType);
}

TEST_CASE("TelnetProtocol DO then DONT sequence completes without error", "[unit][telnet]")
{
    // Verify that a DO→DONT sequence for the same option doesn't crash.
    TelnetProtocol proto(10, 10);

    BYTE doTT[] = { cdIAC, cdDO, opTerminalType };
    REQUIRE(proto.Write(doTT, 3) == 3);

    BYTE dont[] = { cdIAC, cdDONT, opTerminalType };
    REQUIRE(proto.Write(dont, 3) == 3);

    // After DONT, plain data should still pass through
    BYTE data[] = { 'Y' };
    REQUIRE(proto.Write(data, 1) == 1);

    BYTE out[64];
    int m = proto.Recv(out, sizeof(out));
    REQUIRE(m == 1);
    REQUIRE(out[0] == 'Y');
}

TEST_CASE("TelnetProtocol Write handles IAC DO unknown option with WONT", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // Unknown option 99, localOptionState is osCant
    BYTE data[] = { cdIAC, cdDO, 99 };
    REQUIRE(proto.Write(data, 3) == 3);

    BYTE send[64];
    int n = ReadAllSend(proto, send, sizeof(send));
    REQUIRE(n == 3);
    REQUIRE(send[0] == cdIAC);
    REQUIRE(send[1] == cdWONT);
    REQUIRE(send[2] == 99);
}

TEST_CASE("TelnetProtocol Write handles IAC WILL unknown option with DONT", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // Unknown option 99, remoteOptionState is osCant
    BYTE data[] = { cdIAC, cdWILL, 99 };
    REQUIRE(proto.Write(data, 3) == 3);

    BYTE send[64];
    int n = ReadAllSend(proto, send, sizeof(send));
    REQUIRE(n == 3);
    REQUIRE(send[0] == cdIAC);
    REQUIRE(send[1] == cdDONT);
    REQUIRE(send[2] == 99);
}

TEST_CASE("TelnetProtocol Write handles IAC SB Terminal Type SEND sub-negotiation", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    proto.SetTerminalType("VT100");

    // First establish Terminal Type capability: DO → WILL
    BYTE doTT[] = { cdIAC, cdDO, opTerminalType };
    proto.Write(doTT, 3);
    // Read the WILL response
    BYTE tmp[64];
    ReadAllSend(proto, tmp, sizeof(tmp)); // consume WILL

    // IAC SB TerminalType SEND IAC SE
    BYTE data[] = { cdIAC, cdSB, opTerminalType, 1, cdIAC, cdSE };
    REQUIRE(proto.Write(data, 6) == 6);

    // Should respond with IAC SB TerminalType IS "VT100" IAC SE
    BYTE send[256];
    int n = ReadAllSend(proto, send, sizeof(send));
    REQUIRE(n >= 6);
    // IAC SB TerminalType 0 'V' 'T' '1' '0' '0' IAC SE
    REQUIRE(send[0] == cdIAC);
    REQUIRE(send[1] == cdSB);
    REQUIRE(send[2] == opTerminalType);
    REQUIRE(send[3] == 0);
    // Check terminal type string follows
    bool foundV = false, foundT = false;
    for (int i = 4; i < n - 2; i++) {
        if (send[i] == 'V') foundV = true;
        if (send[i] == 'T') foundT = true;
    }
    REQUIRE(foundV);
    REQUIRE(foundT);
    // End with IAC SE
    REQUIRE(send[n - 2] == cdIAC);
    REQUIRE(send[n - 1] == cdSE);
}

TEST_CASE("TelnetProtocol Write handles partial IAC at end of buffer", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // Send only IAC — state goes to stCode, waiting for next byte
    BYTE data[] = { cdIAC };
    REQUIRE(proto.Write(data, 1) == 1);

    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 0); // nothing output yet

    // Now send the second byte: IAC (escaped)
    BYTE data2[] = { cdIAC };
    REQUIRE(proto.Write(data2, 1) == 1);

    n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 1);
    REQUIRE(out[0] == cdIAC);
}

TEST_CASE("TelnetProtocol Write handles unknown code after IAC", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // IAC followed by unknown code (e.g. 0) returns to stData
    BYTE data[] = { cdIAC, 0 };
    REQUIRE(proto.Write(data, 2) == 2);

    // State should be back to stData — following data passes through
    BYTE data2[] = { 'X' };
    REQUIRE(proto.Write(data2, 1) == 1);

    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 1);
    REQUIRE(out[0] == 'X');
}

// ══════════════════════════════════════════════════════════════════
// IAC Escaping Tests (Send side: COM → TCP direction)
// ══════════════════════════════════════════════════════════════════

TEST_CASE("TelnetProtocol Send passes plain data through", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    BYTE data[] = { 'A', 'B', 'C' };
    REQUIRE(proto.Send(data, 3) == 3);

    BYTE out[64];
    int n = ReadAllSend(proto, out, sizeof(out));
    REQUIRE(n == 3);
    REQUIRE(out[0] == 'A');
    REQUIRE(out[1] == 'B');
    REQUIRE(out[2] == 'C');
}

TEST_CASE("TelnetProtocol Send escapes single IAC byte", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    BYTE data[] = { cdIAC };
    REQUIRE(proto.Send(data, 1) == 1);

    BYTE out[64];
    int n = ReadAllSend(proto, out, sizeof(out));
    REQUIRE(n == 2);
    REQUIRE(out[0] == cdIAC);
    REQUIRE(out[1] == cdIAC);
}

TEST_CASE("TelnetProtocol Send escapes IAC surrounded by data", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    BYTE data[] = { 'A', cdIAC, 'B' };
    REQUIRE(proto.Send(data, 3) == 3);

    BYTE out[64];
    int n = ReadAllSend(proto, out, sizeof(out));
    // A, IAC, IAC, B — the IAC is doubled
    REQUIRE(n == 4);
    REQUIRE(out[0] == 'A');
    REQUIRE(out[1] == cdIAC);
    REQUIRE(out[2] == cdIAC);
    REQUIRE(out[3] == 'B');
}

TEST_CASE("TelnetProtocol Send escapes two consecutive IAC bytes", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    BYTE data[] = { cdIAC, cdIAC };
    REQUIRE(proto.Send(data, 2) == 2);

    BYTE out[64];
    int n = ReadAllSend(proto, out, sizeof(out));
    REQUIRE(n == 4);
    REQUIRE(out[0] == cdIAC);
    REQUIRE(out[1] == cdIAC);
    REQUIRE(out[2] == cdIAC);
    REQUIRE(out[3] == cdIAC);
}

TEST_CASE("TelnetProtocol Send does not double option negotiation bytes", "[unit][telnet]")
{
    // Send option bytes are inserted via SendRaw which bypasses Send() escaping.
    // This test verifies that SendOption writes raw IAC WILL/DONT etc.
    TelnetProtocol proto(10, 10);

    // Trigger an option response to verify SendOption uses SendRaw
    BYTE data[] = { cdIAC, cdDO, opEcho }; // unknown echo request
    proto.Write(data, 3);

    BYTE send[64];
    int n = ReadAllSend(proto, send, sizeof(send));
    // Should be IAC WONT Echo (3 bytes, not doubled)
    REQUIRE(n == 3);
    REQUIRE(send[0] == cdIAC);
    REQUIRE(send[1] == cdWONT);
    REQUIRE(send[2] == opEcho);
}

// ══════════════════════════════════════════════════════════════════
// Clean() Reset Tests
// ══════════════════════════════════════════════════════════════════

TEST_CASE("TelnetProtocol Clean resets state to stData", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // Put protocol into stCode by sending partial IAC
    BYTE data[] = { cdIAC };
    proto.Write(data, 1);

    proto.Clean();

    // After Clean, plain data should pass through (state is stData)
    BYTE data2[] = { 'Z' };
    REQUIRE(proto.Write(data2, 1) == 1);

    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 1);
    REQUIRE(out[0] == 'Z');
}

TEST_CASE("TelnetProtocol Clean resets option states", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);

    // Accept echo
    BYTE data[] = { cdIAC, cdWILL, opEcho };
    proto.Write(data, 3);
    BYTE tmp[64];
    // Consume DO response
    ReadAllSend(proto, tmp, sizeof(tmp));

    proto.Clean();

    // After Clean, a new WILL Echo should be treated as fresh negotiation
    BYTE data2[] = { cdIAC, cdWILL, opEcho };
    proto.Write(data2, 3);

    BYTE send[64];
    int n = ReadAllSend(proto, send, sizeof(send));
    // Should get DO Echo (osNo → osYes for remote echo)
    REQUIRE(n == 3);
    REQUIRE(send[0] == cdIAC);
    REQUIRE(send[1] == cdDO);
    REQUIRE(send[2] == opEcho);
}

TEST_CASE("TelnetProtocol Clean resets data streams", "[unit][telnet]")
{
    TelnetProtocol proto(10, 10);
    // Put some data through
    BYTE data[] = { 'H', 'i' };
    proto.Write(data, 2);

    proto.Clean();

    // Streams should be empty after Clean
    BYTE out[64];
    int n = ReadAllWrite(proto, out, sizeof(out));
    REQUIRE(n == 0);
}
