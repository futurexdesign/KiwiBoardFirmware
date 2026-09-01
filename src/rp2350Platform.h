#pragma once

#include "platformCommon.h"

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * RP2350 (generic_rp2350) board: TMC2209 UART on expansion header, no sounder.
 * LCD TFT_* build flags: see platformio.ini [env:rp2350].
 */

class RP2350Platform : public PlatformCommon {
    public:
        RP2350Platform();

        void enableSounder(bool activate) override;

        MotorDriver *createMotorDriver() override;
        void pollScreenCapture() override;

    protected:
        void initializeHardware() override;

        static constexpr PlatformPins PINS = {
            .motor_en = 9,
            .fan_ctl = 14,
            .heater_ctl = 15,
            .button = 18,
            .enc1 = 19,
            .enc2 = 20,
            .expansion1 = 0,
            .expansion2 = 1,
            .expansion3 = 2,
            .expansion4 = 3,
            .tmc_sclk = 10,
            .tmc_mosi = 11,
            .tmc_miso = 12,
            .tmc_ss = 13,
            .tmc_uart_tx = 0,
            .tmc_uart_rx = 1,
            .lcd_miso = 36,
            .lcd_sck = 34,
            .lcd_mosi = 39,
            .lcd_cs = 30,
            .lcd_dc = 32,
            .lcd_reset = 31,
            .lcd_backlight = 37,
            .status = 25,
        };
};
