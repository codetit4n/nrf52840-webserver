# nRF52840 web server

> [!WARNING]\
> Work in progress!

Writing a web server for the nRF52840 microcontroller to learn complex embedded systems programming
and for fun. Built using FreeRTOS.

#### Clone with submodules:

```shell
git clone --recurse-submodules https://github.com/codetit4n/nrf52840-webserver
```

### Commands:

```bash
# Build the firmware - Creates the firmware binary in the build/ directory
make
# Flash the firmware to the nRF52840
make flash
# Or, directly use nrfjprog
nrfjprog --program build/webserver.elf --chiperase --verify --reset
# Clean the build artifacts
make clean
# Monitor the serial output from the nRF52840 for debugging - 1M baud rate
minicom -D /dev/ttyACM0 -b 1000000
```

### Work progress

#### Core drivers

- [x] UARTE driver
- [x] Ring-buffer logger
- [x] Generic SPI master driver
  - [x] FreeRTOS mutex-protected bus
  - [x] Multi-device configuration
  - [x] Blocking TX/RX with timeout
  - [x] EasyDMA-safe buffer handling

#### Networking

- [x] W5500 ioLibrary port
- [x] Minimal networking module
  - [x] Static IP configuration
  - [x] Hardcoded HTTP response
- [ ] Re-enable W5500 alongside SD card
- [ ] Serve files from SD card over HTTP
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

- [x] Read CSD using CMD9
  - [x] Keep CS asserted for the complete data transaction
  - [x] Wait for the `0xFE` data token
  - [x] Read 16-byte CSD
  - [x] Read and discard CRC bytes

- [x] Decode card block count from CSD v2
- [x] Read a single 512-byte block using CMD17
  - [x] Build the CMD17 argument from the requested block number
  - [x] Send the block number in big-endian wire order
  - [x] Wait for the `0xFE` data token
  - [x] Read 512 data bytes
  - [x] Read and discard CRC bytes

- [x] Verify block 0 against a PC-side sector dump
- [ ] Verify repeated raw reads are stable
- [ ] Verify a non-zero block against a PC-side sector dump
- [ ] Add CRC16 validation for received data blocks
- [x] Create a clean read-only block-device API
  - [x] Add `sd_status_t`
  - [x] Return specific errors instead of `-1`
  - [x] Add initialization-state tracking
  - [x] Add block-range validation
  - [x] Keep command helpers private to the SD implementation
  - [x] Define command transaction ownership rules
  - [x] Expose only `sd_init()` and `sd_read_block()` publicly

- [ ] Verify SD and W5500 operation on the shared SPI bus

> Current limitation: only SDHC and SDXC cards are supported. SDSC and SD v1 cards are intentionally rejected.

#### Filesystem

- [ ] Integrate FatFs in read-only mode
- [ ] Implement FatFs disk I/O bridge
- [ ] Mount the SD card
- [ ] Open and read `/index.html`
- [ ] Print file contents over UART
- [ ] Add write support later, if needed

#### Web server

- [ ] Re-enable networking with SD card active
- [ ] Serve `/index.html` from SD
- [ ] Stream files in chunks instead of loading them fully into RAM
- [ ] Test repeated HTTP requests
- [ ] Add socket and SD timeout recovery

#### Documentation and release

- [ ] Architecture diagram
- [ ] Wiring table
- [ ] Build log
- [ ] README cleanup
- [ ] Demo video
- [ ] Tag a v1.0 release
