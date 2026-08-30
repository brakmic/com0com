# Testing: com0com Test Suite

Last updated: 2026-08-29

## Overview

The test suite covers 266 tests across four executables. All tests are run with
`scripts\run_tests.ps1`.

| Test executable | Framework | Tests | What it covers |
|---|---|---|---|
| `setup_tests.exe` | Catch2 v3.12 | 84 | setup.dll, comdb, mocks, integration, null-modem |
| `com2tcp_tests.exe` | Catch2 v3.12 | 60 | Telnet RFC 2217, COM parameters, integration |
| `hub4com_tests.exe` | Catch2 v3.12 | 64 | HubMsg types, routes, filters, 16 plugins |
| `setupg.Tests.dll` | xUnit | 58 | C# setup GUI logic, SetupCommand, SetupOutputParser |

## Quick Start

```
# Build and run all tests
.\scripts\run_tests.ps1

# Run without rebuilding
.\scripts\run_tests.ps1 -SkipBuild

# Run individual suites
com0com\build\tests\Debug\setup_tests.exe
com0com\build\tests\Debug\com2tcp_tests.exe
com0com\build\tests\Debug\hub4com_tests.exe
dotnet test com0com\setupg.Tests\
```

## Test Tiers

### Tier 1: Unit Tests (no OS dependencies)

Test individual functions and classes without external state. Built with the
v143 toolset, no driver or admin privileges required.

**setup.dll:**
- `params_test.cpp`: Port parameter parsing and validation
- `comdb_test.cpp`: COM port database load/save/query

**com2tcp:**
- `telnet_protocol_test.cpp`: TelnetProtocol IAC state machine, escaping, option negotiation
- `com_params_test.cpp`: Command-line COM parameter parsing

**hub4com:**
- `hub_msg_type_test.cpp`: HubMsg type system, union type flags
- `routine_valid_test.cpp`: `ROUTINE_IS_VALID` macro boundary tests
- `go_so_test.cpp`: GO/SO option parsing

**C# setupg:**
- `FakeSetupCommand`: Command parsing, output parsing, port pair data model

### Tier 2: Mock Tests (mocked OS calls)

Test functions that call Win32 APIs by redirecting them to in-memory mocks.
Uses preprocessor redefinition in `win32_overrides.h` to replace registry,
file I/O, and SetupAPI calls.

- `mock_registry_test.cpp`: In-memory registry mock validation
- `mock_comdb_test.cpp`: `LoadComDbLocal()` / `SaveComDbLocal()` round-trip
  with the in-memory registry mock. Production `comdb.cpp` is included
  directly with `win32_overrides.h` applied.

### Tier 3: Integration Tests (real EXE/DLL)

Test real executables and DLLs by loading them and calling their entry points.

- `main_a_test.cpp`: `MainA()` calls via `LoadLibrary` on `setup.dll` (list, help, busynames)
- `com2tcp_integration_test.cpp`: `com2tcp.exe --help` output validation
- `hub4com_integration_test.cpp`: `hub4com.exe --help` and plugin loading

### Tier 4: Driver Tests (requires kernel driver)

Test the running com0com driver through actual COM port I/O. Requires:
- Administrator privileges
- Test signing enabled (`bcdedit /set testsigning on`)
- A CNCA0↔CNCB0 port pair created

- `null_modem_test.cpp`:
  - A→B text, B→A text, bidirectional data transfer
  - Binary data round-trip (bytes 0x00, 0xFF, 0x80, 0x7F, etc.)
  - 3-cycle stress test
  - DCB default verification (8-N-1 @ 9600)
  - Rapid open/close cycle test (20 iterations)

- `plugin_config_test.cpp`:
  - All 16 hub4com plugins: `LoadLibrary("InitA")` + routines table validation
  - Plugin type verification (FILTER vs DRIVER)
  - About metadata (name, copyright, license)
  - Config parsing for serial, tcp, and trace driver plugins

### C# Integration Tests

- `SetupCommandIntegrationTests.cs`: Real `setupc.exe` calls (list, help, invalid command, ListAll, quit)

## Driver Test Setup

Driver tests use the `[driver]` Catch2 tag. The main runner
`scripts\run_tests.ps1` always excludes them. Run them with
`scripts\run_driver_tests.ps1` from an elevated shell.

Before running driver tests:

```
# Create test ports
cd com0com
setupc --silent install 0 PortName=CNCA0 PortName=CNCB0

# Run driver tests only
setup_tests.exe "[driver]"
```

## Mock Framework

Tests in Tier 2 use a preprocessor-based mock framework in
`com0com\setup\tests\mocks\`.

`win32_overrides.h` redefines Win32 API functions to mock equivalents:

```c
#define RegOpenKeyExA  Mock_RegOpenKeyExA
#define RegQueryValueExA Mock_RegQueryValueExA
#define CreateFileA    Mock_CreateFileA
```

Each mock provides an in-memory implementation. `registry_mock.cpp` stores
keys and values in a `std::map`. `comdb_mock.cpp` stores the COM port busy
mask in a file-scope global.

The production source files (e.g. `comdb.cpp`) are compiled directly into the
test binary. The linker resolves all symbols from the mock objects first,
effectively replacing the real OS calls.

Constraints: `extern "C"` functions cannot access C++ class private members.
Mock state (e.g. `MockRegistry::s_store`) is declared public.

## Test Runner

`scripts\run_tests.ps1` builds and runs all tests. Key options:

```powershell
.\scripts\run_tests.ps1              # Build + run all
.\scripts\run_tests.ps1 -SkipBuild   # Run without rebuilding
```

The script produces a summary of pass/fail counts per test executable.
Exit code 0 means all tests passed.

## Adding New Tests

1. Add the `.cpp` file to the relevant `tests\` directory.
2. Add it to the `.vcxproj` `<ClCompile>` list.
3. For Catch2 tests: use `TEST_CASE("description", "[tags]")`.
4. For driver tests: use the `[driver]` tag so the runner can gate on admin privileges.
5. Build with MSBuild as usual. The test project references the shared Catch2
   amalgamation in `com0com\setup\tests\catch_amalgamated.cpp`.
