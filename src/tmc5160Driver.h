#pragma once

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * MotorDriver implementation for the TMC5160 motion controller over SPI.
 */

#include "motorDriver.h"
#include <TMC5160.h>

class TMC5160Driver : public MotorDriver {

    public:
        ~TMC5160Driver() override;

        bool init(Platform *curPlatform, const MOTOR_DRIVER_CONFIG &config) override;
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
        static DriverStatus mapStatus(TMC5160::DriverStatus status);

        TMC5160_SPI *motor = nullptr;
        Platform *platform = nullptr;
        bool stealth_chop = true;
};
