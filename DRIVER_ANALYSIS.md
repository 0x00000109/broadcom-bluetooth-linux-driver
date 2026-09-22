# Apple Broadcom Bluetooth Driver Analysis & Porting Documentation

## 1. Hardware Identification (from `Driver/AppleBTBC.inf`)
- **Vendor ID**: `USB\VID_05ac` (Apple Inc.)
- **Supported Product IDs (PIDs)**:
  - `0x8213`, `0x8215`, `0x8218`, `0x821A`, `0x821B`, `0x821D`, `0x821F`
  - `0x8281`, `0x8286`, `0x8287`, `0x8289`, `0x828A`, `0x828B`, `0x828C`, `0x828D`, `0x828E`, `0x828F`
  - `0x8290`, `0x8291`, `0x8293`, `0x8294`, `0x8294&MI_02`

## 2. Driver Architecture & Windows Integration
- **Driver Type**: KMDF 1.11 Lower Filter Driver (`AppleBTBC.sys`) sitting on top of Microsoft's Bluetooth USB stack (`Bth.inf` / `BthUsb`).
- **Registry / Power Settings**:
  - `LowerFilters`: `AppleBtBc`
  - `DeviceSelectiveSuspended`: `0` (Disabled)
  - `SelectiveSuspendEnabled`: `0` (Disabled)
- **Configuration Keys (Boot Camp)**:
  - `SOFTWARE\Apple Inc.\Boot Camp\BtParameters`

## 3. Reverse Engineering Roadmap & Tools
- **Exact Bytes / Instructions**: `radare2` on `Driver/AppleBTBC.sys`
- **Strings & Raw Xrefs**: `radare2` (`izz`, `ax`)
- **Bulk Data & Automation**: `r2pipe + Python`
- **Control Flow & Pseudocode**: `Ghidra` (Decompiler / CFG)
- **Cross-Binary / Semantic Comparison**: Ghidra P-code / BSim & custom metrics

## 4. Findings Log
- Initial state: Driver package successfully centralized in `Driver/`.
- Infrastructure: PyGhidra configured in `.venv`, session logger ready (`logs/session.log`).
- Binary analysis (`AppleBTBC.sys` via `radare2` & PyGhidra):
  - PDB Path: `D:\BWA\AppleBluetoothBroadcomWin-6206\srcroot\AppleBluetoothBroadcom\x64\Win8Release\AppleBTBC.pdb`
  - Internal structures: `SfDEVICE_EXTENSION`, KMDF library integration (75 functions analyzed).
  - File version: `5.1.0.0` (Apple Inc., 2006-2015).
  - Architectural role: `AppleBTBC.sys` is a thin KMDF lower filter driver that disables selective suspend and interfaces with Boot Camp registry parameters (`BtParameters`), relying on the underlying Microsoft USB Bluetooth stack for core protocol handling.

## 5. Linux Kernel Mapping & Porting Strategy
- **Equivalence in Linux**:
  - Since `AppleBTBC.sys` is a Windows WDF filter driver rather than a standalone protocol stack, Linux does not need a direct binary port of `.sys`.
  - In Linux, Apple Broadcom Bluetooth controllers (VID `0x05ac`, PIDs `0x8213`, `0x8281`, etc.) are natively supported by the kernel `btusb` driver combined with the Broadcom Bluetooth helper module (`btbcm`).
- **Firmware / PatchRAM**:
  - Broadcom chips require firmware patches (.hcd files) uploaded during initialization. Linux BlueZ (`btbcm`) handles this automatically via user-space firmware loaders (`firmware-bcm43xx` or linux-firmware package).
- **Power Management / Quirks**:
  - The registry settings in Windows (`SelectiveSuspendEnabled = 0`) correspond to disabling USB autosuspend for these Apple Bluetooth devices in Linux via udev rules or module parameters (`btusb.enable_autosuspend=0` if needed).
