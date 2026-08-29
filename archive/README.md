# Archive — Obsolete Build System Files

These files were part of the original com0com distribution by Vyacheslav Frolov
(2004-2012). They have been replaced by the modern build system based on Visual
Studio 2026, MSBuild, and .NET 10. See `BUILDING.md` at the workspace root for
current build instructions.

## Contents

### `ddk-build/` — Windows DDK/WDK Build System

The original build used Microsoft's Driver Development Kit build tool (`build.exe`)
with `dirs`, `sources`, and `makefile` files. This system was deprecated circa 2013
in favor of MSBuild and `.vcxproj` files.

| File | Original location | Purpose |
|---|---|---|
| `com0com/dirs` | `com0com\dirs` | Top-level directory traversal for `build.exe` |
| `com0com/sys/makefile` | `com0com\sys\makefile` | Driver build: `build -wcZ -M 1` |
| `com0com/sys/sources` | `com0com\sys\sources` | Source file list for the kernel driver |
| `com0com/setup/makefile` | `com0com\setup\makefile` | setup.dll build |
| `com0com/setup/sources` | `com0com\setup\sources` | Source file list for setup.dll |
| `com0com/setupc/makefile` | `com0com\setupc\makefile` | setupc.exe build |
| `com0com/setupc/sources` | `com0com\setupc\sources` | Source file list for setupc.exe |

Replaced by: `com0com\sys\com0com.vcxproj`, `com0com.slnx`

### `vs2005-projects/` — Visual Studio 2005/2008 Projects

The original build used VC++ 2005 Express Edition with `.vcproj` and `.dsp`/`.dsw`
project formats. These are not compatible with modern MSBuild.

**com2tcp (**`com2tcp/`): `.dsp` (VC++ 6.0), `.dsw` (workspace), `.vcproj` (VS 2008)

**hub4com (**`hub4com/`): `.vcproj` for the main hub binary

**hub4com-plugins (**`hub4com-plugins/`): 16 `.vcproj` files, one per plugin:
`awakseq`, `connector`, `crypt`, `echo`, `escinsert`, `escparse`, `linectl`,
`lsrmap`, `pin2con`, `pinmap`, `purge`, `serial`, `tag`, `tcp`, `telnet`, `trace`

**hub4com-static (**`hub4com-static/`): Static build variant of hub4com

Replaced by: `com2tcp\com2tcp.vcxproj`, `hub4com\hub4com.slnx` (with `.vcxproj` per plugin)

### `cpp-setupg/` — C++/CLI Setup GUI

The original GUI setup tool (`setupg.exe`) was a C++/CLI Windows Forms application
targeting .NET Framework 2.0. It was built with VC++ 2005 Express Edition.

| File | Purpose |
|---|---|
| `setup.vcproj` | VS 2008 project file |
| `app.rc` | Resource file |
| `AssemblyInfo.cpp` | Assembly metadata |
| `stdafx.cpp`, `stdafx.h` | Precompiled header |
| `Form1.h`, `Form1.resx` | Main form (Windows Forms) |
| `resource.h` | Resource identifiers |

Replaced by: `com0com\setupg\setupg.csproj` — C# .NET 10 WinForms

### `nsis-installer/` — NSIS Installer

The original installer used Nullsoft Scriptable Install System (NSIS). The
`install.nsi` script handles multi-CPU driver deployment (i386/amd64/ia64),
optional port pair creation, start menu shortcuts, and calls `setupc.exe` for
driver preinstall/update/infclean/uninstall operations.

Replaced by: WiX Toolset v5 MSI installer (Block F, not yet implemented). The
NSIS script was preserved in full for reference during the WiX implementation.

### `obsolete-docs/` — Old Build Instructions

Two `Building.txt` files describing the DDK and VC++ 2005 build procedure.

Replaced by: `BUILDING.md` at the workspace root.

### Root Files

| File | Original location | Purpose |
|---|---|---|
| `setup.bat` | `com0com\setup\setup.bat` | Batch-based COM port setup (pre-setupc.exe era) |
| `setup.def` | `com0com\setup\setup.def` | Module definition file for setup.dll |

Replaced by: `setupc.exe` (console setup tool, in `com0com\setupc\`)
