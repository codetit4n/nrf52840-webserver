# nrf52840-webserver

nRF52840 based web server

### Uses

- Zephyr RTOS kernel

### Env

```fish
#!/usr/bin/env fish

# Activate Python virtual environment for west
source ~/zephyrproject/bin/activate.fish

# Zephyr toolchain config
set -gx ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb
set -gx GNUARMEMB_TOOLCHAIN_PATH /usr
set -gx ZEPHYR_BASE /home/tit4n/zephyrproject/zephyr
```

> Only for fish sell

### Build

```shell
source <ENV-FILE-ABOVE>
west build -b nrf52840dk/nrf52840 -d build -- -DBOARD_FLASH_RUNNER=nrfjprog
```

> For nrf52840-dk

### Flash

```shell
west flash
```
