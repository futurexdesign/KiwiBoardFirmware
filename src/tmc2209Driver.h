#pragma once

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * MotorDriver implementation for the TMC2209 stepper driver over UART.  Motion is driven
 * via the internal VACTUAL pulse generator and the SoftwareMotionPlanner.
 */

#include "motorDriver.h"
#include "softwareMotionPlanner.h"
#include <TMCStepper.h>

#define TMC2209_RSENSE 0.11f
#define TMC2209_DRIVER_ADDRESS 0b00
#define TMC2209_MICROSTEPS 256

class TMC2209Driver : public MotorDriver {

    public:
        ~TMC2209Driver() override;

        bool init(PicoPlatform *curPlatform, const MOTOR_DRIVER_CONFIG &config) override;
        void stop() override;

        void setRampMode(RampMode mode) override;
        void setAcceleration(uint32_t amax) override;
        void setMaxSpeed(float vmax) override;

        void setCurrentPosition(int32_t position, bool reset) override;
        void setTargetPosition(int32_t position) override;
        int32_t getCurrentPosition() override;
        int32_t getTargetPosition() override;

        float getCurrentSpeed() override;
        bool isTargetVelocityReached() override;
        bool isTargetPositionReached() override;

        void tick(unsigned long dtMs) override;

        DriverStatus getDriverStatus() override;

        void setStealthChop(bool enabled) override;
        bool isStealthChopEnabled() override;
        void printDriveStatus() override;

    private:
        void applyStealthChop(bool enabled);
        void applyVelocity(float vmax);
        int32_t vmaxToVactual(float vmax) const;

        TMC2209Stepper *motor = nullptr;
        PicoPlatform *platform = nullptr;
        SoftwareMotionPlanner planner;
        RampMode ramp_mode = RAMP_VELOCITY;
        int32_t target_position = 0;
        int32_t last_vactual = 0;
        unsigned long last_tick_ms = 0;
        bool stealth_chop = true;
};
