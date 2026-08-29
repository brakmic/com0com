# Changes

## 2026-07-30

### Driver fixes

- **8-bit default**: The driver previously defaulted to 7 data bits, even parity,
  and 1200 baud. Modern serial applications expect 8-N-1 at 9600. Fixed in the
  driver's port initialization code so ports default to 8-N-1 at 9600 baud.
  Applications that call `SetCommState` with explicit settings are unaffected.

- **ACCESS_DENIED on rapid open/close**: Repeated `CreateFile`/`CloseHandle`
  cycles on port pairs could fail with `STATUS_ACCESS_DENIED`. The open count
  decrement was happening asynchronously in the queued close handler. Fixed by
  moving it to the synchronous close path under the spinlock.

### Build system

- **WDK 10.0.26100 + Visual Studio 2026**: The driver now compiles with the
  latest Windows Driver Kit and Visual Studio 2026 toolchain. The project files
  include workarounds for WDK/VS version incompatibilities.

- **WiX v7 MSI installer**: A new Windows Installer package with a custom wizard
  UI. Supports interactive and silent install, feature selection (core files,
  setup GUI, port pair), Start Menu shortcuts, and driver setup automation.
  Replaces the legacy NSIS installer.

- **All user-mode C/C++ projects compile with MSBuild**: setup.dll, setupc.exe,
  hub4com.exe, com2tcp.exe, and all 16 filter plugins build via `msbuild` with
  the v145 toolset.

### Test suite

- **264 tests across 4 tiers**:
  - Tier 1: Unit tests for params, COM database, telnet protocol, hub messages
  - Tier 2: Mock-based tests for registry loading, port enumeration
  - Tier 3: Integration tests for setup.dll, com2tcp, hub4com
  - Tier 4: Null-modem data integrity tests against the running driver

- **Automated runner**: `scripts/run_tests.ps1` builds and runs all test suites.

- **Plugin validation**: All 16 hub4com plugins are validated for correct
  `LoadLibrary`, `InitA`, routine tables, types, and metadata.

### Documentation

- `docs/BUILDING.md`: build prerequisites, driver signing, test invocation
- `docs/ARCHITECTURE.md`: WDM driver model, hub4com filter pipeline, plugin API
- `docs/TESTING.md`: 4-tier test strategy, mock framework, driver setup
- `USERGUIDE.md`: port configuration parameters, setupg GUI reference,
  setupc command-line reference, example workflows

### Upgrade notes

The driver defaults changed from 7-E-1 at 1200 to 8-N-1 at 9600. Existing
installations with explicit `SetCommState` calls in applications are unaffected.
Installations that relied on the old 7-bit default may see different behavior.

The MSI installer replaces the NSIS installer. If an NSIS-based build is
installed, uninstall it first. The MSI uses a different install directory and
registers its own Start Menu shortcuts.
