/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/
#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>
#include "picoPlatform.h"
#include "settings.h"

/**
 * Initialize all IO on the Pico to the correct pins 
*/
void PicoPlatform::initializePlatform() {

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
    // Turn on the LED
//    pinMode(LED_BUILTIN, OUTPUT);
//    digitalWrite(LED_BUILTIN, HIGH);

    // Remap IO to the correct pins for hardware SPI 0 
    SPI1.setRX(TMC_MISO);
    SPI1.setTX(TMC_MOSI);
    SPI1.setSCK(TMC_SCLK);
    SPI1.setCS(TMC_SS);

    // start SPI with hardware chip select disabled (TMC library handles this). 
    SPI1.begin(false);

    pinMode(LCD_BACKLIGHT, OUTPUT);
    analogWrite(LCD_BACKLIGHT, 125);
    //Setup SPI0 for the TFT
    SPI.setCS(LCD_CS);
    SPI.setRX(LCD_MISO);
    SPI.setTX(LCD_MOSI);
    SPI.setSCK(LCD_SCK);
    SPI.begin(false);


    // screenshot button, active high
    pinMode(EXPANSION1, INPUT_PULLDOWN);

    EEPROM.begin(512);
}

/**
 * Enable or disable the heater output based on provided value. 
 * The heater is enabled on logic HIGH.
*/
void PicoPlatform::enableHeater(bool activate) {

    digitalWrite(HEATER_CTL, activate);
    heater_enabled = true;

    // If heater has been turned on, the fan MUST turn on.  
    // If we are turning the heater off, the cooldown logic will turn off the fan 
    if (activate) {
        enableFan(true);
    }
}

/**
 * Enable or disable the fan output based on provided value. 
 * The fan should always run if the heater is enabled. 
 * The fan is enabled on logic HIGH.
*/
void PicoPlatform::enableFan(bool activate) {

    digitalWrite(FAN_CTL, activate);
    fan_enabled = activate;

}

/**
 * Enable or disable the stepper motor controller.  This is attached directly 
 * to the EN pin of the stepper board.  This is active LOW.  
*/
void PicoPlatform::enableMotor(bool activate) {
    digitalWrite(MOTOR_EN, !activate); // active low
    motor_enabled = activate;
}

bool PicoPlatform::isFanEnabled() {
    return fan_enabled;
}

bool PicoPlatform::isHeaterEnabled() {
    return heater_enabled;
}

bool PicoPlatform::isMotorEnabled() {
    return motor_enabled;
}

void PicoPlatform::exec() {

    // Check heater and fan correlation.. this could probably just be a PIO
    if (heater_enabled && !fan_enabled) {
        // This honestly should never happen...
        enableFan(true);
    }

    if (in_cooldown) {
        if (millis() >= cooldown_end) {
            enableFan(false);
            in_cooldown = false;
        }
    }
}

void PicoPlatform::startCooldown() {

    SETTINGS settings = getSettings();

    // turn off the heat
    enableHeater(false);

    if (settings.fanCooldown) {
        // cooldown enabled, calculate the fan end time, mark start time.  ::tick will handle turning fan off
        cooldown_start = millis();
        cooldown_end = cooldown_start + (settings.cooldownTime * 60000);
        in_cooldown = true;

    } else {
        // shut off fan
        enableFan(false);
    }
}

void PicoPlatform::setBacklight(int value) {

    // Convert the 1-8 to 0-254
    int backlight = value * 32 - 1; // 1 = 31 ::  8 = 255;
    analogWrite(LCD_BACKLIGHT, backlight);

}
