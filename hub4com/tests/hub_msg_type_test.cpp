/*
 * hub4com/tests/hub_msg_type_test.cpp
 *
 * Tests for HUB_MSG type constants and macros (plugins_api.h).
 *
 * Every HUB_MSG_TYPE_* constant is a DWORD that encodes three things:
 *   1. Type number (low byte, extracted by HUB_MSG_T2N)
 *   2. Union type (bits 24-31, masked by HUB_MSG_UNION_TYPES_MASK)
 *   3. Value type (bits 16-23, masked by HUB_MSG_VAL_TYPES_MASK)
 *
 * These macros are pure preprocessor constants. A single bit error in any
 * of them would corrupt protocol messages between hub and plugins.
 *
 * See: hub4com/plugins/plugins_api.h — the macro definitions
 */

// Windows.h must come before Catch2 to avoid macro conflicts
#include <windows.h>
#include "catch_amalgamated.hpp"

// Include the plugin API directly — it has its own extern "C" guards
#include "../plugins/plugins_api.h"

// ══════════════════════════════════════════════════════════════════
// Union type constants
// ══════════════════════════════════════════════════════════════════

TEST_CASE("HUB_MSG_UNION_TYPE_NONE is zero", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_UNION_TYPE_NONE == 0x00000000);
}

TEST_CASE("HUB_MSG_UNION_TYPE_BUF has correct mask bit", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_UNION_TYPE_BUF == 0x01000000);
}

TEST_CASE("HUB_MSG_UNION_TYPE_VAL has correct mask bit", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_UNION_TYPE_VAL == 0x02000000);
}

TEST_CASE("HUB_MSG_UNION_TYPES_MASK covers all union type bits", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_UNION_TYPES_MASK == 0xFF000000);
}

// ══════════════════════════════════════════════════════════════════
// Message type numbers (HUB_MSG_T2N extracts low byte)
// ══════════════════════════════════════════════════════════════════

TEST_CASE("HUB_MSG_T2N extracts type number from encoded type", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_T2N(HUB_MSG_TYPE_EMPTY) == 0);
    REQUIRE(HUB_MSG_T2N(HUB_MSG_TYPE_LINE_DATA) == 1);
    REQUIRE(HUB_MSG_T2N(HUB_MSG_TYPE_CONNECT) == 2);
    REQUIRE(HUB_MSG_T2N(HUB_MSG_TYPE_MODEM_STATUS) == 3);
    REQUIRE(HUB_MSG_T2N(HUB_MSG_TYPE_LINE_STATUS) == 4);
    REQUIRE(HUB_MSG_T2N(HUB_MSG_TYPE_TICK) == 24);
}

// ══════════════════════════════════════════════════════════════════
// Full encoded type values (verify no accidental renumbering)
// ══════════════════════════════════════════════════════════════════

TEST_CASE("HUB_MSG_TYPE_LINE_DATA has BUF union and type 1", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_TYPE_LINE_DATA == (1 | HUB_MSG_UNION_TYPE_BUF));
}

TEST_CASE("HUB_MSG_TYPE_CONNECT has VAL+Bool union and type 2", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_TYPE_CONNECT == (2 | HUB_MSG_UNION_TYPE_VAL | HUB_MSG_VAL_TYPE_BOOL));
}

TEST_CASE("HUB_MSG_TYPE_MODEM_STATUS has VAL+Mask union and type 3", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_TYPE_MODEM_STATUS == (3 | HUB_MSG_UNION_TYPE_VAL | HUB_MSG_VAL_TYPE_MASK_VAL));
}

TEST_CASE("HUB_MSG_TYPE_LOOP_TEST has HVAL union and type 20", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_TYPE_LOOP_TEST == (20 | HUB_MSG_UNION_TYPE_HVAL));
}

TEST_CASE("HUB_MSG_TYPE_TICK has HVAL2 union and type 24", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_TYPE_TICK == (24 | HUB_MSG_UNION_TYPE_HVAL2));
}

TEST_CASE("HUB_MSG_TYPE_PURGE_TX_IN has NONE union and type 22", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_TYPE_PURGE_TX_IN == (22 | HUB_MSG_UNION_TYPE_NONE));
}

// ══════════════════════════════════════════════════════════════════
// HUB_MSG_ROUTE_FLOW_CONTROL flag
// ══════════════════════════════════════════════════════════════════

TEST_CASE("HUB_MSG_ROUTE_FLOW_CONTROL is a flag in the upper word", "[unit][hubmsg]")
{
    REQUIRE(HUB_MSG_ROUTE_FLOW_CONTROL == 0x00008000);
}

TEST_CASE("HUB_MSG_TYPE_ADD_XOFF_XON includes flow control flag", "[unit][hubmsg]")
{
    REQUIRE((HUB_MSG_TYPE_ADD_XOFF_XON & HUB_MSG_ROUTE_FLOW_CONTROL) != 0);
}

// ══════════════════════════════════════════════════════════════════
// VAL2MASK / MASK2VAL round-trip
// ══════════════════════════════════════════════════════════════════

TEST_CASE("VAL2MASK and MASK2VAL round-trip for modem status values", "[unit][hubmsg]")
{
    WORD orig = 0x42;
    DWORD mask = VAL2MASK(orig);
    REQUIRE(mask == (DWORD(orig) << 16));
    WORD back = MASK2VAL(mask);
    REQUIRE(back == orig);
}

TEST_CASE("VAL2MASK and MASK2VAL handle zero", "[unit][hubmsg]")
{
    REQUIRE(MASK2VAL(VAL2MASK(0)) == 0);
}

TEST_CASE("VAL2MASK and MASK2VAL handle max WORD", "[unit][hubmsg]")
{
    REQUIRE(MASK2VAL(VAL2MASK(0xFFFF)) == 0xFFFF);
}

// ══════════════════════════════════════════════════════════════════
// Modem status mask constants (verify no overlap with masks)
// ══════════════════════════════════════════════════════════════════

TEST_CASE("MODEM_STATUS constants are distinct bits", "[unit][hubmsg]")
{
    // Each constant should be a single bit
    REQUIRE(MODEM_STATUS_DCTS == 0x01);
    REQUIRE(MODEM_STATUS_DDSR == 0x02);
    REQUIRE(MODEM_STATUS_TERI == 0x04);
    REQUIRE(MODEM_STATUS_DDCD == 0x08);
    REQUIRE(MODEM_STATUS_CTS  == 0x10);
    REQUIRE(MODEM_STATUS_DSR  == 0x20);
    REQUIRE(MODEM_STATUS_RI   == 0x40);
    REQUIRE(MODEM_STATUS_DCD  == 0x80);
}

// ══════════════════════════════════════════════════════════════════
// Pin state constants
// ══════════════════════════════════════════════════════════════════

TEST_CASE("SPS_V2P_MST maps modem status bits to pin state positions", "[unit][hubmsg]")
{
    // CTS modem status bit 0x10 maps to pin state 0x1000
    REQUIRE(PIN_STATE_CTS == 0x1000);
    REQUIRE(PIN_STATE_DSR == 0x2000);
    REQUIRE(PIN_STATE_RI  == 0x4000);
    REQUIRE(PIN_STATE_DCD == 0x8000);
}

TEST_CASE("SPS_P2V_MCR round-trips with SPS_V2P_MCR", "[unit][hubmsg]")
{
    // SPS_V2P_MCR packs a modem control register value into pin state.
    // SPS_P2V_MCR extracts the MCR value from pin state.
    BYTE mcr = 0x03; // DTR + RTS
    WORD pin = SPS_V2P_MCR(mcr);
    REQUIRE(pin == (PIN_STATE_DTR | PIN_STATE_RTS));
    BYTE back = SPS_P2V_MCR(pin);
    REQUIRE(back == mcr);
}
