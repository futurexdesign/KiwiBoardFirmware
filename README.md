# KiwiBoardFirmware
Firmware for KiwiBoard upgrade to KiwiClean watch cleaning machine. 

Build is done via platform.io 

Using the aurdino-pico arduino core instead of the default mbed based version.   arudino-pico allows more flexability with the hardware.   

Windows user: Must enable long-path support to get the platform to install correctly: Ref: https://arduino-pico.readthedocs.io/en/latest/platformio.html#important-steps-for-windows-users-before-installing  

If you are on windows 11 home, you will need to enable gpedit inorder to enable long path support (https://www.itechtics.com/enable-gpedit-msc-windows-11/) 

## Calculations for TMC5160 
See `TMC_calculations.xlsx` spreadsheet for information on current calculations. 
- GlobalScaler should be set such that IRun = 31 represents the maximum RMS current desired.  This makes 
sure the full range of iRun represents all available current settings, and we can't accidentally try and send too much current
to the stepper motor when adjusting iRun. 
- 