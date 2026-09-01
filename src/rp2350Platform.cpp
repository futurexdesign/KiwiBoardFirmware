/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "rp2350Platform.h"
#include "platformFactory.h"
#include "motorDriver.h"
#include "tmc2209Driver.h"

RP2350Platform::RP2350Platform() : PlatformCommon(PINS) {}

void RP2350Platform::initializeHardware() {
    // TMC2209 UART uses expansion1/expansion2; configured in TMC2209Driver::init.
}

void RP2350Platform::enableSounder(bool activate) {
    (void)activate;
}

MotorDriver *RP2350Platform::createMotorDriver() {
    return new TMC2209Driver();
}

void RP2350Platform::pollScreenCapture() {}

Platform *createPlatform() {
    return new RP2350Platform();
}
