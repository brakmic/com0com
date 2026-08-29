# com0com — Virtual Serial Port Emulator for Windows

The com0com project provides tools for creating and managing virtual serial
(COM) port pairs on Windows. Output to one port appears as input on the other —
a null-modem connection in software.

Originally developed by Vyacheslav Frolov (2004–2012) and published on
[SourceForge](http://com0com.sourceforge.net/), this repository preserves the
source code and modernizes the build system for current toolchains.

## What is in this Repository

| Component | Description |
|---|---|
| `com0com.sys` | Kernel-mode WDM bus driver for virtual COM port pairs |
| `setupc.exe` / `setup.dll` | Console tool for COM port database and driver management |
| `setupg.exe` | Graphical setup utility (.NET 10 WinForms) |
| `com2tcp.exe` | COM port to TCP redirector (raw TCP + Telnet RFC 2217) |
| `hub4com.exe` | COM port hub/router with 16 filter/driver plugins |

## Documentation

- **[BUILDING.md](BUILDING.md)** — prerequisites, build commands, driver signing,
  test invocation
- **[ARCHITECTURE.md](ARCHITECTURE.md)** — system design, data flow, plugin API
  reference, registry layout
- **[TESTING.md](TESTING.md)** — test suite structure, tiers, driver test setup,
  mock framework

The original 2004–2012 build system files (DDK `dirs`/`sources`, VC++ 2005
`.vcproj`, NSIS installer) are archived in [`archive/`](archive/README.md).

## Quick Start

```
# Prerequisites: Visual Studio 2026, WDK 10.0.26100, .NET 10

# Build everything
msbuild com0com\sys\com0com.vcxproj /p:Configuration=Release /p:Platform=x64
msbuild com0com\com0com.slnx /p:Configuration=Release /p:Platform=x64

# Run tests
.\scripts\run_tests.ps1
```

## License

This project is distributed under the GNU General Public License, version 2 or
any later version (`GPL-2.0-or-later`).

See [LICENSE](LICENSE) and the copyright notices in individual source files.
