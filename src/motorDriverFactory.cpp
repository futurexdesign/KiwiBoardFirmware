/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "motorDriver.h"
#include "picoPlatform.h"
#include <Arduino.h>

#if defined(ARDUINO_GENERIC_RP2350)
#include "tmc2209Driver.h"
#else
#include "tmc5160Driver.h"
#endif

MotorDriver *createMotorDriver(PicoPlatform *platform) {
    (void)platform;
#if defined(ARDUINO_GENERIC_RP2350)
    return new TMC2209Driver();
#else
    return new TMC5160Driver();
#endif
}
