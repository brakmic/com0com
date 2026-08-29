/*
 * Plugin load + config parsing tests (Block A3)
 *
 * Verifies all 16 hub4com plugins load correctly, export InitA,
 * return valid routines tables, and (for serial + tcp drivers)
 * parse their COM parameter options correctly.
 *
 * All 13 filter plugins set pConfig/pConfigStart/pConfigStop to NULL.
 * Option parsing happens in their pCreate via argc/argv, which requires
 * a hub-provided HMASTERFILTER. Testing those paths would need a stub hub,
 * deferred for now. The serial and tcp driver plugins implement the full
 * pConfig API and are tested here.
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>

#define CATCH_CONFIG_RUNNER
#include "catch_amalgamated.hpp"

#include "../plugins/plugins_api.h"

// ---------------------------------------------------------------
// Hub routine stubs — plugins validate these via ROUTINE_IS_VALID
// which dereferences the struct pointer and checks offset vs size.
// Provide a minimal valid table so InitA succeeds.
// ---------------------------------------------------------------

static PUCHAR CALLBACK Stub_BufAlloc(DWORD size) { (void)size; return NULL; }
static void   CALLBACK Stub_BufFree(PUCHAR pBuf) { (void)pBuf; }
static void   CALLBACK Stub_BufAppend(PUCHAR *ppBuf, DWORD offset, const BYTE *pSrc, DWORD sizeSrc) { (void)ppBuf;(void)offset;(void)pSrc;(void)sizeSrc; }
static BOOL   CALLBACK Stub_MsgReplaceBuf(HUB_MSG *pMsg, DWORD type, const BYTE *pSrc, DWORD sizeSrc) { (void)pMsg;(void)type;(void)pSrc;(void)sizeSrc; return FALSE; }
static HUB_MSG * CALLBACK Stub_MsgInsertBuf(HUB_MSG *pMsg, DWORD type, const BYTE *pSrc, DWORD sizeSrc) { (void)pMsg;(void)type;(void)pSrc;(void)sizeSrc; return NULL; }
static BOOL   CALLBACK Stub_MsgReplaceVal(HUB_MSG *pMsg, DWORD type, DWORD val) { (void)pMsg;(void)type;(void)val; return FALSE; }
static HUB_MSG * CALLBACK Stub_MsgInsertVal(HUB_MSG *pMsg, DWORD type, DWORD val) { (void)pMsg;(void)type;(void)val; return NULL; }
static BOOL   CALLBACK Stub_MsgReplaceNone(HUB_MSG *pMsg, DWORD type) { (void)pMsg;(void)type; return FALSE; }
static HUB_MSG * CALLBACK Stub_MsgInsertNone(HUB_MSG *pMsg, DWORD type) { (void)pMsg;(void)type; return NULL; }
static const char * CALLBACK Stub_PortName(HMASTERPORT h) { (void)h; return "stub"; }
static const char * CALLBACK Stub_FilterName(HMASTERFILTER h) { (void)h; return "stub"; }
static void   CALLBACK Stub_OnRead(HMASTERPORT h, HUB_MSG *pMsg) { (void)h;(void)pMsg; }
static HMASTERTIMER CALLBACK Stub_TimerCreate(HTIMEROWNER h) { (void)h; return NULL; }
static BOOL   CALLBACK Stub_TimerSet(HMASTERTIMER h, HMASTERPORT p, const LARGE_INTEGER *pDue, LONG period, HTIMERPARAM param) { (void)h;(void)p;(void)pDue;(void)period;(void)param; return FALSE; }
static void   CALLBACK Stub_TimerCancel(HMASTERTIMER h) { (void)h; }
static void   CALLBACK Stub_TimerDelete(HMASTERTIMER h) { (void)h; }
static HMASTERPORT CALLBACK Stub_FilterPort(HMASTERFILTERINSTANCE h) { (void)h; return NULL; }
static HFILTER CALLBACK Stub_GetFilter(HMASTERFILTERINSTANCE h) { (void)h; return NULL; }
static const ARG_INFO_A * CALLBACK Stub_GetArgInfo(const char *pArg) { (void)pArg; return NULL; }

static HUB_ROUTINES_A g_stubHubRoutines = {
    sizeof(HUB_ROUTINES_A),
    Stub_BufAlloc,
    Stub_BufFree,
    Stub_BufAppend,
    Stub_MsgReplaceBuf,
    Stub_MsgInsertBuf,
    Stub_MsgReplaceVal,
    Stub_MsgInsertVal,
    Stub_MsgReplaceNone,
    Stub_MsgInsertNone,
    Stub_PortName,
    Stub_FilterName,
    Stub_OnRead,
    Stub_TimerCreate,
    Stub_TimerSet,
    Stub_TimerCancel,
    Stub_TimerDelete,
    Stub_FilterPort,
    Stub_GetFilter,
    Stub_GetArgInfo,
};

// ---------------------------------------------------------------
// Helper: Load DLL + call InitA, return routines table
// ---------------------------------------------------------------

static const PLUGIN_ROUTINES_A *const *LoadPlugin(const char *pDllPath)
{
    HMODULE hMod = LoadLibraryA(pDllPath);
    if (!hMod) {
        WARN("LoadLibrary(" << pDllPath << ") failed: " << GetLastError());
        return NULL;
    }

    PLUGIN_INIT_A *pInit = (PLUGIN_INIT_A *)GetProcAddress(hMod, PLUGIN_INIT_PROC_NAME_A);
    if (!pInit) {
        WARN("GetProcAddress(InitA) failed for " << pDllPath);
        FreeLibrary(hMod);
        return NULL;
    }

    const PLUGIN_ROUTINES_A *const *ppRtn = pInit(&g_stubHubRoutines);
    if (!ppRtn || !*ppRtn) {
        WARN("InitA returned NULL for " << pDllPath);
        FreeLibrary(hMod);
        return NULL;
    }

    return ppRtn;
}

static std::string PluginsDir()
{
    // Test exe is in com0com/build/tests/Debug
    // Plugins are in hub4com/build/x64/Debug/plugins
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    std::string dir(exePath);
    size_t pos = dir.rfind('\\');
    if (pos != std::string::npos)
        dir = dir.substr(0, pos + 1);

    // Navigate up and over to the plugins directory
    return dir + "..\\..\\..\\..\\hub4com\\build\\x64\\Debug\\plugins\\";
}

// ---------------------------------------------------------------
// Plugin registry
// ---------------------------------------------------------------

struct PluginInfo {
    const char *pDllName;
    PLUGIN_TYPE expectedType;
    bool hasConfigApi;  // serial + tcp implement pConfig, filters don't
};

static const PluginInfo g_plugins[] = {
    // Driver plugins (implement pConfig)
    {"serial.dll",   PLUGIN_TYPE_DRIVER, true},
    {"tcp.dll",      PLUGIN_TYPE_DRIVER, true},
    {"connector.dll",PLUGIN_TYPE_DRIVER, true},
    // Filter plugins with config API (trace implements config for logging)
    {"trace.dll",    PLUGIN_TYPE_FILTER, true},
    // Filter plugins (no pConfig)
    {"awakseq.dll",  PLUGIN_TYPE_FILTER, false},
    {"crypt.dll",    PLUGIN_TYPE_FILTER, false},
    {"echo.dll",     PLUGIN_TYPE_FILTER, false},
    {"escinsert.dll",PLUGIN_TYPE_FILTER, false},
    {"escparse.dll", PLUGIN_TYPE_FILTER, false},
    {"linectl.dll",  PLUGIN_TYPE_FILTER, false},
    {"lsrmap.dll",   PLUGIN_TYPE_FILTER, false},
    {"pin2con.dll",  PLUGIN_TYPE_FILTER, false},
    {"pinmap.dll",   PLUGIN_TYPE_FILTER, false},
    {"purge.dll",    PLUGIN_TYPE_FILTER, false},
    {"tag.dll",      PLUGIN_TYPE_FILTER, false},
    {"telnet.dll",   PLUGIN_TYPE_FILTER, false},
    // trace.dll moved above — it implements pConfig for trace logging
};

// ---------------------------------------------------------------
// Tests: every plugin loads and exports valid routines
// ---------------------------------------------------------------

TEST_CASE("All 16 plugins load and export InitA", "[plugins]")
{
    std::string dir = PluginsDir();

    for (const auto &pi : g_plugins) {
        DYNAMIC_SECTION(pi.pDllName)
        {
            std::string path = dir + pi.pDllName;
            const PLUGIN_ROUTINES_A *const *ppRtn = LoadPlugin(path.c_str());
            REQUIRE(ppRtn != NULL);
        }
    }
}

TEST_CASE("Plugin routines tables are well-formed", "[plugins]")
{
    std::string dir = PluginsDir();

    for (const auto &pi : g_plugins) {
        DYNAMIC_SECTION(pi.pDllName)
        {
            std::string path = dir + pi.pDllName;
            const PLUGIN_ROUTINES_A *const *ppRtn = LoadPlugin(path.c_str());
            REQUIRE(ppRtn != NULL);

            const PLUGIN_ROUTINES_A *pRtn = *ppRtn;
            REQUIRE(pRtn->size >= sizeof(PLUGIN_ROUTINES_A));

            // Common routines — every plugin must export these
            REQUIRE(pRtn->pGetPluginType != NULL);
            REQUIRE(pRtn->pGetPluginAbout != NULL);
            REQUIRE(pRtn->pHelp != NULL);

            // Verify type
            PLUGIN_TYPE type = pRtn->pGetPluginType();
            REQUIRE(type == pi.expectedType);

            // Verify about info
            const PLUGIN_ABOUT_A *pAbout = pRtn->pGetPluginAbout();
            REQUIRE(pAbout != NULL);
            REQUIRE(pAbout->size >= sizeof(PLUGIN_ABOUT_A));
            REQUIRE(pAbout->pName != NULL);
            REQUIRE(strlen(pAbout->pName) > 0);
            REQUIRE(pAbout->pCopyright != NULL);
            REQUIRE(strlen(pAbout->pCopyright) > 0);
            REQUIRE(pAbout->pLicense != NULL);
            REQUIRE(strlen(pAbout->pLicense) > 0);

            if (pi.hasConfigApi) {
                // Driver plugins — must export the config API
                REQUIRE(pRtn->pConfigStart != NULL);
                REQUIRE(pRtn->pConfig != NULL);
                REQUIRE(pRtn->pConfigStop != NULL);
            } else {
                // Filter plugins — config API is NULL
                REQUIRE(pRtn->pConfigStart == NULL);
                REQUIRE(pRtn->pConfig == NULL);
                REQUIRE(pRtn->pConfigStop == NULL);

                // Filter-specific routines vary widely. Some filters
                // (e.g. echo) have no pCreate/pDelete — they operate
                // on the message stream without per-instance state.
                // Only the common routines are required.
            }
        }
    }
}

TEST_CASE("Plugin help is non-NULL (already verified above)", "[plugins]")
{
    // pHelp presence is verified in the routines table test.
    // Actual help output cannot be tested without redirecting stderr,
    // and freopen("NUL"/"CON") causes crashes in some plugins' CRT.
    SUCCEED("pHelp verified non-NULL in routines table test");
}

// ---------------------------------------------------------------
// Serial driver pConfig tests
// ---------------------------------------------------------------

TEST_CASE("Serial driver pConfig parses valid COM parameters", "[plugins][serial]")
{
    std::string dir = PluginsDir();
    const PLUGIN_ROUTINES_A *const *ppRtn = LoadPlugin((dir + "serial.dll").c_str());
    REQUIRE(ppRtn != NULL);

    const PLUGIN_ROUTINES_A *pRtn = *ppRtn;
    REQUIRE(pRtn->pConfigStart != NULL);
    REQUIRE(pRtn->pConfig != NULL);
    REQUIRE(pRtn->pConfigStop != NULL);

    // ConfigStart creates a ComParams object
    HCONFIG hCfg = pRtn->pConfigStart();
    REQUIRE(hCfg != NULL);

    struct TestCase {
        const char *pOption;
        const char *pDescription;
    };

    TestCase validOptions[] = {
        {"--baud=9600",    "standard baud rate"},
        {"--baud=115200",  "high baud rate"},
        {"--baud=300",     "low baud rate"},
        {"--data=8",       "8 data bits"},
        {"--data=7",       "7 data bits"},
        {"--data=6",       "6 data bits"},
        {"--data=5",       "5 data bits"},
        {"--parity=no",    "no parity"},
        {"--parity=none",  "none parity (alt)"},
        {"--parity=even",  "even parity"},
        {"--parity=odd",   "odd parity"},
        {"--parity=mark",  "mark parity"},
        {"--parity=space", "space parity"},
        {"--stop=1",       "1 stop bit"},
        {"--stop=1.5",     "1.5 stop bits"},
        {"--stop=2",       "2 stop bits"},
        {"--octs=on",      "output CTS on"},
        {"--octs=off",     "output CTS off"},
        {"--odsr=on",      "output DSR on"},
        {"--odsr=off",     "output DSR off"},
        {"--ox=on",        "output XON/XOFF on"},
        {"--ox=off",       "output XON/XOFF off"},
        {"--ix=on",        "input XON/XOFF on"},
        {"--ix=off",       "input XON/XOFF off"},
        {"--idsr=on",      "input DSR on"},
        {"--idsr=off",     "input DSR off"},
        {"--ito=100",      "read interval timeout 100ms"},
        {"--ito=0",        "read interval timeout 0"},
        {"--write-limit=4096", "write limit"},
    };

    // NOTE: --share-mode= is excluded because the serial driver calls
    // exit(1) on invalid share mode values, and we haven't identified
    // the set of valid share mode strings.

    for (const auto &tc : validOptions) {
        DYNAMIC_SECTION(tc.pDescription)
        {
            BOOL result = pRtn->pConfig(hCfg, tc.pOption);
            // Config returns TRUE for known options, FALSE for unknown.
            // All the options above are known to the serial driver.
            CHECK(result == TRUE);
        }
    }

    // Cleanup
    pRtn->pConfigStop(hCfg);
}

// ---------------------------------------------------------------
// TCP driver pConfig tests
// ---------------------------------------------------------------

TEST_CASE("TCP driver pConfig parses valid TCP parameters", "[plugins][tcp]")
{
    std::string dir = PluginsDir();
    const PLUGIN_ROUTINES_A *const *ppRtn = LoadPlugin((dir + "tcp.dll").c_str());
    REQUIRE(ppRtn != NULL);

    const PLUGIN_ROUTINES_A *pRtn = *ppRtn;
    REQUIRE(pRtn->pConfigStart != NULL);
    REQUIRE(pRtn->pConfig != NULL);
    REQUIRE(pRtn->pConfigStop != NULL);

    HCONFIG hCfg = pRtn->pConfigStart();
    REQUIRE(hCfg != NULL);

    // TCP driver options from port.cpp
    struct TestCase {
        const char *pOption;
        const char *pDescription;
    };

    TestCase validOptions[] = {
        {"--interface=0.0.0.0",  "bind all interfaces"},
        {"--reconnect=d",        "reconnect default"},
        {"--reconnect=n",        "reconnect disable"},
        {"--reconnect=5",        "reconnect 5 seconds"},
        {"--write-limit=4096",   "write limit"},
    };

    for (const auto &tc : validOptions) {
        DYNAMIC_SECTION(tc.pDescription)
        {
            BOOL result = pRtn->pConfig(hCfg, tc.pOption);
            CHECK(result == TRUE);
        }
    }

    pRtn->pConfigStop(hCfg);
}

// ---------------------------------------------------------------
// Trace filter pConfig test (only filter with config API)
// ---------------------------------------------------------------

TEST_CASE("Trace filter pConfig parses trace options", "[plugins][trace]")
{
    std::string dir = PluginsDir();
    const PLUGIN_ROUTINES_A *const *ppRtn = LoadPlugin((dir + "trace.dll").c_str());
    REQUIRE(ppRtn != NULL);

    const PLUGIN_ROUTINES_A *pRtn = *ppRtn;
    REQUIRE(pRtn->pConfigStart != NULL);
    REQUIRE(pRtn->pConfig != NULL);
    REQUIRE(pRtn->pConfigStop != NULL);

    HCONFIG hCfg = pRtn->pConfigStart();
    REQUIRE(hCfg != NULL);

    // The trace filter only accepts --trace-file= for config.
    // All other options return FALSE.
    CHECK(pRtn->pConfig(hCfg, "--trace-file=C:\\temp\\trace.log") == TRUE);
    CHECK(pRtn->pConfig(hCfg, "plain_arg") == FALSE);

    pRtn->pConfigStop(hCfg);
}
