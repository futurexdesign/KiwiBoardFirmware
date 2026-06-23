/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "motorControl.h"
#include "picoPlatform.h"
#include "KiwiBoardFirmware_menu.h"
#include "kiwiDebugLog.h"

void MotorControl::initMotionController(PicoPlatform *curPlatform, uint16_t globalScaler, uint16_t iRun, bool stealthChop) {
    this->platform = curPlatform;

    if (motor == nullptr) {
        motor = createMotorDriver();
    }

    // Configure the active driver using menu values.
    MOTOR_DRIVER_CONFIG config;
    config.globalScaler = globalScaler;
    config.iRun = iRun;
    config.stealthChop = stealthChop;

    motor->init(platform, config);
    last_exec_ms = millis();
}

/**
 * Program ID
 *  0 : wash cycle (agitate)
 *  1 : spin-off (spin for short time to get excess solution off
 *  2 : dry  (spin for a time with the heater active
 *
 * @param programId
 * @param currentSettings
 */
void MotorControl::startProgram(int programId, SETTINGS currentSettings) {

    // A start has been requested
    state.program = programId;
    state.run_start = millis();
    state.isStopping = false;
    LOG_INFO("Program:", state.program);

    if (programId == 0) {
        // wash cycle..
        state.run_end = state.run_start + (currentSettings.wash_duration * 60000);
        state.isRunning = true;


        platform->enableMotor(true);

        // Reset Motion Controller start position to 0.
        motor->setCurrentPosition(0, true);

        // ramp definition
        motor->setRampMode(RAMP_POSITIONING);

        // Set speed in full steps per second.
        motor->setMaxSpeed(rpmToVmax(currentSettings.wash_speed));
        state.rpm = currentSettings.wash_speed;

        motor->setAcceleration(currentSettings.wash_amax);

        // Rotate the configured number of full steps
        // 200 full steps per rotation
        state.washSteps = currentSettings.washRotations * 200;
        motor->setTargetPosition(state.washSteps);

        state.direction = false;
    } else if (programId == 1) {
        // spin-off..
        // Spin is configured in seconds... not minutes...
        state.run_end = state.run_start + (currentSettings.spin_duration * 1000);
        state.isRunning = true;

        // Enable drive, active low
        platform->enableMotor(true);
        motor->setCurrentPosition(0, true); // Reset position
        motor->setTargetPosition(0);

        // Set motor control to velocity mode, setup accelerations, max desired speed
        motor->setRampMode(RAMP_VELOCITY);

        motor->setAcceleration(currentSettings.spin_amax);

        motor->setMaxSpeed(rpmToVmax(currentSettings.spin_speed));     // Full steps per second
        state.rpm = currentSettings.spin_speed;

        state.direction = true;

    } else if (programId == 9) {
        // test motor ramping..
        state.isTesting = true;
        state.isRunning = true;

        // Enable drive, active low
        platform->enableMotor(true);

        // Set motor control to velocity mode, setup accelerations, max desired speed
        // Set low accel, high rpm, check back to see if reached speed - yes? then decellerate to zero
        motor->setRampMode(RAMP_VELOCITY);
        motor->setAcceleration(100); // 300 microsteps per unit of time
        motor->setMaxSpeed(rpmToVmax(400)); // RPM 600

        state.direction = true;

    } else {
        // dry... which is basically a spin-off with heat
        // spin-off..
        state.run_end = state.run_start + (currentSettings.dry_duration * 60000);
        state.isRunning = true;

        // Enable drive, active low
        platform->enableMotor(true);
        motor->setCurrentPosition(0, true);

        // Enable heat and fan
        platform->enableHeater(true);

        // Set motor control to velocity mode, setup accelerations, max desired speed
        motor->setRampMode(RAMP_VELOCITY);
        motor->setAcceleration(300);
        motor->setMaxSpeed(rpmToVmax(currentSettings.dry_speed));
        state.rpm = currentSettings.dry_speed;

        state.direction = true;
    }
}

void MotorControl::exec() {
    const unsigned long now = millis();
    const unsigned long dtMs = now - last_exec_ms;
    last_exec_ms = now;

    motor->tick(dtMs);

    // This is our tick for motor control, Check to see if we are running, need to change directions, or stop

    if (!state.isRunning) {
        return;
    }

    // Check to see if we are stopping
    if (state.isStopping) {
        // Check to see if motion terminated, so we can disable the motion controller.
        if (motor->getCurrentSpeed() == 0) {
            state.isRunning = false;
            state.isStopping = false;
            motor->setCurrentPosition(0, true); // Reset position
            motor->setTargetPosition(0); // Reset position

            platform->enableMotor(false);

            if (stoppedCallback != nullptr) {
                stoppedCallback(state.program);
            }
        } else {
            // Give it 50 ticks to finish before stopping it manually
            if (!state.isTesting && state.stopping_cnt == 50) {
                motor->setCurrentPosition(0, true); // Reset position
                motor->setTargetPosition(0); // Reset position

                platform->enableMotor(false);
                state.isStopping = false;
                state.isRunning = false;

                if (stoppedCallback != nullptr) {
                    stoppedCallback(state.program);
                }
            } else {
                state.stopping_cnt++;
            }
        }

        if (state.program == 2) {
            // trigger cooldown
            platform->startCooldown();
        }
    }

    // If past end time...
    if (!state.isTesting && millis() >= state.run_end) {

        stopMotion();

    } else if (state.program == 0) {
        LOG_DEBUG("Check motion");
        LOG_DEBUG("Target Pos:", motor->getTargetPosition());
        LOG_DEBUG("Current Position:", motor->getCurrentPosition());

        // check if we need to change directions yet
        if (motor->isTargetPositionReached()) {
            LOG_INFO("Reverse");
            // reverse directions
            state.direction = !state.direction;
            motor->setTargetPosition(state.direction ? 0 : state.washSteps);
        }
    } else if (state.isTesting) {
            LOG_DEBUG("Motor testing......");
            LOG_DEBUG("Current Speed:", motor->getCurrentSpeed());
            // If reached target velocity...
            if(motor->isTargetVelocityReached() && motor->getCurrentSpeed() > 0) {
                motor->setMaxSpeed(rpmToVmax(0));
            }
            // If Stopped...
            if(motor->isTargetVelocityReached() && motor->getCurrentSpeed() == 0) {
                menumotorTest.setBoolean(false, true);
                state.isRunning = false;
                state.isTesting = false;
                platform->enableMotor(false);
                if (stoppedCallback != nullptr) {
                    stoppedCallback(state.program);
                }
            }
        }
}

int MotorControl::getRunningProgram() {
    return state.program;
}

bool MotorControl::isRunning() {
    return state.isRunning;
}

void MotorControl::stopMotion() {

    if (state.isRunning) {
        if (getRunningProgram() != 0) {
            // if we are in a wash cycle... let the current motion finish, otherwise, set speed to zero
            // and let the motion controller ramp down.
            motor->setMaxSpeed(0);
        }

        state.isStopping = true;
        state.stopping_cnt = 0;
    }
}

unsigned long MotorControl::getSecondsRemaining() {

    unsigned long rtn = 0;
    if (state.isRunning) {
            if(state.run_end > millis()) // Need to stop 'negative' result 
                rtn = ((state.run_end - millis()) / 1000);
            else
                rtn = 0;
    }

    return rtn;
}

/**
 * Return the current DriverStatus from the underlying motor control
 */
DriverStatus MotorControl::getDriverStatus() {
    return motor->getDriverStatus();
}

void MotorControl::setStoppedCallback(MotorCallbackFn stopFn) {

    stoppedCallback = stopFn;
}

/**
* Convert the provided RPM to full steps per second.
*
* @param rpm
* @return
*/
float MotorControl::rpmToVmax(float rpm) {

    // Fullstep rotation per second..
    float vmax = (rpm / 60.0f) * 200;

    return vmax;
}

void MotorControl::setStealthChop(bool enabled) {
    if (motor != nullptr) {
        motor->setStealthChop(enabled);
    }
}

bool MotorControl::isStealthChopEnabled() {
    if (motor == nullptr) {
        return false;
    }
    return motor->isStealthChopEnabled();
}

void MotorControl::printDriveStatus() {
    if (motor == nullptr) {
        Serial.println(F("Motor control not initialized"));
        return;
    }

    Serial.print(F("motor enabled: "));
    Serial.println(platform->isMotorEnabled() ? F("true") : F("false"));
    motor->printDriveStatus();
}


int MotorControl::getMotorSpeed() {
    return state.rpm;
}

void MotorControl::overrideMotorSpeed(int speedRPM) {

    state.rpm = speedRPM;

    // reconfigure VMAX? This might cause problems in wash cycle..

    motor->setMaxSpeed(rpmToVmax(speedRPM));

}
