# nRF52840 web server

> [!WARNING]\
> Work in progress!

Writing a web server for the nRF52840 microcontroller to learn complex embedded systems programming
and for fun. Built using FreeRTOS.

### Work:

- [x] SPI driver
- [x] UARTE driver
- [x] Logger module - Ring buffer for log messages.
- [ ] W5500 port to work with SPI driver
- [ ] Internet working over ethernet(W5500)
- [ ] SD card reader driver
- [ ] Filesystem for SD card
- ... and more to come

#### Estimated date of completion: Early March 2026

#### Skipped for now/TODOs for later:

- SPI Driver:
  - Fix frequency selector arg in SPI driver
- Logger module:
  - Implement ISR Logging fns
  - Block indefinitely until a producer wakes the logger - task notification
