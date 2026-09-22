# Apple Broadcom Bluetooth Driver Reverse Engineering & Linux Porting Checklist

- [x] **Phase 1: Environment & Repository Setup**
  - [x] Consolidate driver files into `Driver/`
  - [x] Set up local Python environment with PyGhidra and `jpype1`
  - [x] Create session logging utility (`log_action.py` -> `logs/session.log`)
  - [x] Establish initial documentation (`DRIVER_ANALYSIS.md` & `AGENTS.md`)

- [x] **Phase 2: Binary Reconnaissance & Static Analysis (`radare2`)**
  - [x] Extract import/export symbols and sections from `Driver/AppleBTBC.sys`
  - [x] Search for Vendor-Specific HCI commands and Broadcom firmware patch signatures
  - [x] Inspect INF-based hardware IDs and registry parameters (`BtParameters`)

- [x] **Phase 3: Deep Decompilation & Control Flow Analysis (`Ghidra`)**
  - [x] Analyze `DriverEntry` and KMDF driver initialization (`WdfDriverCreate`, `WdfDeviceCreate`)
  - [x] Decompile IOCTL and URB dispatch routines to trace USB communication protocol
  - [x] Identify firmware download / patch RAM initialization routines and constants

- [x] **Phase 4: Linux Kernel Mapping & BlueZ Integration**
  - [x] Compare Apple Broadcom PIDs with Linux kernel `btusb.c` and `hci_bcm.c` tables
  - [x] Analyze firmware loading requirements (e.g., `.hcd` firmware files extraction for Linux)
  - [x] Formulate kernel module configuration / patch requirements for Linux support

- [x] **Phase 5: Verification & Documentation**
  - [x] Document all discovered HCI vendor commands and initialization sequences
  - [x] Finalize Linux porting guide and testing instructions

- [x] **Phase 6: Practical Implementation & Configuration**
  - [x] Create Linux Porting & Configuration Guide (`LINUX_PORT_GUIDE.md`)
  - [x] Formulate udev rules for disabling USB autosuspend (matching Windows driver behavior)

- [x] **Phase 7: Modern Linux 7+ Kernel Driver Architecture Design**
  - [x] Design custom USB/HCI driver architecture (`applebt_usb`) in `LINUX_DRIVER_ARCHITECTURE.md`
  - [x] Define USB endpoint management, URB handling, and Broadcom PatchRAM firmware upload sequences for modern kernels (Linux 6.x/7+)

- [x] **Phase 8: Driver Implementation & Build Verification**
  - [x] Implement modern Linux kernel C driver (`driver_src/brcmbt_usb.c`) supporting Apple VID/PIDs and generic Broadcom IDs
  - [x] Create kernel module `Makefile` and verify clean compilation (`brcmbt_usb.ko`) against Linux kernel headers

- [x] **Phase 9: DKMS Packaging & Deployment**
  - [x] Create `dkms.conf` configuration for automatic module rebuilding across kernel updates
  - [x] Create automated installation script (`driver_src/install.sh`) for system-wide deployment

- [x] **Phase 10: System Installation & Module Verification**
  - [x] Execute system installation via DKMS (`install.sh`)
  - [x] Load kernel module (`modprobe brcmbt_usb`) and verify successful USB interface driver registration (`dmesg` / `lsmod`)
