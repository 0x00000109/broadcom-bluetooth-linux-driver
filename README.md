# Broadcom & Apple Bluetooth USB Linux Driver (Modern Kernels 6.x / 7+)

An open-source universal Linux kernel driver and reverse-engineering repository for Broadcom Bluetooth USB controllers (including OEM integrations in Apple MacBooks, iMacs, and generic Broadcom / ASUS / Dell / HP M.2 / PCIe combo modules like BCM4352 / BCM4360).

## Repository Structure

- `driver_src/`: Modern Linux C driver (`brcmbt_usb.c`), `Makefile`, DKMS configuration (`dkms.conf`), and installation automation (`install.sh`).
- `Driver/`: Consolidated Windows driver package (`AppleBTBC.inf`, `AppleBTBC.sys`, `.cat`, etc.) used for reverse engineering.
- `DRIVER_ANALYSIS.md`: Detailed static analysis and WDF filter architecture findings from `AppleBTBC.sys`.
- `LINUX_DRIVER_ARCHITECTURE.md`: Technical specification of the modern Linux 7+ driver architecture.
- `LINUX_PORT_GUIDE.md`: Guide on `btusb` integration, firmware patchram loading, and `udev` power management rules.
- `CHECKLIST.md`: Full project development and verification checklist.

## Quick Start & Installation (DKMS)

1. Clone or navigate to the repository directory.
2. Run the automated DKMS installation script:
   ```bash
   cd driver_src
   sudo ./install.sh
   ```
3. Load the kernel module:
   ```bash
   sudo modprobe brcmbt_usb
   ```

## License
This project is licensed under the terms of the **GNU General Public License v2.0 (GPLv2)**. See the `LICENSE` file for details.
