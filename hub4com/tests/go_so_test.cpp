/*
 * hub4com/tests/go_so_test.cpp
 *
 * Tests for GO (Get Options) and SO (Set Output) option packing macros.
 *
 * These macros pack/extract option values into/from DWORD bitfields. They are
 * used during hub startup to negotiate capabilities between ports. A single
 * shift error in any of them would cause protocol mismatch.
 *
 * All macros are in plugins_api.h. Tests verify round-trip correctness:
 * original → packed → extracted → same as original.
 *
 * See: hub4com/plugins/plugins_api.h — GO_*, SO_*, ESC_OPTS_*, LC_* macros
 */

#include "catch_amalgamated.hpp"
#include <windows.h>

#include "../plugins/plugins_api.h"

// ══════════════════════════════════════════════════════════════════
// GO_O2I / GO_I2O — set-ID packing (2 bits in bits 30-31)
// ══════════════════════════════════════════════════════════════════

TEST_CASE("GO_O2I then GO_I2O round-trips set ID 0", "[unit][go_so]")
{
    DWORD set0 = GO0_ESCAPE_MODE | GO0_LBR_STATUS;
    DWORD packedSetId = GO_O2I(set0);
    REQUIRE(packedSetId == 0); // set 0 has ID 0
    DWORD back = GO_I2O(packedSetId) | set0; // reconstruct
    REQUIRE((GO_O2I(back)) == 0);
}

TEST_CASE("GO set ID 2 round-trips correctly", "[unit][go_so]")
{
    // Any value with set ID 2 in bits 30-31
    DWORD val = GO_I2O(2) | GO1_RBR_STATUS;
    REQUIRE(GO_O2I(val) == 2);
}

TEST_CASE("GO set ID 3 round-trips correctly", "[unit][go_so]")
{
    DWORD val = GO_I2O(3) | GO1_RLC_STATUS;
    REQUIRE(GO_O2I(val) == 3);
}

// ══════════════════════════════════════════════════════════════════
// GO1 modem status / line status round-trips
// ══════════════════════════════════════════════════════════════════

TEST_CASE("GO1 modem status mask round-trips CTS|DSR", "[unit][go_so]")
{
    // V2O packs a value INTO the options DWORD.
    // O2V extracts a value FROM the options DWORD.
    // Round-trip: pack → extract → same value.
    BYTE orig = MODEM_STATUS_CTS | MODEM_STATUS_DSR; // 0x30
    DWORD packed = GO1_V2O_MODEM_STATUS(orig);
    BYTE back = GO1_O2V_MODEM_STATUS(packed);
    REQUIRE(back == orig);
}

TEST_CASE("GO1 modem status mask round-trips all bits", "[unit][go_so]")
{
    BYTE orig = 0xFF;
    DWORD packed = GO1_V2O_MODEM_STATUS(orig);
    BYTE back = GO1_O2V_MODEM_STATUS(packed);
    REQUIRE(back == orig);
}

TEST_CASE("GO1 line status mask round-trips OE|PE|FE", "[unit][go_so]")
{
    // Line status mask lives in bits 8-15 of GO1 options DWORD
    BYTE orig = LINE_STATUS_OE | LINE_STATUS_PE | LINE_STATUS_FE; // 0x0E
    DWORD packed = GO1_V2O_LINE_STATUS(orig);
    BYTE back = GO1_O2V_LINE_STATUS(packed);
    REQUIRE(back == orig);
}

TEST_CASE("GO1 line status mask round-trips zero", "[unit][go_so]")
{
    REQUIRE(GO1_O2V_LINE_STATUS(GO1_V2O_LINE_STATUS(0)) == 0);
}

// ══════════════════════════════════════════════════════════════════
// GO1 individual bit flags
// ══════════════════════════════════════════════════════════════════

TEST_CASE("GO1_RBR_STATUS is distinct from GO1_RLC_STATUS", "[unit][go_so]")
{
    REQUIRE(GO1_RBR_STATUS != GO1_RLC_STATUS);
    REQUIRE(GO1_RBR_STATUS != GO1_BREAK_STATUS);
    REQUIRE(GO1_RBR_STATUS != GO1_PURGE_TX_IN);
}

TEST_CASE("GO1 flags are in the upper 16 bits of the DWORD", "[unit][go_so]")
{
    REQUIRE(GO1_RBR_STATUS == 0x00010000);
    REQUIRE(GO1_RLC_STATUS == 0x00020000);
    REQUIRE(GO1_BREAK_STATUS == 0x00040000);
    REQUIRE(GO1_PURGE_TX_IN == 0x00080000);
}

// ══════════════════════════════════════════════════════════════════
// SO pin state / line status round-trips
// ══════════════════════════════════════════════════════════════════

TEST_CASE("SO pin state round-trips DTR|RTS", "[unit][go_so]")
{
    WORD orig = PIN_STATE_DTR | PIN_STATE_RTS; // 0x0003
    DWORD packed = SO_O2V_PIN_STATE(orig);
    WORD back = SO_V2O_PIN_STATE(packed);
    REQUIRE(back == orig);
}

TEST_CASE("SO pin state round-trips all output pins", "[unit][go_so]")
{
    WORD orig = PIN_STATE_DTR | PIN_STATE_RTS | PIN_STATE_OUT1 | PIN_STATE_OUT2;
    DWORD packed = SO_O2V_PIN_STATE(orig);
    WORD back = SO_V2O_PIN_STATE(packed);
    REQUIRE(back == orig);
}

TEST_CASE("SO line status mask round-trips BI|FIFOERR", "[unit][go_so]")
{
    // Line status mask in SO is in bits 16-23
    BYTE orig = LINE_STATUS_BI | LINE_STATUS_FIFOERR; // 0x90
    DWORD packed = SO_V2O_LINE_STATUS(orig);
    BYTE back = SO_O2V_LINE_STATUS(packed);
    REQUIRE(back == orig);
}

// ══════════════════════════════════════════════════════════════════
// SO capability flags
// ══════════════════════════════════════════════════════════════════

TEST_CASE("SO_SET_BR, SO_SET_LC, SO_PURGE_TX are distinct", "[unit][go_so]")
{
    REQUIRE(SO_SET_BR != SO_SET_LC);
    REQUIRE(SO_SET_BR != SO_PURGE_TX);
    REQUIRE(SO_SET_LC != SO_PURGE_TX);
}

TEST_CASE("SO flags are in bits 24-26", "[unit][go_so]")
{
    REQUIRE(SO_SET_BR   == 0x01000000);
    REQUIRE(SO_SET_LC   == 0x02000000);
    REQUIRE(SO_PURGE_TX == 0x04000000);
}

TEST_CASE("SO flags do not overlap with pin state or line status fields", "[unit][go_so]")
{
    // Pin state is in bits 0-15
    DWORD pinField = SO_O2V_PIN_STATE(PIN_STATE_DTR | PIN_STATE_RTS | PIN_STATE_OUT1 | PIN_STATE_OUT2 | PIN_STATE_BREAK);
    REQUIRE((pinField & SO_SET_BR) == 0);
    REQUIRE((pinField & SO_SET_LC) == 0);
    REQUIRE((pinField & SO_PURGE_TX) == 0);

    // Line status is in bits 16-23
    DWORD lsField = SO_O2V_LINE_STATUS(0xFF);
    REQUIRE((lsField & SO_SET_BR) == 0);
    REQUIRE((lsField & SO_SET_LC) == 0);
    REQUIRE((lsField & SO_PURGE_TX) == 0);
}

// ══════════════════════════════════════════════════════════════════
// Escape options
// ══════════════════════════════════════════════════════════════════

TEST_CASE("ESC_OPTS_MAP_EO_2_GO1 and ESC_OPTS_MAP_GO1_2_EO round-trip", "[unit][go_so]")
{
    DWORD orig = GO1_RBR_STATUS | GO1_RLC_STATUS;
    DWORD mapped = ESC_OPTS_MAP_EO_2_GO1(orig);
    REQUIRE(mapped == orig); // lower 24 bits preserved
    DWORD back = ESC_OPTS_MAP_GO1_2_EO(mapped);
    REQUIRE(back == orig);
}

TEST_CASE("ESC_OPTS escape char encoding round-trips", "[unit][go_so]")
{
    BYTE ch = 0x7E; // typical escape char
    DWORD packed = ESC_OPTS_V2O_ESCCHAR(ch);
    REQUIRE((packed >> 24) == ch);
    BYTE back = ESC_OPTS_O2V_ESCCHAR(packed);
    REQUIRE(back == ch);
}

// ══════════════════════════════════════════════════════════════════
// Line control (LC) packing
// ══════════════════════════════════════════════════════════════════

TEST_CASE("LC byte size round-trips 8 bits", "[unit][go_so]")
{
    BYTE orig = 8;
    DWORD packed = VAL2LC_BYTESIZE(orig);
    BYTE back = LC2VAL_BYTESIZE(packed);
    REQUIRE(back == orig);
}

TEST_CASE("LC byte size round-trips 5 bits", "[unit][go_so]")
{
    BYTE orig = 5;
    DWORD packed = VAL2LC_BYTESIZE(orig);
    BYTE back = LC2VAL_BYTESIZE(packed);
    REQUIRE(back == orig);
}

TEST_CASE("LC parity round-trips SPACEPARITY(4)", "[unit][go_so]")
{
    BYTE orig = 4;
    DWORD packed = VAL2LC_PARITY(orig);
    BYTE back = LC2VAL_PARITY(packed);
    REQUIRE(back == orig);
}

TEST_CASE("LC stop bits round-trips TWOSTOPBITS(2)", "[unit][go_so]")
{
    BYTE orig = 2;
    DWORD packed = VAL2LC_STOPBITS(orig);
    BYTE back = LC2VAL_STOPBITS(packed);
    REQUIRE(back == orig);
}

TEST_CASE("LC combined fields do not overlap", "[unit][go_so]")
{
    DWORD bs = VAL2LC_BYTESIZE(8);
    DWORD parity = VAL2LC_PARITY(0); // NOPARITY
    DWORD sb = VAL2LC_STOPBITS(0);   // ONESTOPBIT

    // Byte size in bits 0-7, parity 8-15, stop bits 16-23
    REQUIRE((bs & parity) == 0);
    REQUIRE((bs & sb) == 0);
    REQUIRE((parity & sb) == 0);
}

TEST_CASE("LC mask constants match the field positions", "[unit][go_so]")
{
    REQUIRE(LC_MASK_BYTESIZE == 0x01000000);
    REQUIRE(LC_MASK_PARITY   == 0x02000000);
    REQUIRE(LC_MASK_STOPBITS == 0x04000000);
}
