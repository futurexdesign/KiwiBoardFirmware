/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "softwareMotionPlanner.h"
#include <Arduino.h>
#include <math.h>

static float signum(float value) {
    if (value > 0.0f) {
        return 1.0f;
    }
    if (value < 0.0f) {
        return -1.0f;
    }
    return 0.0f;
}

void SoftwareMotionPlanner::reset() {
    ramp_mode = RAMP_VELOCITY;
    phase = PHASE_IDLE;
    accel = 100.0f;
    current_speed = 0.0f;
    target_speed = 0.0f;
    cruise_speed = 0.0f;
    position = 0.0f;
    target_position = 0;
    is_stopping = false;
}

void SoftwareMotionPlanner::setPosition(int32_t newPosition) {
    position = (float)newPosition;
}

// Match TMC5160 library setAcceleration() units: full steps per second squared.
// Menu/CLI accel values are passed through directly (not AMAX register values).
void SoftwareMotionPlanner::setAcceleration(uint32_t accelStepsPerSec2) {
    accel = (float)accelStepsPerSec2;
    if (accel < 1.0f) {
        accel = 1.0f;
    }
}

void SoftwareMotionPlanner::setCruiseSpeed(float vmax) {
    cruise_speed = vmax;
}

void SoftwareMotionPlanner::setVelocityMode(float targetSpeed) {
    ramp_mode = RAMP_VELOCITY;
    is_stopping = false;
    cruise_speed = targetSpeed;
    target_speed = targetSpeed;
    phase = (fabsf(targetSpeed) < 0.01f) ? PHASE_DECEL : PHASE_ACCEL;
}

void SoftwareMotionPlanner::setPositionMode(int32_t newTargetPosition) {
    ramp_mode = RAMP_POSITIONING;
    is_stopping = false;
    target_position = newTargetPosition;

    const float distance = (float)newTargetPosition - position;
    if (fabsf(distance) < 0.5f) {
        current_speed = 0.0f;
        target_speed = 0.0f;
        phase = PHASE_IDLE;
        position = (float)newTargetPosition;
        return;
    }

    if (fabsf(cruise_speed) < 0.01f) {
        cruise_speed = signum(distance) * 50.0f;
    } else {
        cruise_speed = signum(distance) * fabsf(cruise_speed);
    }

    // Short moves cannot reach the configured cruise speed; cap to a triangular profile.
    const float maxSpeedForDistance = sqrtf(accel * fabsf(distance));
    if (fabsf(cruise_speed) > maxSpeedForDistance) {
        cruise_speed = signum(distance) * maxSpeedForDistance;
    }

    const float direction = signum(distance);
    const float absCruise = fabsf(cruise_speed);
    target_speed = direction * absCruise;

    // Preserve momentum when retargeting mid-move (wash reversal); updateVelocity refines each tick.
    const float stopDistance = (current_speed * current_speed) / (2.0f * accel);
    if (fabsf(distance) <= stopDistance + 0.5f) {
        phase = PHASE_DECEL;
        target_speed = 0.0f;
    } else if (signum(current_speed) != direction && fabsf(current_speed) > 0.01f) {
        phase = PHASE_DECEL;
    } else if (fabsf(current_speed) < absCruise - 0.01f) {
        phase = PHASE_ACCEL;
    } else {
        phase = PHASE_CRUISE;
    }
}

void SoftwareMotionPlanner::requestStop() {
    is_stopping = true;
    target_speed = 0.0f;
    phase = PHASE_DECEL;
}

void SoftwareMotionPlanner::updateVelocity(float dtSec) {
    if (phase == PHASE_IDLE) {
        return;
    }

    if (ramp_mode == RAMP_POSITIONING && !is_stopping) {
        const float remaining = (float)target_position - position;
        const float direction = signum(remaining);
        const float stopDistance = (current_speed * current_speed) / (2.0f * accel);

        if (fabsf(remaining) <= stopDistance + 0.5f) {
            target_speed = 0.0f;
            phase = PHASE_DECEL;
        } else if (fabsf(current_speed) < fabsf(cruise_speed)) {
            target_speed = direction * fabsf(cruise_speed);
            phase = PHASE_ACCEL;
        } else {
            target_speed = direction * fabsf(cruise_speed);
            phase = PHASE_CRUISE;
        }
    }

    const float delta = target_speed - current_speed;
    const float maxDelta = accel * dtSec;

    if (fabsf(delta) <= maxDelta) {
        current_speed = target_speed;
    } else {
        current_speed += signum(delta) * maxDelta;
    }

    if (phase == PHASE_DECEL && fabsf(current_speed) < 0.01f) {
        current_speed = 0.0f;
        phase = PHASE_IDLE;
    } else if (phase == PHASE_ACCEL && fabsf(current_speed - target_speed) < 0.01f) {
        phase = (target_speed == 0.0f) ? PHASE_IDLE : PHASE_CRUISE;
    }
}

void SoftwareMotionPlanner::tick(unsigned long dtMs) {
    if (dtMs == 0) {
        return;
    }

    const float dtSec = (float)dtMs / 1000.0f;
    updateVelocity(dtSec);

    if (fabsf(current_speed) > 0.0f) {
        const float delta = current_speed * dtSec;
        const float remaining = (float)target_position - position;
        const bool movingTowardTarget =
            (remaining > 0.0f && current_speed > 0.0f) ||
            (remaining < 0.0f && current_speed < 0.0f);

        if (movingTowardTarget &&
            ((remaining > 0.0f && delta > remaining) || (remaining < 0.0f && delta < remaining))) {
            position = (float)target_position;
        } else {
            position += delta;
        }
    }

    if (ramp_mode == RAMP_POSITIONING && !is_stopping) {
        const bool reachedForward = (cruise_speed >= 0.0f && position >= (float)target_position);
        const bool reachedReverse = (cruise_speed < 0.0f && position <= (float)target_position);
        if ((reachedForward || reachedReverse) && phase == PHASE_IDLE) {
            position = (float)target_position;
        }
    }
}

float SoftwareMotionPlanner::getCurrentSpeed() const {
    return current_speed;
}

int32_t SoftwareMotionPlanner::getCurrentPosition() const {
    return (int32_t)lroundf(position);
}

int32_t SoftwareMotionPlanner::getTargetPosition() const {
    return target_position;
}

bool SoftwareMotionPlanner::isTargetVelocityReached() const {
    return fabsf(current_speed - target_speed) < 0.5f;
}

bool SoftwareMotionPlanner::isTargetPositionReached() const {
    const float remaining = (float)target_position - position;

    if (fabsf(remaining) < 0.5f) {
        // Fully stopped at target, or still moving through the endpoint (seamless wash reversal).
        return (phase == PHASE_IDLE && fabsf(current_speed) < 0.01f) ||
               fabsf(current_speed) > 0.01f;
    }

    if (fabsf(current_speed) < 0.01f) {
        return false;
    }

    // Passed target while moving (5160-style positioning).
    if (cruise_speed >= 0.0f && remaining < 0.0f) {
        return true;
    }
    if (cruise_speed < 0.0f && remaining > 0.0f) {
        return true;
    }

    return false;
}

bool SoftwareMotionPlanner::isStopped() const {
    return phase == PHASE_IDLE && fabsf(current_speed) < 0.01f;
}
