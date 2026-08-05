# Project progress

This file tracks implementation progress for the nRF52840 web server.

## Current milestone: v1.0

Build a low-cost embedded web server around the nRF52840 using the W5500 for Ethernet and an SD card for persistent file storage.

The system uses a shared SPI bus protected by FreeRTOS synchronization, a ring-buffer logger over UARTE, a custom SDHC/SDXC driver, FatFs in read-only mode, and a networking task based on the W5500 ioLibrary.

The current milestone is to make the SD card and Ethernet stack operate reliably together, then serve static files such as HTML, CSS, and JavaScript directly from the SD card over HTTP.

## Core drivers

- [x] UARTE driver
- [x] Ring-buffer logger
  - [x] Queue complete log lines before transmission
  - [x] Add logger flush support
- [x] Generic SPI master driver
  - [x] FreeRTOS mutex-protected shared bus
  - [x] Multi-device configuration
  - [x] Blocking TX/RX with timeout
  - [x] EasyDMA-safe buffer handling

#### System startup

- [x] Add ordered startup initialization
  - [x] Initialize the SD card first
  - [x] Mount the filesystem
  - [x] Initialize the W5500
  - [x] Create the networking task
  - [x] Delete the startup task after initialization
- [x] Return initialization errors instead of halting inside `net_init()`

#### Networking

- [x] W5500 ioLibrary port
- [x] Minimal networking module
  - [x] Static IP configuration
  - [x] Hardcoded HTTP response
- [x] Run W5500 alongside the SD card on the shared SPI bus
- [x] Add readable socket-state logging
- [x] Test repeated HTTP requests with the hardcoded response
- [ ] Investigate occasional TCP connection resets
- [ ] Serve files from the SD card over HTTP
- [ ] Add basic static-file support
  - [ ] HTML
  - [ ] CSS
  - [ ] JavaScript
  - [ ] MIME types
  - [ ] 404 responses
  - [ ] Basic path sanitization

#### SD card support

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
  - [x] Build the CMD17 argument from the requested block number
  - [x] Send the block number in big-endian wire order
  - [x] Wait for the `0xFE` data token
  - [x] Read 512 data bytes
  - [x] Read and discard CRC bytes

- [x] Verify block 0 against a PC-side sector dump
- [x] Verify repeated raw reads are stable
- [ ] Verify a non-zero block against a PC-side sector dump
- [ ] Add CRC16 validation for received data blocks

- [x] Create a clean read-only block-device API
  - [x] Add `sd_status_t`
  - [x] Return specific errors instead of `-1`
  - [x] Add initialization-state tracking
  - [x] Add block-range validation
  - [x] Keep command helpers private to the SD implementation
  - [x] Define command transaction ownership rules
  - [x] Expose SD initialization, block reads, readiness, and block-count queries

- [x] Verify SD and W5500 operation on the shared SPI bus
  - [x] Initialize both devices successfully
  - [x] Perform SD reads while networking is active
  - [x] Confirm shared-bus mutex protection works

> Current limitation: only SDHC and SDXC cards are supported. SDSC and SD v1 cards are intentionally rejected.

#### Filesystem

- [x] Integrate FatFs in read-only mode
- [x] Implement the FatFs disk I/O bridge
  - [x] `disk_status()`
  - [x] `disk_initialize()`
  - [x] `disk_read()`
  - [x] `disk_ioctl()`
  - [x] Multi-sector buffer handling
  - [x] Sector-range validation
- [x] Mount the SD card
- [x] Open and read a test file
- [x] Print file contents over UART
- [ ] Open and read `/index.html`
- [ ] Wrap FatFs access in a small filesystem module
- [ ] Add write support later, if needed

#### Web server

- [x] Re-enable networking with the SD card active
- [ ] Map `/` to `/index.html`
- [ ] Serve `/index.html` from the SD card
- [ ] Stream files in chunks instead of loading them fully into RAM
- [ ] Send appropriate HTTP response headers
- [ ] Test repeated requests while reading files from the SD card
- [ ] Add socket timeout and recovery handling
- [ ] Add SD read-error recovery

#### Documentation and release

- [ ] Architecture diagram
- [ ] Shared SPI wiring table
- [ ] Filesystem and request-flow diagram
- [ ] Update the build log
- [ ] README cleanup
- [ ] Demo video
- [ ] Tag a v1.0 release
