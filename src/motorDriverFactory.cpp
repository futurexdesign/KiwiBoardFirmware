/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "motorDriver.h"

#if defined(MOTOR_DRIVER_TMC2209)
#include "tmc2209Driver.h"
#else
#include "tmc5160Driver.h"
#endif

/**
 * Create the motor driver implementation selected at compile time.
 */
MotorDriver *createMotorDriver() {
#if defined(MOTOR_DRIVER_TMC2209)
    return new TMC2209Driver();
#else
    return new TMC5160Driver();
#endif
}
