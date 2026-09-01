#pragma once

#include "platform.h"

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

class PlatformCommon : public Platform {
    public:
        explicit PlatformCommon(const PlatformPins &pins);

        void initializePlatform() override;

        void enableHeater(bool activate) override;
        void enableFan(bool activate) override;
        void enableMotor(bool activate) override;

        void set_audioLevel(int level) override;

        void startCooldown() override;
        void startPreheat() override;

        void exec() override;

        void setBacklight(int value) override;

        bool isMotorEnabled() override;
        bool isHeaterEnabled() override;
        bool isFanEnabled() override;

        const PlatformPins &getPins() const override;

    protected:
        const PlatformPins &pins;

        virtual void initializeHardware() = 0;
        virtual void onExecTick() {}

        int sndLevel = 0;

        bool in_preheat = false;
        bool in_cooldown = false;
        bool motor_enabled = false;
        bool heater_enabled = false;
        bool fan_enabled = false;
        bool led = false;

        unsigned long cooldown_start = 0;
        unsigned long cooldown_end = 0;
        unsigned long preheat_start = 0;
        unsigned long preheat_end = 0;
};
