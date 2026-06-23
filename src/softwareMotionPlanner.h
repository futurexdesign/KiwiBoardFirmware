#pragma once

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * Software trapezoidal motion planner used by the TMC2209 driver.  Replaces the TMC5160
 * hardware ramp generator for velocity and positioning moves.
 */

#include <stdint.h>
#include "motorDriver.h"

class SoftwareMotionPlanner {
    public:
        void reset();
        void setPosition(int32_t position);
        void setAcceleration(uint32_t amax);
        void setCruiseSpeed(float vmax);
        void setVelocityMode(float targetSpeed);
        void setPositionMode(int32_t targetPosition);
        void requestStop();

        /**
         * Advance the planner by the provided interval in milliseconds.
         */
        void tick(unsigned long dtMs);

        float getCurrentSpeed() const;
        int32_t getCurrentPosition() const;
        int32_t getTargetPosition() const;
        bool isTargetVelocityReached() const;
        bool isTargetPositionReached() const;
        bool isStopped() const;

    private:
        enum PlannerPhase {
            PHASE_IDLE,
            PHASE_ACCEL,
            PHASE_CRUISE,
            PHASE_DECEL
        };

        void updateVelocity(float dtSec);

        RampMode ramp_mode = RAMP_VELOCITY;
        PlannerPhase phase = PHASE_IDLE;

        float accel = 100.0f;
        float current_speed = 0.0f;
        float target_speed = 0.0f;
        float cruise_speed = 0.0f;

        float position = 0.0f;
        int32_t target_position = 0;
        bool is_stopping = false;
};
