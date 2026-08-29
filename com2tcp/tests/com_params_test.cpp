/*
 * com2tcp/tests/com_params_test.cpp
 *
 * Tests for ComParams option parsing (utils.cxx).
 *
 * Covers: BaudRate defaults and parsing, ByteSize defaults and parsing,
 * Parity parsing for all five values, StopBits parsing for all three values,
 * ignoreDSR/connectDTR flags.
 *
 * See: com2tcp/utils.h — ComParams class declaration
 * See: com2tcp/utils.cxx — ComParams implementation
 */

#include "catch_amalgamated.hpp"
#include "precomp.h"

// ══════════════════════════════════════════════════════════════════
// Defaults
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ComParams defaults to baud 19200", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.BaudRate() == 19200);
}

TEST_CASE("ComParams defaults to 8 data bits", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.ByteSize() == 8);
}

TEST_CASE("ComParams defaults to no parity", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.Parity() == 0); // NOPARITY
}

TEST_CASE("ComParams defaults to 1 stop bit", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.StopBits() == 0); // ONESTOPBIT
}

TEST_CASE("ComParams defaults ignoreDSR to false", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.IgnoreDSR() == FALSE);
}

TEST_CASE("ComParams defaults connectDTR to false", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.ConnectDTR() == FALSE);
}

// ══════════════════════════════════════════════════════════════════
// Baud Rate
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ComParams SetBaudRate parses 9600", "[unit][comparams]")
{
    ComParams p;
    p.SetBaudRate("9600");
    REQUIRE(p.BaudRate() == 9600);
}

TEST_CASE("ComParams SetBaudRate parses 115200", "[unit][comparams]")
{
    ComParams p;
    p.SetBaudRate("115200");
    REQUIRE(p.BaudRate() == 115200);
}

// ══════════════════════════════════════════════════════════════════
// Byte Size
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ComParams SetByteSize parses 7", "[unit][comparams]")
{
    ComParams p;
    p.SetByteSize("7");
    REQUIRE(p.ByteSize() == 7);
}

TEST_CASE("ComParams SetByteSize parses 8", "[unit][comparams]")
{
    ComParams p;
    p.SetByteSize("8");
    REQUIRE(p.ByteSize() == 8);
}

// ══════════════════════════════════════════════════════════════════
// Parity
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ComParams SetParity no is valid", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("no") == TRUE);
    REQUIRE(p.Parity() == 0); // NOPARITY
}

TEST_CASE("ComParams SetParity n is valid shorthand", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("n") == TRUE);
    REQUIRE(p.Parity() == 0);
}

TEST_CASE("ComParams SetParity odd is valid", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("odd") == TRUE);
    REQUIRE(p.Parity() == 1); // ODDPARITY
}

TEST_CASE("ComParams SetParity o is valid shorthand", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("o") == TRUE);
    REQUIRE(p.Parity() == 1);
}

TEST_CASE("ComParams SetParity even is valid", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("even") == TRUE);
    REQUIRE(p.Parity() == 2); // EVENPARITY
}

TEST_CASE("ComParams SetParity e is valid shorthand", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("e") == TRUE);
    REQUIRE(p.Parity() == 2);
}

TEST_CASE("ComParams SetParity mark is valid", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("mark") == TRUE);
    REQUIRE(p.Parity() == 3); // MARKPARITY
}

TEST_CASE("ComParams SetParity m is valid shorthand", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("m") == TRUE);
    REQUIRE(p.Parity() == 3);
}

TEST_CASE("ComParams SetParity space is valid", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("space") == TRUE);
    REQUIRE(p.Parity() == 4); // SPACEPARITY
}

TEST_CASE("ComParams SetParity s is valid shorthand", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("s") == TRUE);
    REQUIRE(p.Parity() == 4);
}

TEST_CASE("ComParams SetParity default sets sentinel value -1", "[unit][comparams]")
{
    // The production code uses -1 as a sentinel meaning "driver default",
    // not the constructor default (NOPARITY=0).
    ComParams p;
    p.SetParity("odd");
    REQUIRE(p.SetParity("default") == TRUE);
    REQUIRE(p.Parity() == -1);
}

TEST_CASE("ComParams SetParity d shorthand sets sentinel value -1", "[unit][comparams]")
{
    ComParams p;
    p.SetParity("even");
    REQUIRE(p.SetParity("d") == TRUE);
    REQUIRE(p.Parity() == -1);
}

TEST_CASE("ComParams SetParity rejects invalid value", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetParity("invalid") == FALSE);
}

// ══════════════════════════════════════════════════════════════════
// Stop Bits
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ComParams SetStopBits 1 is valid", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetStopBits("1") == TRUE);
    REQUIRE(p.StopBits() == 0); // ONESTOPBIT
}

TEST_CASE("ComParams SetStopBits 1.5 is valid", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetStopBits("1.5") == TRUE);
    REQUIRE(p.StopBits() == 1); // ONE5STOPBITS
}

TEST_CASE("ComParams SetStopBits 2 is valid", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.SetStopBits("2") == TRUE);
    REQUIRE(p.StopBits() == 2); // TWOSTOPBITS
}

TEST_CASE("ComParams SetStopBits default sets sentinel value -1", "[unit][comparams]")
{
    // The production code uses -1 as a sentinel meaning "driver default",
    // not the constructor default (ONESTOPBIT=0).
    ComParams p;
    p.SetStopBits("2");
    REQUIRE(p.SetStopBits("default") == TRUE);
    REQUIRE(p.StopBits() == -1);
}

TEST_CASE("ComParams SetStopBits d shorthand sets sentinel value -1", "[unit][comparams]")
{
    ComParams p;
    p.SetStopBits("1.5");
    REQUIRE(p.SetStopBits("d") == TRUE);
    REQUIRE(p.StopBits() == -1);
}

TEST_CASE("ComParams SetStopBits rejects invalid value 3", "[unit][comparams]")
{
    ComParams p;
    // The switch only handles '1', '2', 'd'. '3' falls to default which returns FALSE.
    REQUIRE(p.SetStopBits("3") == FALSE);
    // Value unchanged
    REQUIRE(p.StopBits() == 0);
}

// ══════════════════════════════════════════════════════════════════
// Flags
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ComParams SetIgnoreDSR toggles correctly", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.IgnoreDSR() == FALSE);
    p.SetIgnoreDSR(TRUE);
    REQUIRE(p.IgnoreDSR() == TRUE);
    p.SetIgnoreDSR(FALSE);
    REQUIRE(p.IgnoreDSR() == FALSE);
}

TEST_CASE("ComParams SetConnectDTR toggles correctly", "[unit][comparams]")
{
    ComParams p;
    REQUIRE(p.ConnectDTR() == FALSE);
    p.SetConnectDTR(TRUE);
    REQUIRE(p.ConnectDTR() == TRUE);
    p.SetConnectDTR(FALSE);
    REQUIRE(p.ConnectDTR() == FALSE);
}

// ══════════════════════════════════════════════════════════════════
// Static string lists (verify they are non-empty)
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ComParams BaudRateLst returns non-empty string", "[unit][comparams]")
{
    const char *lst = ComParams::BaudRateLst();
    REQUIRE(lst != nullptr);
    REQUIRE(strlen(lst) > 0);
}

TEST_CASE("ComParams ByteSizeLst returns non-empty string", "[unit][comparams]")
{
    const char *lst = ComParams::ByteSizeLst();
    REQUIRE(lst != nullptr);
    REQUIRE(strlen(lst) > 0);
}

TEST_CASE("ComParams ParityLst returns non-empty string", "[unit][comparams]")
{
    const char *lst = ComParams::ParityLst();
    REQUIRE(lst != nullptr);
    REQUIRE(strlen(lst) > 0);
}

TEST_CASE("ComParams StopBitsLst returns non-empty string", "[unit][comparams]")
{
    const char *lst = ComParams::StopBitsLst();
    REQUIRE(lst != nullptr);
    REQUIRE(strlen(lst) > 0);
}
