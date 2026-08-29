#include "catch_amalgamated.hpp"
#include <windows.h>
#include "mocks/win32_overrides.h"
#include "mocks/registry_mock.h"
#include "precomp.h"
#define MessageBoxA Mock_MessageBoxA
extern "C" { int WINAPI Mock_MessageBoxA(void*,const char*,const char*,unsigned int){return 0;} }
#define TEXT_PREF
#include "../../include/com0com.h"
#define COMDB_MAX_PORTS_ARBITRATED 4096
#include "../comdb.cpp"

TEST_CASE("SaveComDbLocal then LoadComDbLocal round-trips", "[mock][comdb]")
{
    MockRegistry::Reset();
    BusyMask saved;
    saved.AddNum(0); saved.AddNum(3); saved.AddNum(7);
    REQUIRE(SaveComDbLocal(saved));
    BusyMask loaded;
    REQUIRE(LoadComDbLocal(loaded));
    REQUIRE_FALSE(loaded.IsFreeNum(0));
    REQUIRE(loaded.IsFreeNum(1));
    REQUIRE_FALSE(loaded.IsFreeNum(3));
    REQUIRE_FALSE(loaded.IsFreeNum(7));
}

TEST_CASE("LoadComDbLocal returns empty when no registry data", "[mock][comdb]")
{
    MockRegistry::Reset();
    BusyMask mask;
    REQUIRE(LoadComDbLocal(mask));
    REQUIRE(mask.IsFreeNum(0));
}

TEST_CASE("SaveComDbLocal empty mask deletes registry value", "[mock][comdb]")
{
    MockRegistry::Reset();
    BusyMask m1; m1.AddNum(0);
    REQUIRE(SaveComDbLocal(m1));
    BusyMask empty;
    REQUIRE(SaveComDbLocal(empty));
    std::vector<BYTE> data; DWORD type;
    REQUIRE_FALSE(MockRegistry::GetValue(
        "HKLM\\SYSTEM\\CurrentControlSet\\Services\\com0com\\COM Name Arbiter",
        "ComDB", &type, data));
}
