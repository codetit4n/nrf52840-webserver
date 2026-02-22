# nRF52840 web server

> [!WARNING]\
> Work in progress!

Writing a web server for the nRF52840 microcontroller to learn complex embedded systems programming
and for fun. Built using FreeRTOS.

### Work progress:

- [x] UARTE driver
- [x] Logger module - Ring buffer for log messages.
- [x] SPI driver
- [x] W5500 ioLibrary port to work with SPI driver
- [ ] Internet working over ethernet(W5500)
- [ ] SD card reader driver
- [ ] Filesystem for SD card
- ... and more to come

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

#### Skipped for now/TODOs for later:

- Logger module:
  - Implement ISR Logging fns
  - make `payload_t` enum `unit8_t` backed - optimization
