#include "catch_amalgamated.hpp"
#include <windows.h>
#define TEXT_PREF
#include "../include/com0com.h"
#include "../params.h"

TEST_CASE("minimal dash test", "[debug]") {
    PortParameters p("com0com", "CNCA0");
    // Set something
    bool ok1 = p.ParseParametersStr("EmuBR=yes");
    printf("ParseParametersStr(yes) returned %d\n", ok1);
    
    // Try dash
    bool ok2 = p.ParseParametersStr("-");
    printf("ParseParametersStr(-) returned %d\n", ok2);
    
    // Now check via FillParametersStr
    char buf[256];
    p.FillParametersStr(buf, sizeof(buf), true);
    printf("After dash: [%s]\n", buf);
    
    REQUIRE(ok1);
    REQUIRE(ok2);
}
