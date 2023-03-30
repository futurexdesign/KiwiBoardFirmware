/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include "picoPlatform.h"
#include "settings.h"

/**
 * Initialize all IO on the Pico to the correct pins 
*/
void PicoPlatform::init_platform() {

    // Set input pins.
    pinMode(ENC1, INPUT);
    pinMode(ENC2, INPUT);
    pinMode(BUTTON, INPUT);

    // Set output pins.   Adjust current limit if needed
    pinMode(MOTOR_EN, OUTPUT_8MA);
    digitalWrite(MOTOR_EN, HIGH); //active low 

    pinMode(HEATER_CTL, OUTPUT_8MA);
    digitalWrite(HEATER_CTL, LOW);

    pinMode(FAN_CTL, OUTPUT_8MA);
    digitalWrite(FAN_CTL, LOW);

    // Remap  IO to the correct pins for i2c0
    Wire.setSDA(OLED_SDA);
    Wire.setSCL(OLED_SCL);

    Wire.begin();

    // Remap IO to the correct pins for hardware SPI 0 
    SPI.setRX(TMC_MISO);
    SPI.setTX(TMC_MOSI);
    SPI.setSCK(TMC_SCLK);
    SPI.setCS(TMC_SS);

    // start SPI with hardware chip select disabled (TMC library handles this). 
    SPI.begin(false);

    EEPROM.begin(512);
}

/**
 * Enable or disable the heater output based on provided value. 
 * The heater is enabled on logic HIGH.
*/
void PicoPlatform::heater_enable(bool activate) {

    digitalWrite(HEATER_CTL, activate);
    heater_enabled = true;

    // If heater has been turned on, the fan MUST turn on.  
    // If we are turning the heater off, the cooldown logic will turn off the fan 
    if (activate) {
        fan_enable(true);
    }
}

/**
 * Enable or disable the fan output based on provided value. 
 * The fan should always run if the heater is enabled. 
 * The fan is enabled on logic HIGH.
*/
void PicoPlatform::fan_enable(bool activate) {

    digitalWrite(FAN_CTL, activate);
    fan_enabled = activate;

}

/**
 * Enable or disable the stepper motor controller.  This is attached directly 
 * to the EN pin of the stepper board.  This is active LOW.  
*/
void PicoPlatform::motor_enable(bool activate) {
    digitalWrite(MOTOR_EN, !activate); // active low
    motor_enabled = activate;
}

bool PicoPlatform::is_fan_enabled() {
    return fan_enabled;
}

bool PicoPlatform::is_heater_enabled() {
    return heater_enabled;
}

bool PicoPlatform::is_motor_enabled() {
    return motor_enabled;
}

void PicoPlatform::exec() {

    // Check heater and fan correlation.. this could probably just be a PIO
    if (heater_enabled && !fan_enabled) {
        // This honestly should never happen...
        fan_enable(true);
    }

    if (in_cooldown) {
        if (millis() >= cooldown_end) {
            fan_enable(false);
        }
    }
}

void PicoPlatform::start_cooldown() {

    SETTINGS settings = getSettings();

    // turn off the heat
    heater_enable(false);

    if (settings.fanCooldown) {
        // cooldown enabled, calculate the fan end time, mark start time.  ::tick will handle turning fan off
        cooldown_start = millis();
        cooldown_end = cooldown_start + (settings.cooldownTime * 60000);
        in_cooldown = true;

    } else {
        // shut off fan
        fan_enable(false);
    }
}
