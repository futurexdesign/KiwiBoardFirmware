#pragma once

#include <TaskManagerIO.h>
#include <stdint.h>

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

class MotorDriver;

struct PlatformPins {
    uint8_t motor_en;
    uint8_t fan_ctl;
    uint8_t heater_ctl;
    uint8_t button;
    uint8_t enc1;
    uint8_t enc2;
    uint8_t expansion1;
    uint8_t expansion2;
    uint8_t expansion3;
    uint8_t expansion4;
    uint8_t tmc_sclk;
    uint8_t tmc_mosi;
    uint8_t tmc_miso;
    uint8_t tmc_ss;
    uint8_t tmc_uart_tx;
    uint8_t tmc_uart_rx;
    uint8_t lcd_miso;
    uint8_t lcd_sck;
    uint8_t lcd_mosi;
    uint8_t lcd_cs;
    uint8_t lcd_dc;
    uint8_t lcd_reset;
    uint8_t lcd_backlight;
    uint8_t status;
};

class Platform : public Executable {
    public:
        virtual ~Platform() = default;

        virtual void initializePlatform() = 0;

        virtual void enableHeater(bool activate) = 0;
        virtual void enableFan(bool activate) = 0;
        virtual void enableMotor(bool activate) = 0;
        virtual void enableSounder(bool activate) = 0;

        virtual void set_audioLevel(int level) = 0;

        virtual void startCooldown() = 0;
        virtual void startPreheat() = 0;

        virtual void exec() override = 0;

        virtual void setBacklight(int value) = 0;

        virtual bool isMotorEnabled() = 0;
        virtual bool isHeaterEnabled() = 0;
        virtual bool isFanEnabled() = 0;

        virtual const PlatformPins &getPins() const = 0;

        virtual MotorDriver *createMotorDriver() = 0;
        virtual void pollScreenCapture() = 0;
};
