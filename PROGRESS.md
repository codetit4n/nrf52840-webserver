# Project progress

This file tracks implementation progress for the nRF52840 web server.

## Current milestone: v1.0

Build a low-cost embedded web server around the nRF52840 using the W5500 for Ethernet and a microSD card for persistent file storage.

The system uses a shared SPI bus protected by FreeRTOS synchronization, a ring-buffer logger over UARTE, a custom SDHC/SDXC driver, FatFs in read-only mode, and a networking task based on the W5500 ioLibrary.

The v1.0 milestone is focused on serving static files directly from the SD card over HTTP while keeping the W5500 and SD card operating on the same SPI bus.

---

## Core drivers

- [x] UARTE driver

- [x] Ring-buffer logger
  - [x] Queue complete log lines before transmission
  - [x] Logger flush support
  - [x] Literal logging
  - [x] Hex logging
  - [x] Integer logging

- [x] Generic SPI master driver
  - [x] FreeRTOS mutex-protected shared bus
  - [x] Multi-device configuration
  - [x] Per-device SPI configuration
  - [x] Blocking TX/RX with timeout
  - [x] EasyDMA-safe buffer handling
  - [x] Support W5500 and SD card on the same bus

---

## System startup

- [x] Ordered startup initialization
  - [x] Initialize the SD card
  - [x] Mount the filesystem
  - [x] Initialize the W5500
  - [x] Create the networking task
  - [x] Delete the startup task after initialization

- [x] Return initialization errors instead of halting inside `net_init()`

---

## SD card support

- [x] SDHC/SDXC initialization in SPI mode
  - [x] CMD0
  - [x] CMD8
  - [x] CMD55 + ACMD41
  - [x] CMD58
  - [x] SDHC/SDXC detection
  - [x] Reject unsupported legacy cards

- [x] Read CSD using CMD9
  - [x] Keep CS asserted for the complete data transaction
  - [x] Wait for the `0xFE` data token
  - [x] Read the 16-byte CSD
  - [x] Read and discard CRC bytes

- [x] Decode card block count from CSD v2

- [x] Read a single 512-byte block using CMD17
  - [x] Build CMD17 argument from the requested block number
  - [x] Send block number in big-endian wire order
  - [x] Wait for the `0xFE` data token
  - [x] Read 512 data bytes
  - [x] Read and discard CRC bytes

- [x] Verify block 0 against a PC-side sector dump
- [x] Verify repeated raw reads are stable
- [ ] Verify a non-zero block against a PC-side sector dump
- [ ] Add CRC16 validation for received data blocks

- [x] Read-only block-device API
  - [x] `sd_status_t`
  - [x] Specific error codes
  - [x] Initialization-state tracking
  - [x] Block-range validation
  - [x] Private command helpers
  - [x] Defined command transaction ownership
  - [x] SD initialization API
  - [x] Block-read API
  - [x] Readiness query
  - [x] Block-count query

> Current limitation: only SDHC and SDXC cards are supported. SDSC and SD v1 cards are intentionally rejected.

---

## Shared SPI integration

- [x] Run W5500 and SD card on the same SPI bus
- [x] Protect shared bus access using a FreeRTOS mutex
- [x] Verify both devices work independently
- [x] Verify SD reads while networking is active
- [x] Capture SD/W5500 transactions using a logic analyzer
- [x] Verify SD and W5500 CS lines do not overlap
- [x] Verify SD trailing clocks after deselection

### SD module MISO contention

- [x] Identify intermittent corruption after SD → W5500 transitions
- [x] Trace the SD module MISO path
- [x] Identify always-enabled SN74HC125 MISO buffer
- [x] Confirm MISO buffer OE is tied to GND instead of SD CS
- [x] Confirm shared-MISO contention as the main hardware issue
- [x] Add a 2 kΩ series resistor to the SD module MISO line as a workaround
- [x] Verify major reliability improvement after adding the resistor
- [ ] Replace workaround with proper HC125 OE control as a post-v1 hardware experiment

> Current hardware workaround:
>
> `SD module MISO -> 2 kΩ -> shared MISO`
>
> The proper hardware fix is to make the HC125 MISO output-enable follow SD CS so the module releases MISO when deselected.

---

## Filesystem

- [x] Integrate FatFs in read-only mode

- [x] FatFs disk I/O bridge
  - [x] `disk_status()`
  - [x] `disk_initialize()`
  - [x] `disk_read()`
  - [x] `disk_ioctl()`
  - [x] Multi-sector buffer handling
  - [x] Sector-range validation

- [x] Mount the SD card
- [x] Open and read files from the SD card
- [x] Read `/INDEX.HTML`
- [x] Read files while networking is active
- [ ] Wrap FatFs access in a small filesystem module
- [ ] Add write support later, if needed

---

## Networking

- [x] Port W5500 ioLibrary
- [x] Static IPv4 configuration
- [x] Create TCP server sockets
- [x] Use multiple W5500 hardware sockets
- [x] Add readable socket-state logging
- [x] Receive HTTP requests
- [x] Detect complete HTTP headers using `\r\n\r\n`
- [x] Add request timeout handling
- [x] Add request-processing timeout
- [x] Test repeated HTTP requests
- [x] Test concurrent HTTP requests
- [x] Handle partial `send()` results
  - [x] Add `net_send_all()`
  - [x] Use it for HTTP headers
  - [x] Use it for file chunks
  - [x] Use it for 404 responses

### Known networking limitation

- [ ] Investigate occasional W5500 socket/listen failure after repeated requests

Observed occasionally during stress testing:

- TCP connection reset by peer
- `listen()` occasionally returns `SOCKERR_SOCKCLOSED`
- socket reaches `SOCK_INIT` successfully before the failed `listen()`

This is currently considered a known v1 limitation and is not blocking the remaining application-layer work.

---

## Web server

- [x] Serve files directly from the SD card
- [x] Map URL paths to FatFs paths
  - [x] `/` -> `0:/INDEX.HTML`
  - [x] General paths such as `/test.txt`
  - [x] Nested paths supported by FatFs path mapping

- [x] Stream files instead of loading them fully into RAM
  - [x] Read files in `SPI_MAX_XFER` chunks
  - [x] Fully transmit each chunk before reading the next one

- [x] Generate HTTP `200 OK` response headers
- [x] Build response headers dynamically
- [x] Send complete response headers in one buffer

- [x] MIME type detection
  - [x] HTML / HTM
  - [x] CSS
  - [x] JavaScript
  - [x] TXT
  - [x] PNG
  - [x] JPG / JPEG
  - [x] SVG
  - [x] ICO
  - [x] Unknown file types -> `application/octet-stream`

- [x] 404 responses for missing files
- [x] Browser rendering of HTML
- [x] Browser rendering of plain-text files

- [ ] Basic path sanitization
  - [ ] Reject `..`
  - [ ] Reject malformed paths
  - [ ] Enforce maximum path length
  - [ ] Prevent paths from escaping the intended web root

- [ ] Add SD read-error recovery
- [ ] Final mixed-file stress test

---

## v1.0 validation

- [ ] Test HTML page with referenced CSS
- [ ] Test HTML page with referenced JavaScript
- [ ] Test image loading from SD card
- [ ] Test favicon loading
- [ ] Test missing resources / 404 handling
- [ ] Test unknown file extension fallback
- [ ] Test invalid paths
- [ ] Run 500-1000 sequential HTTP requests
- [ ] Run concurrent request stress test
- [ ] Test mixed static-file requests
- [ ] Restore intended SPI frequencies
- [ ] Repeat reliability tests at final SPI frequencies

---

## Cleanup

- [ ] Remove temporary debug instrumentation
- [ ] Remove obsolete TODOs
- [ ] Keep useful runtime error logging
- [ ] Review networking error paths
- [ ] Review stack usage and temporary buffers
- [ ] Review `MAX_PATH_LEN`
- [ ] Review timeout values

---

## Documentation and release

- [ ] Architecture diagram
- [ ] Shared SPI wiring table
- [ ] HTTP request/file-serving flow diagram
- [ ] Document SD-module MISO contention issue
- [ ] Document the 2 kΩ MISO workaround
- [ ] Document known networking limitation
- [ ] Update build log
- [ ] README cleanup
- [ ] Add setup instructions for SD-card web files
- [ ] Demo video
- [ ] Final v1.0 stress test
- [ ] Tag `v1.0`

---

## Post-v1 ideas

- [ ] Proper HC125 OE modification on the SD module
- [ ] Filesystem abstraction module
- [ ] SD CRC16 validation
- [ ] SD write support
- [ ] More HTTP methods
- [ ] HTTP keep-alive
- [ ] HTTP caching / ETag support
- [ ] Directory handling
- [ ] Dynamic responses
- [ ] Custom PCB
