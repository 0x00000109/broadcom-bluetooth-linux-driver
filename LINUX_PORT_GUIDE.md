# Linux Porting & Configuration Guide for Broadcom Bluetooth Controllers (BCM4352 / BCM4360 / BCM20702)

Based on driver analysis and supported USB Product IDs (`VID_05ac` / `VID_0a5c`), this guide details how to configure and run Broadcom Bluetooth modules (used across Apple, Asus, Dell, HP, and generic OEM hardware) under Linux (BlueZ / kernel `btusb`).

## 1. Supported Broadcom Hardware (OEM & Generic VIDs)
- **OEM Integrations (e.g., Apple VID: `0x05ac`)**:
  - `0x8213`, `0x8215`, `0x8218`, `0x821A`, `0x821B`, `0x821D`, `0x821F`
  - `0x8281`, `0x8286`, `0x8287`, `0x8289`, `0x828A`, `0x828B`, `0x828C`, `0x828D`, `0x828E`, `0x828F`
  - `0x8290`, `0x8291`, `0x8293`, `0x8294`
- **Generic Broadcom USB VIDs (e.g., `0x0a5c`)**:
  - Standard USB Bluetooth controllers and HMB/M.2 combo card USB interfaces.

## 2. Kernel Module Configuration (`btusb` & `btbcm`)
Linux kernel includes native support for these devices via `btusb`. Ensure the following modules are loaded:
```bash
sudo modprobe btusb
sudo modprobe btbcm
```

## 3. Power Management & Selective Suspend (Equivalent to Windows Filter Settings)
In Windows, vendor filter drivers disable USB selective suspend (`SelectiveSuspendEnabled = 0`). Under Linux, to prevent stability issues or dropouts on Broadcom Bluetooth controllers, you can disable USB autosuspend via udev rules.

Create `/etc/udev/rules.d/99-broadcom-bluetooth.rules`:
```udev
# Disable USB autosuspend for Broadcom Bluetooth controllers (Apple & Generic VIDs)
SUBSYSTEM=="usb", ATTRS{idVendor}=="05ac|0a5c", TEST=="power/control", ATTR{power/control}="on"
```

## 4. Firmware Patch RAM Loading
Broadcom Bluetooth controllers require a firmware patch (.hcd file) uploaded upon initialization.
- Ensure `linux-firmware` package is installed.
- BlueZ and `btbcm` will automatically request and upload the correct patch file for BCM chipsets across different hardware implementations.
