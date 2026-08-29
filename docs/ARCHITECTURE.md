# Architecture: com0com Virtual Serial Port Suite

Last updated: 2026-08-29

## Component Overview

```
+------------------+     +------------------+     +------------------+
|   Application A  |     |   Application B  |     |   Application C  |
| (e.g. HyperTerm) |     | (e.g. fax app)   |     | (e.g. GPS reader)|
+--------+---------+     +--------+---------+     +--------+---------+
         |                        |                        |
         v                        v                        v
     \\.\CNCA0              \\.\CNCB0                 \\.\COM4
         |                        |                        |
+--------+------------------------+------------------------+--------+
|                        Windows Serial API (IOCTLs)                 |
+--------+------------------------+------------------------+--------+
         |                        |                        |
         v                        v                        v
+--------+---------+     +--------+---------+     +--------+---------+
|   com0com.sys    |     |   com0com.sys    |     |   com0com.sys    |
|  (bus driver)    |<--->|  (bus driver)    |     |  (bus driver)    |
|  Port CNCA0 FDO  |     |  Port CNCB0 FDO  |     |  Port COM4 FDO   |
+------------------+     +------------------+     +------------------+
         |                        |                        |
         +-------- paired --------+                        |
         |                                                 |
    null-modem link:                                Paired with
    output to A = input from B                    another port...
    output to B = input from A
```

The com0com suite consists of five main components:

| Component                      | Type           | Purpose                                           |
| ------------------------------ | -------------- | ------------------------------------------------- |
| `com0com.sys`                | Kernel driver  | Virtual null-modem COM port pairs                 |
| `setupc.exe` / `setup.dll` | User-mode tool | COM port database management, driver installation |
| `setupg.exe`                 | GUI tool       | Graphical port pair configuration (.NET 10)       |
| `com2tcp.exe`                | User-mode tool | TCP-to-COM redirector with Telnet RFC 2217        |
| `hub4com.exe`                | User-mode tool | COM port hub/router with plugin filter chain      |

---

## com0com.sys: Kernel-Mode WDM Bus Driver

### Device Model

com0com.sys is a WDM bus driver that creates virtual COM port pairs. Each pair
consists of two Function Device Objects (FDOs) connected via a shared I/O buffer.

- **Bus device:** A single root-enumerated device. The INF registers the
  service for `root\com0com`, and PnP assigns the instance number
  (for example `0000`) at runtime.
- **Port FDOs:** Child devices created by the bus driver. Each port appears as a
  standard serial port to Windows, identified by hardware IDs:

  - `com0com\cncport`: CNC port class. The driver builds the default port names
    CNCA0/CNCB0 from `C0C_PREF_PORT_NAME_A` and `C0C_PREF_PORT_NAME_B`
    (`include/com0com.h`). The INF does not contain them.
  - `com0com\comport`: COM port class. Ports registered under this class get
    COM numbers assigned through the COM port database.

### Port Pairing

Two port FDOs are linked together as a pair. When data is written to one port,
the driver places it into the paired port's read buffer. Modem control signals
DTR, RTS, CTS, DSR, DCD, and RI are mirrored between paired ports. This creates
a virtual null-modem cable.

### Key Data Structures

- `C0C_COMMON_EXTENSION` (`sys/com0com.h`): base device extension for all
  device types
- `C0C_FDOPORT_EXTENSION`: port FDO extension, contains the local I/O port
  state (`pIoPortLocal`) and open count
- `C0C_IO_PORT`: I/O port state: read/write buffers, line control (baud rate,
  byte size, parity, stop bits), modem control, handshake configuration,
  pinout mapping

### IRP Handling

The driver handles standard serial IRPs:

| IRP                       | Handler                          | Purpose                                       |
| ------------------------- | -------------------------------- | --------------------------------------------- |
| `IRP_MJ_CREATE`         | `c0cOpen` → `FdoPortOpen`   | Open port, allocate read buffer               |
| `IRP_MJ_CLOSE`          | `c0cClose` → `FdoPortClose` | Close port, free read buffer                  |
| `IRP_MJ_READ`           | `read.c`                       | Read from paired port's write buffer          |
| `IRP_MJ_WRITE`          | `write.c`                      | Write to paired port's read buffer            |
| `IRP_MJ_DEVICE_CONTROL` | `ioctl.c`                      | Serial IOCTLs (baud rate, line control, etc.) |
| `IRP_MJ_PNP`            | `pnp.c`                        | PNP start/stop/remove                         |
| `IRP_MJ_POWER`          | `power.c`                      | System and device power transitions           |

### Default Port Configuration

The driver defaults are:

- 8 data bits, no parity, 1 stop bit, 9600 baud
- Handshake: `SERIAL_DTR_CONTROL` for DTR flow control and
  `SERIAL_RTS_CONTROL` for RTS assertion
- XON/XOFF characters: 0x11 / 0x13

### Pinout Customization

The driver supports custom pinout wiring between paired ports via the
`PinMap` function. Each of the four input pins CTS, DSR, DCD, and RI can be
wired to a local or remote signal source with optional inversion.

### Modem Control

Modem control signals are mirrored between paired ports on open:

- Writing DTR/RTS on port A asserts DSR/CTS on port B
- On open, the driver sets the pseudo bit `C0C_MCR_OPEN` (0x80) in the modem
  control register to signal the open state to the paired port

---

## setup.dll / setupc.exe: COM Port Configuration

`setup.dll` is a library that manages the COM port database, INF files, and
driver installation. `setupc.exe` is a console frontend.

### COM Port Database (`comdb.cpp`)

Windows maintains a database of assigned COM port numbers in the registry.
`setup.dll` provides functions to:

- `ComDbGetInUse()`, `ComDbClaim()`, `ComDbRelease()`, `ComDbQueryNames()`
  (declared in `comdb.h`): read the COM DB and reserve or release COM port
  numbers
- Query which COM port numbers are in use (`busynames`)

### INF File Parsing (`inffile.cpp`)

`inffile.cpp` implements a generic INF file parser. `setup.cpp` feeds it the
three driver INF files `com0com.inf`, `cncport.inf`, and `comport.inf` to
extract driver installation parameters.

### Driver Installation

`setupc.exe install` calls the Windows PNP manager to create port pairs:

1. Creates the root-enumerated device node via `SetupDiCreateDeviceInfo` and
   `SetupDiCallClassInstaller(DIF_REGISTERDEVICE)`, then updates the driver
   with `UpdateDriverForPlugAndPlayDevices`
2. Registers port names in the COM port database
3. The PNP manager loads `com0com.sys` and creates port FDOs

---

## com2tcp: TCP-to-COM Redirector

com2tcp bridges TCP network connections to COM ports. It operates in two modes.

### Raw TCP Mode

Direct byte stream between a TCP socket and a COM port. Uses `CreateFile("\\.\COMx")`
to open the serial port and standard Winsock for the TCP connection.

### Telnet RFC 2217 Mode

Implements the Telnet COM Port Control Protocol (RFC 2217). This allows remote
configuration of serial port parameters (baud rate, data bits, parity, stop bits,
flow control) over the telnet control channel.

The `TelnetProtocol` class (`telnet.h`, implemented in `telnet.cpp`) implements:

- IAC (Interpret As Command) state machine: `stData → stCode → stOption → stSubParams → stSubCode`
- IAC byte escaping: 0xFF bytes in the data stream are doubled
- Option negotiation: WILL/WONT/DO/DONT for ECHO, Terminal Type, COM Port Control
- Sub-negotiation for RFC 2217 COM port parameters

### Command Line

```
com2tcp --baud 9600 --data 8 --parity none --stop 1 \.\CNCA0 192.168.1.1 5000
```

This connects `\\.\CNCA0` to a TCP server at `192.168.1.1` on port `5000`.
Option values are space-separated, not `=`-joined.

---

## hub4com: COM Port Hub/Router

hub4com routes data and control signals between multiple COM ports through a
configurable pipeline of filter plugins. It acts as a software patch panel
for serial ports.

### Architecture

```
+--------+     +------------------------------------------+     +--------+
| COM1   | --> | Filters: [echo] → [crypt] → [linectl]    | --> | COM2   |
+--------+     |                                          |     +--------+
               | Route table: COM1 → COM2, COM2 → COM1     |
+--------+     |                                          |     +--------+
| COM3   | <-- | Drivers:  [serial]  [tcp]  [connector]   | <-- | COM4   |
+--------+     +------------------------------------------+     +--------+
```

### Port Types

- **Serial ports**: physical or virtual COM ports on the local machine
- **TCP ports**: TCP listeners/clients for remote serial communication
- **Connector ports**: connect two hub4com instances together

### Routing (`route.cpp`)

Routes define which source port sends data to which destination port. A port
can send to multiple destinations and receive from multiple sources. Routes
control both data flow and flow control signal propagation.

Key route options:

- `--route=FROM:TO`: basic data route
- `--echo-route=<Lst>`: echo route. Takes a single port list; messages routed
  through it are sent back to the sender.
- `--fc-route=FROM:TO`: flow control route only

### Filter Pipeline

Each port can have a chain of filters applied to incoming and outgoing messages.
Filters are loaded from DLLs and connected in order:

1. `pInMethod`: called for data arriving at the port
2. `pOutMethod`: called for data leaving the port

Messages can be modified, replaced, consumed, or passed through by each filter.

### Available Filter Plugins

| Filter        | Purpose                                                       |
| ------------- | ------------------------------------------------------------- |
| `echo`      | Echo data back to sender (alternative to`--echo-route`)     |
| `crypt`     | Encrypt/decrypt data with a shared secret                     |
| `linectl`   | Modify line control (baud rate, byte size, parity, stop bits) |
| `pinmap`    | Remap modem control signals (e.g. DTR→CTS)                   |
| `pin2con`   | Map pin changes to connect/disconnect actions                 |
| `escparse`  | Parse escape sequences from the data stream                   |
| `escinsert` | Insert escape sequences into the data stream                  |
| `awakseq`   | Detect and process wake-up sequences                          |
| `tag`       | Add/remove tags to message frames                             |
| `lsrmap`    | Map line status register (LSR) bits                           |
| `purge`     | Purge buffers on specific events                              |
| `telnet`    | Telnet RFC 2217 protocol handling within hub4com              |
| `trace`     | Log all messages passing through for debugging                |

### Available Driver Plugins

| Driver        | Purpose                                    |
| ------------- | ------------------------------------------ |
| `serial`    | Connect to a physical/virtual COM port     |
| `tcp`       | TCP listener/client for remote connections |
| `connector` | Interconnect two hub4com instances         |

---

## Plugin API

Plugins are DLLs that export a single function:

```c
const PLUGIN_ROUTINES_A *const *InitA(const HUB_ROUTINES_A *pHubRoutines);
```

### Common Routines (`PLUGIN_ROUTINES_A`)

Every plugin provides:

- `pGetPluginType()`: returns `PLUGIN_TYPE_FILTER` or `PLUGIN_TYPE_DRIVER`
- `pGetPluginAbout()`: name, copyright, license, description
- `pHelp(progPath)`: print usage to stderr
- `pConfigStart()` / `pConfig(pArg)` / `pConfigStop()`: parse configuration
  options

Most filter plugins leave the config callbacks NULL and read their options
from `argc`/`argv` when the filter instance is created. Only the `serial`,
`tcp`, and `trace` plugins implement the config API.

### Filter-Specific Routines (`FILTER_ROUTINES_A`)

- `pCreate`: `HFILTER (CALLBACK *)(HMASTERFILTER hMasterFilter, HCONFIG hConfig,
  int argc, const char *const argv[])`, creates a filter instance
- `pDelete(hFilter)`: destroy a filter instance
- `pCreateInstance(hMasterFilterInstance)`: create a per-port filter instance
- `pDeleteInstance(hFilterInstance)`: destroy a per-port filter instance
- `pInMethod(hFilter, hFilterInstance, pInMsg, ppEchoMsg)`: process an incoming
  message
- `pOutMethod(hFilter, hFilterInstance, hFromPort, pOutMsg)`: process an
  outgoing message

### Hub Routines (`HUB_ROUTINES_A`)

The hub provides 19 callback functions to plugins for buffer allocation,
message creation/manipulation, port naming, timers, and argument parsing.

### Message Types

Messages flowing through the hub are typed `HUB_MSG` structs with a union type.
The type constants are defined in `plugins/plugins_api.h`. `hubmsg.cpp` wraps
`HUB_MSG` in the `HubMsg` class:

| Type                           | Content                   |
| ------------------------------ | ------------------------- |
| `HUB_MSG_TYPE_EMPTY`         | No data                   |
| `HUB_MSG_TYPE_LINE_DATA`     | Serial data buffer        |
| `HUB_MSG_TYPE_CONNECT`       | Boolean connection state  |
| `HUB_MSG_TYPE_MODEM_STATUS`  | Modem control signal bits |
| `HUB_MSG_TYPE_LINE_STATUS`   | Line status register bits |
| `HUB_MSG_TYPE_SET_PIN_STATE` | Set output pin state      |

---

## Registry Layout

The com0com driver stores its configuration in the registry. The MSI installer
adds its own keys.

### Driver Parameters

`HKLM\SYSTEM\CurrentControlSet\Services\com0com\Parameters`

Port-specific parameters are stored per port under
`HKLM\SYSTEM\CurrentControlSet\Services\com0com\Parameters\<port>` (for
example `...\Parameters\CNCA0`). The driver also writes the `PortName` value to
the port's PnP device key under
`HKLM\SYSTEM\CurrentControlSet\Enum\root\com0com\...\Device Parameters` via
`IoOpenDeviceRegistryKey`.

### COM Port Database

`HKLM\SYSTEM\CurrentControlSet\Control\COM Name Arbiter`

Windows maintains the COM port number assignment here. `setup.dll` reads and
writes this database to reserve/release COM port numbers.

### Setup Parameters

`HKLM\Software\com0com`

The MSI installer writes an `Install_Dir` value here with the installation
directory path.

### Uninstall Information

`HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\com0com`

The MSI installer registers the standard uninstall keys here: DisplayName,
Publisher, Version, UninstallString, and others.

---

## Data Flow: Null-Modem Write

```
Application A                 Driver                  Application B
    |                           |                           |
    WriteFile(\\.\CNCA0)        |                           |
    |-------------------------->|                           |
    |  IRP_MJ_WRITE             |                           |
    |                    write.c:                           |
    |                    Copy data to                       |
    |                    CNCB0's read buffer                |
    |                           |                           |
    |                           |  Complete write IRP       |
    |<--------------------------|                           |
    |                           |                           |
    |                           |  Signal wait event        |
    |                           |-------------------------->|
    |                           |                  ReadFile(\\.\CNCB0)
    |                           |                  returns data
```

The paired port's read buffer acts as the null-modem link. Under the port
spinlock, a write to port A populates port B's read buffer directly. If the
write exceeds the read buffer's capacity, the overflow is staged in a TX
buffer, which also drives baud-rate emulation timing.

## Data Flow: hub4com with Filters

```
COM1 → [serial driver] → [echo filter] → [crypt filter] → route → [tcp driver] → network
                                                                   ↓
                                                              [trace filter]
                                                                   ↓
                                                              trace.log
```

Messages flow through driver plugins (which read/write to actual ports), then
through the filter chain, then through the route table, and finally to the
destination driver plugins.
