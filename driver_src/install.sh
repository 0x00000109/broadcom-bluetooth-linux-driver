#!/bin/bash
# Installation script for brcmbt-usb DKMS driver

DRV_NAME="brcmbt-usb"
DRV_VERSION="1.1.0"
SRC_DIR="/usr/src/${DRV_NAME}-${DRV_VERSION}"

echo "Installing ${DRV_NAME} version ${DRV_VERSION} to DKMS..."

sudo mkdir -p "${SRC_DIR}"
sudo cp brcmbt_usb.c Makefile dkms.conf "${SRC_DIR}/"

sudo dkms add -m "${DRV_NAME}" -v "${DRV_VERSION}"
sudo dkms build -m "${DRV_NAME}" -v "${DRV_VERSION}"
sudo dkms install -m "${DRV_NAME}" -v "${DRV_VERSION}"

echo "Installation complete. Load module with: sudo modprobe brcmbt_usb"
