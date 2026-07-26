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

**NOTE** When you regnerate the code in tcMenu Designer, you will need to add in the include for picoPlatform.h to the kiwiboardMenu.h file, as the project is set to reference the defines done in the header file. 


## Calculations for TMC5160 
See `TMC_calculations.xlsx` spreadsheet for information on current calculations. 
- GlobalScaler should be set such that IRun = 31 represents the maximum RMS current desired.  This makes 
sure the full range of iRun represents all available current settings, and we can't accidentally try and send too much current
to the stepper motor when adjusting iRun. 

## Motor driver build targets

PlatformIO supports compile-time selection and boot-time autodetection, for both the
RP2040 (Pico) board and the RP2350 (`generic_rp2350`) board revision (see
[Board variants](#board-variants) below):

- `pio run -e pico` / `pico-autodetect` — probe TMC2209 then TMC5160 at boot (recommended)
- `pio run -e pico-tmc5160` — force SPI TMC5160
- `pio run -e pico-tmc2209` — force TMC2209 with expansion-header UART (TX/RX + 1k)
- `pio run -e pico-tmc2209-halfduplex` — force TMC2209 with single-pin half-duplex on `TMC_SS`
- `pio run -e rp2350` / `rp2350-autodetect` — same as `pico-autodetect`, for the RP2350 board
- `pio run -e rp2350-tmc5160` — same as `pico-tmc5160`, for the RP2350 board
- `pio run -e rp2350-tmc2209` — same as `pico-tmc2209`, for the RP2350 board
- `pio run -e rp2350-tmc2209-halfduplex` — same as `pico-tmc2209-halfduplex`, for the RP2350 board

### Autodetect probe order

1. Half-duplex UART on `TMC_SS` (GPIO13) — accept TMC2209 if `version() == 0x21`
2. If no response, reconfigure GPIO13 as SPI chip-select and probe TMC5160 IOIN — accept if version `0x30`

### TMC2209 half-duplex wiring (autodetect / halfduplex builds)

PDN_UART is hardwired to `TMC_SS` (GPIO13). Firmware uses that pin alone for
half-duplex UART (TX and RX). Expansion header pins are not used.

```
GPIO13 (TMC_SS) -------- PDN_UART (hardwired)
```

SPI SCLK/MOSI/MISO (GPIO10-12) are driven low during 2209 operation.

### TMC2209 expansion UART wiring (`pico-tmc2209` only)

| Expansion | GPIO | UART |
|-----------|------|------|
| EXPANSION1 | GPIO0 | TX |
| EXPANSION2 | GPIO1 | RX |

```
GPIO0 (TX) ---[1k]---+--- PDN_UART (TMC2209)
GPIO1 (RX) ----------+
```

Sounder and screen-capture uses of the expansion pins are disabled in this build so these pins are free for UART.

See also [`plans/tmc2209-runtime-detection-recommendation.md`](plans/tmc2209-runtime-detection-recommendation.md).

## Board variants

The firmware supports two board revisions, sharing all of the same source and the
full motor driver build matrix described above. The board is selected purely by
which PlatformIO environment you build/upload (`pico*` vs `rp2350*`); no other
setup is required.

| | RP2040 (`pico*`) | RP2350 (`rp2350*`, `generic_rp2350`) |
|---|---|---|
| MCU | RP2040 | RP2350 |
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
(GPIO20 on `pico*`, GPIO30 on `rp2350*`); `LCD_CS` in `picoPlatform.h` is only used for
the (unused, hardware-CS-disabled) Arduino `SPI` object pin mapping. This mismatch
pre-dates the RP2350 variant and is preserved as-is on `pico*` builds.

All other pins (motor enable, heater/fan control, expansion header, TMC SPI/UART pins)
are unchanged between board revisions. The RP2350 pin overrides live in
`src/picoPlatform.h`, guarded by `ARDUINO_GENERIC_RP2350` (defined automatically by
the `generic_rp2350` board definition), and the LCD SPI pins are additionally set via
`-DTFT_*` build flags in the `rp2350-*` environments of `platformio.ini`.

