# Building com0com from Source

Last updated: 2026-07-30

## Prerequisites

| Component | Version | How to Install |
|---|---|---|
| Visual Studio 2026 Community | 18.8.2 | [visualstudio.microsoft.com](https://visualstudio.microsoft.com/) |
| Windows Driver Kit (WDK) | 10.0.26100.6584 | `winget install --id Microsoft.WindowsWDK.10.0.26100 --silent --accept-package-agreements` |
| .NET SDK | 10.0.302 | Included with Visual Studio, or [dotnet.microsoft.com](https://dotnet.microsoft.com/) |
| WiX Toolset v7 | 7.0.0 | `dotnet tool install -g wix` (for building the .msi installer only) |

The MSVC toolchain (v14.51) and MSBuild (v18.8) are included with Visual Studio.

Verify your installation:

```
msbuild -version
dotnet --version
dir "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\km\ntddk.h"
```

## Build Overview

The workspace contains four groups of projects:

| Group | Toolset | Output |
|---|---|---|
| Kernel driver | `WindowsKernelModeDriver10.0` | `com0com.sys`, `.inf`, `.cat` |
| User-mode C/C++ | `v143` | `setup.dll`, `setupc.exe`, `com2tcp.exe`, `hub4com.exe`, plugins |
| C# GUI | .NET 10.0 | `setupg.exe` |
| C++ tests (Catch2) | `v143` | `setup_tests.exe`, `com2tcp_tests.exe`, `hub4com_tests.exe` |
| C# tests (xUnit) | .NET 10.0 | `setupg.Tests.dll` |

All build output goes to `build\` directories under each project.

## Quick Build

From the workspace root:

```
# Kernel driver
msbuild com0com\sys\com0com.vcxproj /p:Configuration=Release /p:Platform=x64

# All user-mode C/C++ projects
msbuild com0com\com0com.slnx /p:Configuration=Release /p:Platform=x64

# hub4com and plugins
msbuild hub4com\hub4com.slnx /p:Configuration=Release /p:Platform=x64

# C# setup GUI
dotnet build com0com\setupg\setupg.csproj --configuration Release

# All tests
msbuild com0com\com0com.slnx /p:Configuration=Debug /p:Platform=x64 /t:setup_tests
dotnet build com0com\setupg.Tests\setupg.Tests.csproj
```

## Driver Build Details

The driver project is at `com0com\sys\com0com.vcxproj`. It targets the WDM driver model
for Windows Desktop. Three workarounds are applied in the project file because the WDK
shipped before Visual Studio 2026:

1. `VisualStudioVersion` is overridden to `17.0`. The WDK build task DLLs are version 17.0
   but VS 2026 sets this property to 18.0, causing a load failure.

2. `TreatWarningsAsErrors` is disabled and warning 4996 is suppressed. The WDK deprecates
   `ExAllocatePoolWithTag` in favor of `ExAllocatePool2`. Migrating to the new API would
   require changes to every call site.

3. `SkipPackageVerification` is set to true. The WDK's `InfVerif.dll` (x86 variant) is
   not included in all WDK installations.

4. The Release build injects `/d1nodatetime` which disables the `__DATE__` and `__TIME__`
   macros. The code in `trace.c` was updated to not use these macros.

Output is at `com0com\sys\x64\Release\com0com.sys`.

### Driver Defects Fixed

This build includes two fixes over the upstream v3.0.0.0 source:

- **DEFECT-001:** The driver defaulted to 7 data bits, even parity, 1200 baud.
  Modern serial applications expect 8-N-1 at 9600. Fixed in `adddev.c`.

- **DEFECT-002:** Rapid open/close cycles on a port pair caused `STATUS_ACCESS_DENIED`
  because the close path decremented the open count asynchronously. Fixed in
  `openclos.c` by moving the decrement to the synchronous close handler.

## Driver Signing

64-bit Windows requires all kernel drivers to be signed. For development and testing,
use a self-signed certificate with test signing mode.

### Create a test certificate (once)

Run `scripts\create_certs.ps1` as Administrator. This script creates:

- A root CA certificate (`com0com Test Root CA`) installed to `LocalMachine\Root`
- A kernel signing certificate (`com0com Kernel Signing`) with EKU
  `1.3.6.1.4.1.311.61.4.1` (Kernel Mode Code Signing), installed to
  `LocalMachine\TrustedPublisher`

The root CA thumbprint is `1F28A664FA08D8B1C5256AB8EB2D7022852C9F70`.
The signing cert thumbprint is `808BDD474790DA8AC920E3D812E5E1FC4EC4E4E6`.

If you recreate the certificates, update these thumbprints in any scripts that
reference them.

### Sign the driver (every build)

```
signtool sign /fd SHA256 /sha1 808BDD474790DA8AC920E3D812E5E1FC4EC4E4E6 com0com\sys\x64\Release\com0com.sys
signtool sign /fd SHA256 /sha1 808BDD474790DA8AC920E3D812E5E1FC4EC4E4E6 com0com\sys\x64\Release\com0com\com0com.cat
```

`signtool.exe` is at `C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe`.

### Enable test signing (once, requires reboot)

```
bcdedit /set testsigning on
```

Reboot. Verify with `bcdedit /enum | findstr testsigning` — must show `Yes`.

## Installing and Testing the Driver

After building and signing, install the driver and create a test port pair:

```
devcon install com0com.inf root\com0com
setupc --silent install 0 PortName=CNCA0 PortName=CNCB0
```

`devcon.exe` is at `C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe`.
`setupc.exe` is at `com0com\setupc\build\x64\Release\setupc.exe`.

Run from the `com0com\` directory (where `com0com.inf` lives).

Verify the driver is running:

```
sc query com0com
```

Must show `STATE: 4 RUNNING, WIN32_EXIT_CODE: 0`.

Run the null-modem data integrity test:

```
com0com\build\tests\Debug\setup_tests.exe "[driver]"
```

All assertions must pass, including binary data round-trip and rapid open/close cycles.

## Running the Test Suite

The full test suite covers 263 tests across four executables:

```
.\scripts\run_tests.ps1           # Build and run all tests
.\scripts\run_tests.ps1 -SkipBuild # Run without rebuilding
```

Individual test executables:

```
com0com\build\tests\Debug\setup_tests.exe      # 81 tests (setup.dll: params, comdb, mocks, integration, null-modem)
com0com\build\tests\Debug\com2tcp_tests.exe    # 60 tests (telnet protocol, com params, integration)
com0com\build\tests\Debug\hub4com_tests.exe    # 64 tests (hubmsg, routes, filters, plugins, integration)
dotnet test com0com\setupg.Tests\              # 58 tests (C# setup GUI logic)
```

The driver tests (`[driver]` tag in Catch2) require Administrator privileges, test signing
enabled, and a CNCA0↔CNCB0 port pair created.

## Test Signing Certificate (Quick Reference)

The script `scripts\create_certs.ps1` creates certificates with these properties:

- Root CA: `CN=com0com Test Root CA`, Basic Constraints CA=TRUE, pathLen=2
- Signing cert: `CN=com0com Kernel Signing`, EKU 1.3.6.1.5.5.7.3.3 + 1.3.6.1.4.1.311.61.4.1

Both are installed to the LocalMachine store (Root and TrustedPublisher respectively).
The script requires Administrator privileges.

A simple self-signed certificate created with `New-SelfSignedCertificate -Type CodeSigningCert`
will NOT work for kernel drivers. The kernel requires the ELAM/Kernel Mode EKU
(`1.3.6.1.4.1.311.61.4.1`) even in test signing mode on Windows 11.

## Building the MSI Installer

The WiX project is at `setup\wix\com0com.wixproj`. It produces `com0com.msi` — a
standalone Windows Installer package with a custom wizard UI and driver setup automation.

### Prerequisites

```
dotnet tool install -g wix   # one-time, or dotnet tool update -g wix
```

Verify:

```
dotnet wix --version
```

### Build

```
dotnet build setup\wix\com0com.wixproj
```

Output: `setup\wix\build\com0com.msi` (~492 KB).

The build is self-contained — no NuGet package restore is needed beyond the
`WixToolset.Sdk/7.0.0` SDK reference. The MSI embeds all files, the custom UI,
and the deferred custom actions that invoke `setupc.exe`.

### What the Installer Does

1. Copies driver files (`com0com.sys`, `.inf`, `.cat`), setup tools (`setupc.exe`,
   `setupg.exe`, `setup.dll`), and documentation to the install directory.
2. Writes uninstall registry entries.
3. Runs `setupc.exe --silent preinstall` (deferred, after InstallFiles).
4. Runs `setupc.exe --silent update` (deferred).
5. Runs `setupc.exe --silent infclean` (deferred).
6. Optionally runs `setupc.exe --silent install 0 PortName=CNCA0 PortName=CNCB0`
   if the PortPair feature is selected (Level 2, off by default).

### Running the Installer

Interactive (wizard with custom UI):

```
msiexec /i setup\wix\build\com0com.msi
```

Quiet (no UI, default directory):

```
msiexec /i setup\wix\build\com0com.msi /qn
```

Quiet with custom directory:

```
msiexec /i setup\wix\build\com0com.msi /qn INSTALLDIR=C:\MyPath\com0com
```

Quiet with port pair:

```
msiexec /i setup\wix\build\com0com.msi /qn ADDLOCAL=Core,PortPair
```

Enable verbose logging:

```
msiexec /i setup\wix\build\com0com.msi /l*v install.log
```

### Uninstalling

```
msiexec /x setup\wix\build\com0com.msi
```

Or from Add/Remove Programs: "Null-modem emulator (com0com)".

The uninstall runs `setupc.exe --silent uninstall` (deferred, before RemoveFiles)
then removes all installed files and registry entries.

### Wizard Dialogs

The installer presents a 7-page wizard sequence:

1. **Welcome** — What's new in this build (from `whatsnew.rtf`)
2. **License Agreement** — GPL-2.0 (from `license.rtf`), accept checkbox gates Next
3. **Destination Folder** — PathEdit with Browse button
4. **Custom Setup** — Feature tree (Core, PortPair)
5. **Ready to Install** — Destination + features summary, test signing warning
6. **Progress** — Action text + progress bar during installation
7. **Installation Complete** — Success message with test signing reminder

### Directory Layout

```
com0com/
├── com0com/              # Kernel driver + setup tools
│   ├── sys/              # WDM bus driver source
│   ├── setup/            # setup.dll (COM DB, INF parsing)
│   ├── setupc/           # setupc.exe (console setup tool)
│   ├── setupg/           # setupg.exe (C# GUI, .NET 10)
│   └── tests/            # Catch2 test suite
├── com2tcp/              # COM-to-TCP redirector
│   └── tests/            # Catch2 test suite
├── hub4com/              # COM port hub/router
│   ├── plugins/          # 16 filter/driver plugins
│   └── tests/            # Catch2 test suite
├── scripts/              # Build and test automation
├── scratchpad/           # Research, plans, and temporary files
└── docs/                 # Architecture and design documents
```
