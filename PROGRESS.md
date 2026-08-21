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
  - [x] Literal logging
  - [x] Hex logging
  - [x] Integer logging
  - [x] Logger flush support

- [x] Generic SPI master driver
  - [x] FreeRTOS mutex-protected shared bus
  - [x] Per-device configuration
  - [x] Blocking TX/RX with timeout
  - [x] EasyDMA-safe buffer handling
  - [x] Shared W5500 + SD operation

---

## SD card

- [x] SDHC/SDXC initialization in SPI mode
  - [x] CMD0
  - [x] CMD8
  - [x] CMD55 + ACMD41
  - [x] CMD58
  - [x] Reject unsupported legacy cards

- [x] Read and decode CSD v2 using CMD9
- [x] Determine card block count
- [x] Read 512-byte blocks using CMD17
- [x] Verify block 0 against a PC-side sector dump
- [x] Verify repeated raw reads are stable

- [x] Read-only SD block-device API
  - [x] Error/status codes
  - [x] Initialization-state tracking
  - [x] Block-range validation
  - [x] Initialization API
  - [x] Block-read API
  - [x] Block-count query

- [x] Runtime SD recovery
  - [x] Track raw initialization state
  - [x] Track filesystem availability
  - [x] Trigger recovery after SD access failures
  - [x] Bounded recovery attempts
  - [x] Remount FatFs after successful recovery

> v1.0 supports SDHC and SDXC cards only.

---

## Shared SPI integration

- [x] W5500 and SD card share one SPI bus
- [x] Separate chip-select lines
- [x] FreeRTOS mutex protection
- [x] Verify both devices independently
- [x] Verify SD access while networking is active
- [x] Logic-analyzer verification
- [x] Verify CS lines do not overlap
- [x] Verify SD trailing clocks

### SD MISO contention

- [x] Identify corruption after SD -> W5500 transitions
- [x] Trace SD module MISO path
- [x] Identify always-enabled SN74HC125 output
- [x] Confirm OE is tied low
- [x] Confirm shared-MISO contention
- [x] Add 2 kΩ series resistor on SD MISO
- [x] Verify major reliability improvement

Current workaround:

```text
SD module MISO -> 2 kΩ -> shared MISO
```

---

## Filesystem

- [x] Integrate FatFs in read-only mode

- [x] FatFs disk I/O bridge
  - [x] `disk_status()`
  - [x] `disk_initialize()`
  - [x] `disk_read()`
  - [x] `disk_ioctl()`
  - [x] Multi-sector reads
  - [x] Sector-range validation

- [x] Mount SD card
- [x] Open and read files
- [x] Read files while networking is active

---

## Networking

- [x] Port W5500 ioLibrary
- [x] Static IPv4 configuration
- [x] TCP server sockets
- [x] Use all 8 W5500 hardware sockets
- [x] Readable socket-state logging
- [x] Receive HTTP requests
- [x] Detect complete HTTP headers using `\r\n\r\n`
- [x] Request timeout handling
- [x] Request-processing timeout
- [x] Handle partial `send()` results with `net_send_all()`
- [x] Sequential request testing
- [x] Concurrent request testing

### Runtime network recovery

- [x] Detect repeated `socket()` / `listen()` failures
- [x] Trigger recovery after consecutive failures
- [x] Reset and reinitialize W5500
- [x] Restore network configuration
- [x] Reset cached socket states
- [x] Recover networking without resetting the nRF52840

---

## Web server

- [x] Serve files directly from SD card
- [x] Use `0:/static` as the web root

- [x] URL mapping
  - [x] `/` -> `0:/static/INDEX.HTML`
  - [x] `/file.ext` -> `0:/static/file.ext`
  - [x] Nested paths
  - [x] `/foo/` -> `0:/static/foo/index.html`
  - [x] Ignore URL query strings when resolving files

- [x] Stream files instead of loading them fully into RAM
- [x] Read and transmit files in chunks

- [x] HTTP `200 OK` responses
- [x] HTTP `400 Bad Request`
- [x] HTTP `404 Not Found`
- [x] `Connection: close`

- [x] MIME type detection
  - [x] HTML / HTM
  - [x] CSS
  - [x] JavaScript
  - [x] TXT
  - [x] PNG
  - [x] JPG / JPEG
  - [x] SVG
  - [x] ICO
  - [x] Unknown -> `application/octet-stream`

### Path validation

- [x] Require paths to begin with `/`
- [x] Reject `..`
- [x] Reject `\`
- [x] Reject `:`
- [x] Reject non-printable characters
- [x] Enforce maximum path length
- [x] Prevent escaping the static web root

---

## Release configuration

- [x] W5500 SPI frequency: 16 MHz
- [x] SD initialization frequency: 250 kHz
- [x] SD normal-operation frequency: 2 MHz
- [x] Final mixed-file stress test completed
- [x] Concurrent request testing completed
- [x] SD recovery tested
- [x] Network recovery tested
- [x] Browser serving tested
- [x] HTML/CSS/JS/image/favicon serving tested individually

---

## Remaining v1.0 release work

- [ ] Remove temporary debug instrumentation
- [ ] Remove obsolete TODOs
- [ ] Keep useful runtime error/recovery logs
- [ ] Review `MAX_PATH_LEN`
- [ ] Review timeout values
- [ ] Review stack usage and temporary buffers

- [ ] Update README
  - [ ] Architecture overview
  - [ ] Wiring / pin table
  - [ ] SD card `/static` layout
  - [ ] SPI frequencies
  - [ ] 2 kΩ SD MISO workaround
  - [ ] Runtime recovery behavior
  - [ ] Current limitations

- [ ] Update build log
- [ ] Add architecture / flow diagrams
- [ ] Demo video or screenshots
- [ ] Final clean build
- [ ] Final flash and smoke test
- [ ] Final commit
- [ ] Tag `v1.0.0`
- [ ] Publish release
