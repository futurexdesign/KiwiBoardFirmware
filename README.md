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

PlatformIO supports compile-time selection between TMC5160 (default) and TMC2209:

- `pio run -e pico-tmc5160` — SPI motion controller with hardware SixPoint ramp generator
- `pio run -e pico-tmc2209` — UART driver with internal VACTUAL pulse generator and software motion planner

The default `pico` environment aliases `pico-tmc5160`.

### TMC2209 UART wiring (expansion header)

For `pico-tmc2209` builds, hardware UART0 is mapped to the expansion header:

| Expansion | GPIO | UART |
|-----------|------|------|
| EXPANSION1 | GPIO0 | TX |
| EXPANSION2 | GPIO1 | RX |

Connect to the driver `PDN_UART` pin (single-wire):

```
GPIO0 (TX) ---[1k]---+--- PDN_UART (TMC2209)
GPIO1 (RX) ----------+
```

Sounder and screen-capture uses of the expansion pins are disabled in TMC2209 builds so these pins are free for UART.

For a future release, runtime auto-detection of the installed chip (single UF2 for both hardware revisions) is recommended but deferred until TMC2209 hardware validation is complete. See [`plans/tmc2209-runtime-detection-recommendation.md`](plans/tmc2209-runtime-detection-recommendation.md).

