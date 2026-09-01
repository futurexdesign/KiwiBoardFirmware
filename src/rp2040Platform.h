#pragma once

#include "platformCommon.h"

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * RP2040 (Pico) board: TMC5160 SPI, sounder PWM on expansion header.
 * LCD TFT_* build flags: see platformio.ini [env:pico].
 */

class RP2040Platform : public PlatformCommon {
    public:
        RP2040Platform();

        void enableSounder(bool activate) override;

        MotorDriver *createMotorDriver() override;
        void pollScreenCapture() override;

    protected:
        void initializeHardware() override;
        void onExecTick() override;

    private:
        static uint32_t pwmSetFreqDuty(uint slice_num, uint chan, uint32_t f, int d);

        uint pwm_slice = 0;
        uint pwm_channel = 0;

        static constexpr PlatformPins PINS = {
            .motor_en = 9,
            .fan_ctl = 14,
            .heater_ctl = 15,
            .button = 6,
            .enc1 = 5,
            .enc2 = 4,
            .expansion1 = 0,
            .expansion2 = 1,
            .expansion3 = 2,
            .expansion4 = 3,
            .tmc_sclk = 10,
            .tmc_mosi = 11,
            .tmc_miso = 12,
            .tmc_ss = 13,
            .tmc_uart_tx = 0,
            .tmc_uart_rx = 0,
            .lcd_miso = 16,
            .lcd_sck = 18,
            .lcd_mosi = 19,
            .lcd_cs = 17,
            .lcd_dc = 21,
            .lcd_reset = 22,
            .lcd_backlight = 26,
            .status = 25,
        };
};
