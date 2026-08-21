# nRF52840 web server

https://loke.sh/blog/nrf52840-web-server

> [!IMPORTANT]
> Subscribe to my [Newsletter](https://loke.sh/blog/newsletter/) for updates on this project's technical write-ups.

Writing a web server for the [nRF52840](https://www.nordicsemi.com/Products/nRF52840) microcontroller to learn complex
embedded systems programming and for fun. Built using FreeRTOS.

## Commands

#### Clone with submodules:

```shell
git clone --recurse-submodules https://github.com/codetit4n/nrf52840-webserver
```

#### Build, Flash and Clean:

```shell
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

## Static file server

Static files served over HTTP must be placed inside a `static` directory at the root of the SD card.

For example:

```text
/
└── static/
    ├── INDEX.HTML
    ├── STYLE.CSS
    ├── APP.JS
    └── images/
        └── logo.png
```

HTTP paths are mapped to files inside this directory:

```text
/                -> 0:/static/INDEX.HTML
/style.css       -> 0:/static/style.css
/app.js          -> 0:/static/app.js
/images/logo.png -> 0:/static/images/logo.png
```

The `static` directory acts as the web root. Other directories on the SD card are kept separate from files exposed by the static server.

## Project progress

For the detailed implementation checklist, completed milestones, and upcoming work, see [PROGRESS.md](PROGRESS.md).
