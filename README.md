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

## Project progress

For the detailed implementation checklist, completed milestones, and upcoming work, see [PROGRESS.md](PROGRESS.md).
