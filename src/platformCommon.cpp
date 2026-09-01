/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include <Arduino.h>
#include <EEPROM.h>
#include "platformCommon.h"
#include "settings.h"
#include "kiwiDebugLog.h"

PlatformCommon::PlatformCommon(const PlatformPins &pins) : pins(pins) {}

void PlatformCommon::initializePlatform() {
    pinMode(pins.enc1, INPUT);
    pinMode(pins.enc2, INPUT);
    pinMode(pins.button, INPUT);

    pinMode(pins.motor_en, OUTPUT_8MA);
    digitalWrite(pins.motor_en, HIGH);

    pinMode(pins.heater_ctl, OUTPUT_8MA);
    digitalWrite(pins.heater_ctl, LOW);

    pinMode(pins.fan_ctl, OUTPUT_12MA);
    digitalWrite(pins.fan_ctl, LOW);

    pinMode(pins.status, OUTPUT);
    digitalWrite(pins.status, HIGH);

    pinMode(pins.lcd_backlight, OUTPUT);
    analogWrite(pins.lcd_backlight, 125);

    // Hold TFT CS idle-high. Do not call SPI.set*/begin() here:
    // - TFT_eSPI owns its own SPIClassRP2040 instance (pins from -DTFT_* flags).
    // - SPI.setCS(LCD_CS) panics on RP2350B: GPIO30 is not a valid SPI0 CS alt.
    pinMode(pins.lcd_cs, OUTPUT);
    digitalWrite(pins.lcd_cs, HIGH);

    initializeHardware();

    EEPROM.begin(768);
}

void PlatformCommon::enableHeater(bool activate) {
    digitalWrite(pins.heater_ctl, activate);
    heater_enabled = activate;

    if (activate) {
        enableFan(true);
        if (in_preheat) {
            in_preheat = false;
        }
    }
}

void PlatformCommon::enableFan(bool activate) {
    digitalWrite(pins.fan_ctl, activate);
    fan_enabled = activate;
}

void PlatformCommon::enableMotor(bool activate) {
    digitalWrite(pins.motor_en, !activate);
    motor_enabled = activate;
}

bool PlatformCommon::isFanEnabled() {
    return fan_enabled;
}

bool PlatformCommon::isHeaterEnabled() {
    return heater_enabled;
}

bool PlatformCommon::isMotorEnabled() {
    return motor_enabled;
}

void PlatformCommon::exec() {
    onExecTick();

    if (heater_enabled && !fan_enabled) {
        LOG_WARN("Force fan on, heater on...");
        enableFan(true);
    }

    if (in_cooldown) {
        LOG_DEBUG("in cooldown");
        LOG_DEBUG("cooldown_end:", cooldown_end);
        LOG_DEBUG("millis:", millis());
        if (millis() >= cooldown_end) {
            LOG_INFO("cooldown over, turn off fan");
            enableFan(false);
            in_cooldown = false;
        }
    }

    if (in_preheat) {
        if (millis() >= preheat_end) {
            startCooldown();
        }
    }

    led = !led;
    digitalWrite(pins.status, led);
}

void PlatformCommon::startCooldown() {
    SETTINGS settings = getSettings();

    enableHeater(false);
    in_preheat = false;

    if (settings.fanCooldown) {
        cooldown_start = millis();
        cooldown_end = cooldown_start + (settings.cooldownTime * 60000UL);
        in_cooldown = true;
    } else {
        enableFan(false);
    }
}

void PlatformCommon::setBacklight(int value) {
    int backlight = value * 32 - 1;
    analogWrite(pins.lcd_backlight, backlight);
}

void PlatformCommon::startPreheat() {
    SETTINGS settings = getSettings();

    enableHeater(true);

    preheat_start = millis();
    preheat_end = preheat_start + (settings.preheatTime * 60000UL);
    in_preheat = true;
}

void PlatformCommon::set_audioLevel(int level) {
    sndLevel = level;
}

const PlatformPins &PlatformCommon::getPins() const {
    return pins;
}
