/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "motorDriver.h"
#include "picoPlatform.h"
#include "kiwiDebugLog.h"
#include <Arduino.h>

#if defined(MOTOR_DRIVER_AUTODETECT) || defined(MOTOR_DRIVER_TMC2209)
#include "tmc2209Driver.h"
#include "tmcHalfDuplexUart.h"
#include <TMCStepper.h>
#endif

#if defined(MOTOR_DRIVER_AUTODETECT) || !defined(MOTOR_DRIVER_TMC2209)
#include "tmc5160Driver.h"
#include <TMC5160.h>
#endif

static DetectedMotorDriver g_detectedDriver = DETECTED_NONE;

DetectedMotorDriver getDetectedMotorDriver() {
    return g_detectedDriver;
}

#if defined(MOTOR_DRIVER_AUTODETECT)

#include <SPI.h>
#include <hardware/gpio.h>

// Hold SPI data/clock lines low while talking to a TMC2209 on TMC_SS.
// Floating/SPI-mode GPIO10-12 can upset PDN_UART on boards that share that connector.
static void parkSpiBusPinsLow() {
    SPI1.end();

    pinMode(TMC_SCLK, OUTPUT);
    digitalWrite(TMC_SCLK, LOW);
    pinMode(TMC_MOSI, OUTPUT);
    digitalWrite(TMC_MOSI, LOW);
    pinMode(TMC_MISO, OUTPUT);
    digitalWrite(TMC_MISO, LOW);
}

static void restoreSpiBusPins() {
    SPI1.setRX(TMC_MISO);
    SPI1.setTX(TMC_MOSI);
    SPI1.setSCK(TMC_SCLK);
    SPI1.setCS(TMC_SS);
    SPI1.begin(false);

    pinMode(TMC_SS, OUTPUT);
    digitalWrite(TMC_SS, HIGH);
}

static bool probeTmc2209(PicoPlatform *platform) {
    platform->enableMotor(true);
    delay(100);

    Serial.println(F("Parking SPI bus pins (GPIO10-12) low for TMC2209 probe"));
    parkSpiBusPinsLow();

    tmcHalfDuplexBegin(115200);
    delay(100);

    Serial.print(F("UART RX/TX half-duplex on GPIO"));
    Serial.print(TMC_UART_RX);
    Serial.print(F(" func="));
    Serial.println((int)gpio_get_function(TMC_UART_RX));

    TMC2209Stepper probe(&tmcHalfDuplexStream(), TMC2209_RSENSE, TMC2209_DRIVER_ADDRESS);
    probe.begin();
    probe.pdn_disable(true);
    probe.mstep_reg_select(true);
    // Stretch driver reply turnaround; FIFO stays armed (no pin remux).
    probe.senddelay(15);
    delay(20);

    const uint8_t version = probe.version();
    Serial.print(F("TMC2209 probe version: 0x"));
    Serial.println(version, HEX);

    tmcHalfDuplexEnd();
    platform->enableMotor(false);

    if (version == 0x21) {
        parkSpiBusPinsLow();
        return true;
    }

    restoreSpiBusPins();
    return false;
}

static bool probeTmc5160(PicoPlatform *platform) {
    restoreSpiBusPins();

    platform->enableMotor(true);
    delay(50);

    TMC5160_SPI probe(TMC_SS, TMC5160::DEFAULT_F_CLK, SPISettings(1000000, MSBFIRST, SPI_MODE0), SPI1);
    TMC5160_Reg::IOIN_Register ioin = {0};
    ioin.value = probe.readRegister(TMC5160_Reg::IO_INPUT_OUTPUT);

    Serial.print(F("TMC5160 probe version: 0x"));
    Serial.println(ioin.version, HEX);

    platform->enableMotor(false);
    return ioin.version == TMC5160::IC_VERSION;
}

/**
 * Create the motor driver selected by boot-time probing.
 * Probe TMC2209 UART (half-duplex on TMC_SS) first; fall back to TMC5160 SPI.
 */
MotorDriver *createMotorDriver(PicoPlatform *platform) {
    Serial.println(F("Motor driver autodetection..."));
    Serial.print(F("Probing TMC2209 half-duplex UART on GPIO"));
    Serial.print(TMC_UART_RX);
    Serial.println(F(" (TMC_SS) — expansion port not used"));

    if (probeTmc2209(platform)) {
        g_detectedDriver = DETECTED_TMC2209;
        Serial.print(F("Detected TMC2209 (half-duplex GPIO"));
        Serial.print(TMC_UART_RX);
        Serial.println(')');
        return new TMC2209Driver();
    }

    Serial.println(F("No TMC2209 response; probing TMC5160 over SPI..."));

    if (probeTmc5160(platform)) {
        g_detectedDriver = DETECTED_TMC5160;
        Serial.println(F("Detected TMC5160 (SPI)"));
        return new TMC5160Driver();
    }

    g_detectedDriver = DETECTED_NONE;
    Serial.println(F("ERROR: no motor driver responded"));
    return nullptr;
}

#elif defined(MOTOR_DRIVER_TMC2209)

MotorDriver *createMotorDriver(PicoPlatform *platform) {
    (void)platform;
    g_detectedDriver = DETECTED_TMC2209;
    return new TMC2209Driver();
}

#else

MotorDriver *createMotorDriver(PicoPlatform *platform) {
    (void)platform;
    g_detectedDriver = DETECTED_TMC5160;
    return new TMC5160Driver();
}

#endif
