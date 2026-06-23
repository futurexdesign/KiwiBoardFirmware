#pragma once

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * Common motor driver interface used by MotorControl.  Implementations wrap the TMC5160
 * motion controller (hardware ramp generator) and the TMC2209 driver (VACTUAL pulse generator
 * with software motion planner).
 */

#include <stdint.h>

class PicoPlatform;

enum DriverStatus {
    DRIVER_OK = 0,
    DRIVER_OT,
    DRIVER_OTPW,
    DRIVER_S2VSA,
    DRIVER_S2VSB,
    DRIVER_S2GA,
    DRIVER_S2GB,
    DRIVER_CP_UV,
    DRIVER_UNKNOWN
};

enum RampMode {
    RAMP_POSITIONING,
    RAMP_VELOCITY
};

/**
 * Configuration values passed from the menu/settings layer into the active motor driver.
 */
struct MOTOR_DRIVER_CONFIG
{
    uint16_t globalScaler;
    uint16_t iRun;
    bool stealthChop;
};

class MotorDriver {
    public:
        virtual ~MotorDriver() = default;

        /**
         * Initialize the driver on the provided platform using menu-configured values.
         */
        virtual bool init(PicoPlatform *curPlatform, const MOTOR_DRIVER_CONFIG &config) = 0;

        /**
         * Stop all motion immediately.
         */
        virtual void stop() = 0;

        virtual void setRampMode(RampMode mode) = 0;
        /**
         * Set ramp acceleration in full steps per second squared (same units as TMC5160 library).
         */
        virtual void setAcceleration(uint32_t accelStepsPerSec2) = 0;

        /**
         * Set the target velocity in full steps per second.
         */
        virtual void setMaxSpeed(float vmax) = 0;

        virtual void setCurrentPosition(int32_t position, bool reset) = 0;
        virtual void setTargetPosition(int32_t position) = 0;
        virtual int32_t getCurrentPosition() = 0;
        virtual int32_t getTargetPosition() = 0;

        /**
         * Return the current velocity in full steps per second.
         */
        virtual float getCurrentSpeed() = 0;
        virtual bool isTargetVelocityReached() = 0;
        virtual bool isTargetPositionReached() = 0;

        /**
         * Advance motion state.  Called from the MotorControl task loop.
         */
        virtual void tick(unsigned long dtMs) = 0;

        virtual DriverStatus getDriverStatus() = 0;

        /**
         * Enable or disable StealthChop at runtime (does not persist to EEPROM).
         */
        virtual void setStealthChop(bool enabled) = 0;
        virtual bool isStealthChopEnabled() = 0;

        /**
         * Print driver-specific diagnostic information to Serial.
         */
        virtual void printDriveStatus() = 0;
};

MotorDriver *createMotorDriver();
