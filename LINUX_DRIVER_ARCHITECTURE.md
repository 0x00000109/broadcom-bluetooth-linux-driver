# Linux 7+ Kernel Driver Architecture for Broadcom Bluetooth Controllers

## 1. Overview & Objectives
While mainline Linux (`btusb`) handles many Broadcom Bluetooth controllers (used across Apple, Asus, Dell, HP, and generic Wi-Fi/BT combo cards like BCM4352/BCM4360) via standard Broadcom helpers (`btbcm`), specialized hardware or custom firmware patch loading requirements on modern Linux kernels (Linux 6.x / 7+) benefit from a dedicated out-of-tree USB/HCI driver (`brcmbt_usb`).

This document outlines the software architecture, kernel APIs, and control flow for a modern Linux kernel driver designed for Broadcom Bluetooth USB devices (supporting both generic Broadcom VID `0x0a5c` and OEM integrations such as Apple VID `0x05ac`).

---

## 2. Architectural Layers

```
+-------------------------------------------------------+
|                BlueZ User-space / Socket              |
+-------------------------------------------------------+
                          |
                          v
+-------------------------------------------------------+
|               Linux Bluetooth Core (HCI)              |
+-------------------------------------------------------+
                          |
                          v
+-------------------------------------------------------+
|       Custom Driver: brcmbt_usb (USB Core Driver)     |
|  - USB Endpoint Management (Bulk IN/OUT, Interrupt)   |
|  - HCI Packet Framing & URB Submission                |
+-------------------------------------------------------+
                          |
                          v
+-------------------------------------------------------+
|       Broadcom PatchRAM / Firmware Loader Engine       |
|  - Asymmetric Firmware Request (`request_firmware`)   |
|  - Vendor HCI Commands (BCM specific OGF 0x3F / 0xFC) |
+-------------------------------------------------------+
                          |
                          v
+-------------------------------------------------------+
|           Hardware (Broadcom / OEM Modules)           |
+-------------------------------------------------------+
```

---

## 3. Core Components & Data Structures

### 3.1. USB Device Registration (`struct usb_driver`)
The driver registers with the USB subsystem targeting Broadcom Vendor ID (`0x0a5c`) and OEM integrations (`0x05ac`):
```c
static const struct usb_device_id brcmbt_table[] = {
    { USB_DEVICE(0x05ac, 0x8213) },
    { USB_DEVICE(0x0a5c, 0x21e8) },
    { }
};
MODULE_DEVICE_TABLE(usb, brcmbt_table);
```

### 3.2. Driver Context (`struct brcmbt_data`)
Encapsulates device state, USB interface references, HCI device pointer, and URB queues:
```c
struct brcmbt_data {
    struct usb_device *udev;
    struct usb_interface *intf;
    struct hci_dev *hdev;
    
    // USB Endpoints
    __u8 bulk_in_ep;
    __u8 bulk_out_ep;
    __u8 int_in_ep;
    
    // URBs for Asynchronous I/O
    struct urb *bulk_in_urb;
    struct urb *int_in_urb;
    
    // Firmware state
    char fw_name[64];
    bool firmware_loaded;
};
```

---

## 4. Key Subsystems & Workflows

### 4.1. Probe & Initialization (`probe` callback)
1. Allocate driver context (`kzalloc`).
2. Identify and assign USB bulk IN/OUT and interrupt endpoints from `usb_host_interface`.
3. Allocate and initialize HCI device (`hci_alloc_dev`).
4. Set up HCI transport callbacks (`open`, `close`, `flush`, `send_frame`).
5. Register HCI device (`hci_register_dev`).

### 4.2. Firmware PatchRAM Upload (Broadcom Protocol)
Unlike standard USB devices, Broadcom chips require a firmware patch upload sequence before normal HCI operation:
1. Trigger asynchronous firmware request: `request_firmware_nowait(...)`.
2. Upon firmware delivery (`fw_cb`):
   - Send Vendor HCI Reset command.
   - Send baud rate configuration commands (if applicable).
   - Stream firmware patch chunks via Vendor-Specific HCI Download commands (`0xFC4E` or similar based on BCM protocol).
   - Send final launch command to start execution of downloaded firmware patch.

### 4.3. URB Handling & Data Transfer
- **ACL / SCO / Event Reception**: Submitted via continuous asynchronous URBs (`usb_fill_bulk_urb` and `usb_fill_int_urb`).
- **HCI Transmission (`send_frame`)**: Translates BlueZ SKBs into USB URBs submitted to the bulk out endpoint.

---

## 5. Modern Linux 7+ Kernel API Considerations
- **Memory Management**: Use `GFP_KERNEL` for sleepable allocations and `GFP_ATOMIC` inside interrupt/URB completion contexts.
- **Locking**: Replace legacy global locks with per-device `mutex` and spinlocks (`spinlock_t`).
- **Power Management**: Implement runtime PM hooks (`suspend`/`resume`) matching Windows selective suspend disabling to ensure zero Bluetooth dropouts.
- **Compilation & Standards**: Strict adherence to kernel coding style, `__init`/`__exit` macros, and module licensing (`MODULE_LICENSE("GPL")`).
