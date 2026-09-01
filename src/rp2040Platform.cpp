/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include <Arduino.h>
#include <SPI.h>
#include "rp2040Platform.h"
#include "platformFactory.h"
#include "motorDriver.h"
#include "tmc5160Driver.h"
#include "hardware/pwm.h"

#ifdef SCREENCAP
#include "screenServer.h"
#endif

#define SOUNDER_PWM_FREQ 3000

RP2040Platform::RP2040Platform() : PlatformCommon(PINS) {}

void RP2040Platform::initializeHardware() {
    pinMode(pins.expansion1, OUTPUT_12MA);
    digitalWrite(pins.expansion1, HIGH);

    gpio_set_function(pins.expansion1, GPIO_FUNC_PWM);
    pwm_slice = pwm_gpio_to_slice_num(pins.expansion1);
    pwm_channel = pwm_gpio_to_channel(pins.expansion1);

    pwmSetFreqDuty(pwm_slice, pwm_channel, SOUNDER_PWM_FREQ, sndLevel);
    pwm_set_enabled(pwm_slice, true);

    digitalWrite(pins.expansion1, HIGH);

    SPI1.setRX(pins.tmc_miso);
    SPI1.setTX(pins.tmc_mosi);
    SPI1.setSCK(pins.tmc_sclk);
    SPI1.begin(false);
    pinMode(pins.tmc_ss, OUTPUT);
    digitalWrite(pins.tmc_ss, HIGH);

    pinMode(pins.expansion4, INPUT_PULLDOWN);
}

void RP2040Platform::enableSounder(bool activate) {
    if (!activate) {
        pinMode(pins.expansion1, OUTPUT_12MA);
        digitalWrite(pins.expansion1, true);
    } else {
        gpio_set_function(pins.expansion1, GPIO_FUNC_PWM);
        pwm_set_enabled(pwm_slice, activate);
    }
}

void RP2040Platform::onExecTick() {
    pwmSetFreqDuty(pwm_slice, pwm_channel, SOUNDER_PWM_FREQ, sndLevel);
}

MotorDriver *RP2040Platform::createMotorDriver() {
    return new TMC5160Driver();
}

void RP2040Platform::pollScreenCapture() {
#ifdef SCREENCAP
    if (digitalRead(pins.expansion4) == HIGH) {
        screenServer();
    }
#endif
}

uint32_t RP2040Platform::pwmSetFreqDuty(uint slice_num, uint chan, uint32_t f, int d) {
    uint32_t clock = 125000000;
    uint32_t divider16 = clock / f / 4096 + (clock % (f * 4096) != 0);
    if (divider16 / 16 == 0) {
        divider16 = 16;
    }
    uint32_t wrap = clock * 16 / divider16 / f - 1;
    pwm_set_clkdiv_int_frac(slice_num, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap * d / 100);
    return wrap;
}

Platform *createPlatform() {
    return new RP2040Platform();
}
