# Project progress

This file tracks implementation progress for the nRF52840 web server.

## v1.0 goal

Build a low-cost embedded web server around the nRF52840 using:

- W5500 Ethernet
- microSD storage
- one shared SPI bus
- FreeRTOS
- FatFs
- UARTE logging

The v1.0 milestone serves static files directly from the SD card over HTTP.

---

## Core drivers

- [x] UARTE driver
- [x] Ring-buffer logger
- [x] Generic SPI master driver
  - [x] FreeRTOS mutex-protected shared bus
  - [x] Per-device configuration
  - [x] Blocking TX/RX with timeout
  - [x] EasyDMA-safe buffer handling
  - [x] Shared W5500 + SD operation

---

## SD card

- [x] SDHC/SDXC initialization in SPI mode
- [x] Read and decode CSD v2 using CMD9
- [x] Determine card block count
- [x] Read 512-byte blocks using CMD17
- [x] Verify raw reads against a PC-side sector dump
- [x] Read-only SD block-device API
- [x] Runtime SD recovery
- [x] Remount FatFs after successful recovery

> v1.0 supports SDHC and SDXC cards only.

---

## Shared SPI integration

- [x] W5500 and SD card share one SPI bus
- [x] Separate chip-select lines
- [x] FreeRTOS mutex protection
- [x] Verify SD access while networking is active
- [x] Logic-analyzer verification
- [x] Verify CS lines do not overlap
- [x] Verify SD trailing clocks

### SD MISO contention

- [x] Identify shared-MISO contention caused by the SD module
- [x] Add 2 kΩ series resistor on SD MISO
- [x] Verify reliability improvement

```text id="r9f4x2"
SD module MISO -> 2 kΩ -> shared MISO
```

---

## Filesystem

- [x] Integrate FatFs in read-only mode
- [x] FatFs disk I/O bridge
- [x] Multi-sector reads
- [x] Mount SD card
- [x] Open and read files
- [x] Read files while networking is active

---

## Networking

- [x] Port W5500 ioLibrary
- [x] Static IPv4 configuration
- [x] TCP server sockets
- [x] Use all 8 W5500 hardware sockets
- [x] Receive HTTP requests
- [x] Request timeout handling
- [x] Handle partial `send()` results
- [x] Sequential and concurrent request testing
- [x] Runtime W5500 recovery

---

## Web server

- [x] Serve files directly from SD card
- [x] Use `0:/static` as the web root
- [x] Root, file, nested-directory, and query-string URL handling
- [x] Stream files in chunks
- [x] HTTP `200`, `400`, and `404` responses
- [x] MIME type detection
- [x] Path validation

---

## Release configuration

- [x] W5500 SPI frequency: 16 MHz
- [x] SD initialization frequency: 250 kHz
- [x] SD normal-operation frequency: 2 MHz
- [x] Mixed-file stress testing
- [x] Concurrent request testing
- [x] SD recovery testing
- [x] Network recovery testing
- [x] Browser serving testing
- [x] Block and wiring diagrams
- [x] README updated

**v1.0 ready.**
