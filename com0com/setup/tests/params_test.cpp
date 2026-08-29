/*
 * setup/tests/params_test.cpp
 *
 * Tests for parameter string parsing in setup.dll (params.cpp).
 *
 * Covers: ParseParametersStr() for all five types (FLAG, PIN, PROBABILITY,
 * UNSIGNED, OTHER), case insensitivity, the shortcut values "-" and "*",
 * error paths, edge cases, and the individual SetBit type dispatchers.
 *
 * See: setup/params.cpp — the implementation under test
 * See: setup/params.h — public interface declarations
 * See: include/com0com.h — C0C_PIN_* and C0C_PROBABILITY_* constants
 */

#include "catch_amalgamated.hpp"
#include <cstring>

// The test harness: include production sources with stubs.
// We include params.cpp directly so we can test static functions.
// A test subclass exposes protected SetBit/SetFlag/etc. for fine-grained tests.

// Stub the precompiled header — test_stubs.cpp provides Trace/SNPRINTF etc.
// We include <windows.h> for Win32 types used by params.h.
#include <windows.h>

// Define TEXT_PREF as empty (ANSI build) before including com0com.h
#define TEXT_PREF
#include "../include/com0com.h"

#include "../params.h"

// ── Test fixture: exposes protected methods for unit testing ───────

class TestPortParameters : public PortParameters
{
public:
    TestPortParameters() : PortParameters("com0com", "CNCA0") {}

    // Expose protected methods
    using PortParameters::SetBit;
    using PortParameters::SetFlag;
    using PortParameters::SetPin;
    using PortParameters::SetProbability;
    using PortParameters::SetUnsigned;
    using PortParameters::ParseParametersStr;

    // Accessors for internal state
    DWORD GetEmuBR() const { return emuBR; }
    DWORD GetEmuOverrun() const { return emuOverrun; }
    DWORD GetPlugInMode() const { return plugInMode; }
    DWORD GetExclusiveMode() const { return exclusiveMode; }
    DWORD GetHiddenMode() const { return hiddenMode; }
    DWORD GetAllDataBits() const { return allDataBits; }
    DWORD GetPinCTS() const { return pinCTS; }
    DWORD GetPinDSR() const { return pinDSR; }
    DWORD GetPinDCD() const { return pinDCD; }
    DWORD GetPinRI() const { return pinRI; }
    DWORD GetEmuNoise() const { return emuNoise; }
    DWORD GetAddRTTO() const { return addRTTO; }
    DWORD GetAddRITO() const { return addRITO; }
    DWORD GetMaskExplicit() const { return maskExplicit; }
    DWORD GetMaskChanged() const { return maskChanged; }
};

// ── ParseParametersStr tests ───────────────────────────────────────

TEST_CASE("ParseParametersStr returns ok with empty string", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr(""));
}

TEST_CASE("ParseParametersStr returns ok with single flag yes", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuBR=yes"));
    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
}

TEST_CASE("ParseParametersStr returns ok with single flag no", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuBR=no"));
    REQUIRE(p.GetEmuBR() == 0x00000000);
}

TEST_CASE("ParseParametersStr is case insensitive for keys", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("emubr=yes"));
    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
}

TEST_CASE("ParseParametersStr is case insensitive for yes values", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuBR=YES"));
    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
}

TEST_CASE("ParseParametersStr sets multiple flags", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuBR=yes,EmuOverrun=no,PlugInMode=yes"));
    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
    REQUIRE(p.GetEmuOverrun() == 0x00000000);
    REQUIRE(p.GetPlugInMode() == 0xFFFFFFFF);
}

TEST_CASE("ParseParametersStr sets pin value rrts", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("cts=rrts"));
    REQUIRE(p.GetPinCTS() == C0C_PIN_RRTS);
}

TEST_CASE("ParseParametersStr sets pin value rdtr", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("dsr=rdtr"));
    REQUIRE(p.GetPinDSR() == C0C_PIN_RDTR);
}

TEST_CASE("ParseParametersStr sets negated pin value", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("cts=!rrts"));
    REQUIRE(p.GetPinCTS() == (C0C_PIN_RRTS | C0C_PIN_NEGATIVE));
}

TEST_CASE("ParseParametersStr sets pin to ON", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("ri=on"));
    REQUIRE(p.GetPinRI() == C0C_PIN_ON);
}

TEST_CASE("ParseParametersStr sets pin lopen correctly", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("dcd=lopen"));
    REQUIRE(p.GetPinDCD() == C0C_PIN_LOPEN);
}

TEST_CASE("ParseParametersStr sets probability zero", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuNoise=0"));
    REQUIRE(p.GetEmuNoise() == 0);
}

TEST_CASE("ParseParametersStr sets probability 0.5", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuNoise=0.5"));
    REQUIRE(p.GetEmuNoise() == 50000000);
}

TEST_CASE("ParseParametersStr sets probability max value", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuNoise=0.99999999"));
    REQUIRE(p.GetEmuNoise() == 99999999);
}

TEST_CASE("ParseParametersStr sets unsigned small value", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("AddRTTO=42"));
    REQUIRE(p.GetAddRTTO() == 42);
}

TEST_CASE("ParseParametersStr sets unsigned zero", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("AddRITO=0"));
    REQUIRE(p.GetAddRITO() == 0);
}

TEST_CASE("ParseParametersStr dash shortcut produces correct FillParametersStr output", "[unit][params]")
{
    // After applying "-" to a modified parameter set, FillParametersStr
    // should show the default values, not the previously-set ones.
    // This is tested through the public API rather than direct member access.

    PortParameters p("com0com", "CNCA0");

    // Set non-default values
    REQUIRE(p.ParseParametersStr("EmuBR=yes,EmuOverrun=yes"));
    char buf1[256];
    REQUIRE(p.FillParametersStr(buf1, sizeof(buf1), true));
    // Should contain "yes" for the flags we set
    REQUIRE(strstr(buf1, "EmuBR=yes") != nullptr);
    REQUIRE(strstr(buf1, "EmuOverrun=yes") != nullptr);

    // Apply dash shortcut
    bool ok = p.ParseParametersStr("-");
    REQUIRE(ok);

    // After dash, FillParametersStr should show "no" (default)
    char buf2[256];
    REQUIRE(p.FillParametersStr(buf2, sizeof(buf2), true));
    REQUIRE(strstr(buf2, "EmuBR=no") != nullptr);
    REQUIRE(strstr(buf2, "EmuOverrun=no") != nullptr);
}

TEST_CASE("ParseParametersStr star shortcut preserves current values in output", "[unit][params]")
{
    PortParameters p("com0com", "CNCA0");

    REQUIRE(p.ParseParametersStr("EmuBR=yes,EmuOverrun=no"));
    char buf1[256];
    REQUIRE(p.FillParametersStr(buf1, sizeof(buf1), true));
    REQUIRE(strstr(buf1, "EmuBR=yes") != nullptr);
    REQUIRE(strstr(buf1, "EmuOverrun=no") != nullptr);

    // Star should be a no-op
    REQUIRE(p.ParseParametersStr("*"));

    char buf2[256];
    REQUIRE(p.FillParametersStr(buf2, sizeof(buf2), true));
    REQUIRE(strstr(buf2, "EmuBR=yes") != nullptr);
    REQUIRE(strstr(buf2, "EmuOverrun=no") != nullptr);
}

TEST_CASE("ParseParametersStr star shortcut keeps current values", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuBR=yes"));
    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);

    // Star should be a no-op for all values
    REQUIRE(p.ParseParametersStr("*"));
    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
}

TEST_CASE("ParseParametersStr trailing comma is ignored", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuBR=yes,"));
    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
}

TEST_CASE("ParseParametersStr leading comma is ignored", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr(",EmuBR=yes"));
    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
}

TEST_CASE("ParseParametersStr whitespace around value is not trimmed by implementation", "[unit][params]")
{
    // The production ParseParametersStr uses STRTOK_R which does not trim
    // whitespace. "EmuBR= yes " passes " yes " (with spaces) to SetFlag,
    // which does lstrcmpi(" yes ", "yes") — this comparison fails.
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("EmuBR= yes "));
}

TEST_CASE("ParseParametersStr mixed parameter types", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr(
        "EmuBR=yes,cts=!rrts,EmuNoise=0.001,AddRTTO=100"));

    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
    REQUIRE(p.GetPinCTS() == (C0C_PIN_RRTS | C0C_PIN_NEGATIVE));
    REQUIRE(p.GetEmuNoise() == 100000);  // 0.001 * 100,000,000
    REQUIRE(p.GetAddRTTO() == 100);
}

TEST_CASE("ParseParametersStr handles all 14 parameter keys", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr(
        "EmuBR=yes,EmuOverrun=no,PlugInMode=yes,ExclusiveMode=no,"
        "HiddenMode=yes,AllDataBits=no,"
        "cts=rrts,dsr=rdtr,dcd=lopen,ri=!on,"
        "EmuNoise=0.5,AddRTTO=10,AddRITO=20"));

    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
    REQUIRE(p.GetEmuOverrun() == 0);
    REQUIRE(p.GetPlugInMode() == 0xFFFFFFFF);
    REQUIRE(p.GetExclusiveMode() == 0);
    REQUIRE(p.GetHiddenMode() == 0xFFFFFFFF);
    REQUIRE(p.GetAllDataBits() == 0);
    REQUIRE(p.GetPinCTS() == C0C_PIN_RRTS);
    REQUIRE(p.GetPinDSR() == C0C_PIN_RDTR);
    REQUIRE(p.GetPinDCD() == C0C_PIN_LOPEN);
    REQUIRE(p.GetPinRI() == (C0C_PIN_ON | C0C_PIN_NEGATIVE));
    REQUIRE(p.GetEmuNoise() == 50000000);
    REQUIRE(p.GetAddRTTO() == 10);
    REQUIRE(p.GetAddRITO() == 20);
}

TEST_CASE("ParseParametersStr returns error for malformed entry without equals", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("garbage"));
}

TEST_CASE("ParseParametersStr returns error for unknown parameter key", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("UnknownKey=value"));
}

// ── SetFlag tests ──────────────────────────────────────────────────

// SetFlag is called via the SetBit dispatcher when a FLAG-type key is parsed.
// The test verifies the parse-through path: ParseParametersStr → SetBit → SetFlag.
// Direct SetFlag calls require a valid bit mask (non-zero), or GetDwPtr returns NULL.

TEST_CASE("SetFlag yes through ParseParametersStr sets value to 0xFFFFFFFF", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuBR=yes"));
    REQUIRE(p.GetEmuBR() == 0xFFFFFFFF);
}

TEST_CASE("SetFlag no through ParseParametersStr sets value to zero", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuBR=no"));
    REQUIRE(p.GetEmuBR() == 0x00000000);
}

// ── SetPin tests ───────────────────────────────────────────────────

TEST_CASE("SetPin rrts produces correct bitmask", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("cts=rrts"));
    REQUIRE(p.GetPinCTS() == C0C_PIN_RRTS);
}

TEST_CASE("SetPin with negation prefix produces inverted bitmask", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("cts=!rdtr"));
    REQUIRE(p.GetPinCTS() == (C0C_PIN_RDTR | C0C_PIN_NEGATIVE));
}

TEST_CASE("SetPin rejects unknown pin name", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("cts=xyz"));
}

TEST_CASE("SetPin empty value is error", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("cts="));
}

// ── SetProbability tests ───────────────────────────────────────────

TEST_CASE("SetProbability zero is valid", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuNoise=0"));
    REQUIRE(p.GetEmuNoise() == 0);
}

TEST_CASE("SetProbability 0.5 converts to 50000000", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuNoise=0.5"));
    REQUIRE(p.GetEmuNoise() == 50000000);
}

TEST_CASE("SetProbability max 8 decimal digits accepted", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("EmuNoise=0.99999999"));
    REQUIRE(p.GetEmuNoise() == 99999999);
}

TEST_CASE("SetProbability rejects no leading zero before decimal", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("EmuNoise=.5"));
}

TEST_CASE("SetProbability rejects non-numeric value", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("EmuNoise=abc"));
}

// ── SetUnsigned tests ──────────────────────────────────────────────

TEST_CASE("SetUnsigned accepts valid number", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("AddRTTO=42"));
    REQUIRE(p.GetAddRTTO() == 42);
}

TEST_CASE("SetUnsigned accepts zero", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE(p.ParseParametersStr("AddRITO=0"));
    REQUIRE(p.GetAddRITO() == 0);
}

TEST_CASE("SetUnsigned rejects negative number", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("AddRTTO=-1"));
}

TEST_CASE("SetUnsigned rejects non-numeric value", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("AddRTTO=abc"));
}

TEST_CASE("SetUnsigned rejects empty value", "[unit][params]")
{
    TestPortParameters p;
    REQUIRE_FALSE(p.ParseParametersStr("AddRTTO="));
}
