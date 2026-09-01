# KiwiBoardFirmware
Firmware for KiwiBoard upgrade to KiwiClean watch cleaning machine. 

![screenshot_21984](https://user-images.githubusercontent.com/8275093/234726693-8163635c-4dfb-4c79-9a8b-26653372aa87.png)


## Install
To install the firmware, you can download a release from the github page.  It contains a .uf2 file with the firmware. 

1) Depress the `bootsel` button on the microcontroller board, while holding it in, connect to the machine via USB, release the button. 
2) The machine will show up on your computer as a flash drive. 
3) Copy the `.uf2` file contianing the firmware to the drive that appeared on your computer. 
4) The machine will automatically disconnect from USB, flash the new firmware and reboot. 
5) Disconnect from usb once the machine boots. 
6) Power up the machine from it's normal power supply. 

## Build

Build is done via platform.io 

Using the aurdino-pico arduino core instead of the default mbed based version.   arudino-pico allows more flexability with the hardware.   

Windows user: Must enable long-path support to get the platform to install correctly: Ref: https://arduino-pico.readthedocs.io/en/latest/platformio.html#important-steps-for-windows-users-before-installing  

If you are on windows 11 home, you will need to enable gpedit inorder to enable long path support (https://www.itechtics.com/enable-gpedit-msc-windows-11/) 

## User Interface
The user interface is coded using tcMenu.   To edit the menu structure, the tcMenu Designer tool should be used.   The kiwiboard.emf file contains the menu definitions. 

## Calculations for TMC5160 
See `TMC_calculations.xlsx` spreadsheet for information on current calculations. 
- GlobalScaler should be set such that IRun = 31 represents the maximum RMS current desired.  This makes 
sure the full range of iRun represents all available current settings, and we can't accidentally try and send too much current
to the stepper motor when adjusting iRun. 

## Build targets

Hardware is bifurcated by MCU board revision. Each PlatformIO environment builds
one board + motor driver combination:

- `pio run -e pico` — RP2040 (Pico) with TMC5160 over SPI
- `pio run -e rp2350` — RP2350 (`generic_rp2350`) with TMC2209 over UART

### TMC2209 UART wiring (RP2350 builds)

| Expansion | GPIO | UART |
|-----------|------|------|
| EXPANSION1 | GPIO0 | TX |
| EXPANSION2 | GPIO1 | RX |

```
GPIO0 (TX) ---[1k]---+--- PDN_UART (TMC2209)
GPIO1 (RX) ----------+
```

Sounder and screen-capture uses of the expansion pins are disabled on RP2350 so
these pins are free for UART.

## Board variants

| | RP2040 (`pico`) | RP2350 (`rp2350`, `generic_rp2350`) |
|---|---|---|
| MCU | RP2040 | RP2350 |
| Motor driver | TMC5160 (SPI) | TMC2209 (UART) |
| `BUTTON` | GPIO6 | GPIO18 |
| `ENC1` | GPIO5 | GPIO19 |
| `ENC2` | GPIO4 | GPIO20 |
| LCD `MISO` | GPIO16 | GPIO36 |
| LCD `MOSI` | GPIO19 | GPIO39 |
| LCD `SCLK` | GPIO18 | GPIO34 |
| LCD `CS` | GPIO17† | GPIO30 |
| LCD `DC` | GPIO21 | GPIO32 |
| LCD `RESET` | GPIO22 | GPIO31 |
| LCD `BACKLIGHT` | GPIO26 | GPIO37 |

† The actual TFT_eSPI chip-select is set independently via the `-DTFT_CS` build flag
(GPIO20 on `pico`, GPIO30 on `rp2350`); `lcd_cs` in the platform pin tables is only used for
the (unused, hardware-CS-disabled) Arduino `SPI` object pin mapping. This mismatch
pre-dates the RP2350 variant and is preserved as-is on `pico` builds.

All other pins (motor enable, heater/fan control, expansion header, TMC SPI/UART pins)
are unchanged between board revisions. Board-specific GPIO mappings live in
[`src/rp2040Platform.h`](src/rp2040Platform.h) and [`src/rp2350Platform.h`](src/rp2350Platform.h);
PlatformIO selects the implementation at link time via `build_src_filter`. LCD SPI pins
are additionally set via `-DTFT_*` build flags in `platformio.ini`.

