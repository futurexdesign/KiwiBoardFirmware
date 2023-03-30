/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/

#include "motorControl.h"
#include "picoPlatform.h"

void MotorControl::init_tmc(PicoPlatform* curPlatform)
{
    this->platform = curPlatform; // Set the current platform.

    // start by disabling motor controller. 
    platform->motor_enable(false);

    TMC5160::PowerStageParameters powerStageParams; // defaults.
    TMC5160::MotorParameters motorParams;
    motorParams.globalScaler = 98; // Adapt to your driver and motor (check TMC5160 datasheet - "Selecting sense resistors")
    motorParams.irun = 16; //31 is max running current... so cutting in half brought things down to <5w 
    motorParams.ihold = 0; // holding current, 0 enables freewheel 

    // TODO Setup any stealth-chop settings we may want changed from defaults. 

    motor = new TMC5160_SPI(TMC_SS);

    motor->begin(powerStageParams, motorParams, TMC5160::NORMAL_MOTOR_DIRECTION);
    motor->stop(); // Ensure the controller is initialized with the motor stopped

    // TODO, add in coms check back in?
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
void MotorControl::start_program(int programId, SETTINGS currentSettings) {

    // A start has been requested
    state.program = programId;
    state.run_start = millis();

    if(programId == 0) {
        // wash cycle..
        state.run_end = state.run_start + (currentSettings.wash_duration * 60000);
        state.isRunning = true;

        // Initial prototype had agitate happening in velocity mode, which
        // has some downsides on the acceleration curves, and creates a floor on
        // how fast the cycle time can be (need enough time for it to actually accel/decel
        // should probably just switch back to positioning mode.

        platform->motor_enable(true);

        // ramp definition
        motor->setRampMode(TMC5160::POSITIONING_MODE);
        motor->setMaxSpeed(400);
        motor->setAcceleration(1000);

        // Make two rotations
        motor->setTargetPosition(400); //200 full steps per revolution
        state.direction = true;

    } else if (programId == 1) {
        // spin off..
        state.run_end = state.run_start + (currentSettings.spin_duration * 60000);
        state.isRunning = true;

        // Enable drive, active low
        platform->motor_enable(true);

        // Set motor control to velocity mode, setup accelerations, max desired speed
        motor->setRampMode(TMC5160::VELOCITY_MODE);
        motor->setAcceleration(1000); // TODO Unit?
        motor->setMaxSpeed(350); // Full steps per second

        state.direction = true;

    } else {
        // dry .. which is basically a spin off with heat
        // spin off..
        state.run_end = state.run_start + (currentSettings.dry_duration * 60000);
        state.isRunning = true;

        // Enable drive, active low
        platform->motor_enable(true);

        // Enable heat and fan
        platform->heater_enable(true);

        // Set motor control to velocity mode, setup accelerations, max desired speed
        motor->setRampMode(TMC5160::VELOCITY_MODE);
        motor->setAcceleration(1000); // TODO Unit?
        motor->setMaxSpeed(350); // Full steps per second


        state.direction = true;
    }

}

void MotorControl::exec() {
    // This is our tick for motor control, Check to see if we are running, need to change directions, or stop

    if (!state.isRunning) {
        return;
    }

    // If past end time...
    if (millis() >= state.run_end) {
        motor->setMaxSpeed(0);
        platform->motor_enable(false);
        if (state.program == 2) {
            // If we finished a dry cycle, trigger cooldown
            platform->start_cooldown();
        }
        state.isRunning = false;
    } else if (state.program == 0) {
        // check if we need to change directions yet
        if (motor->getTargetPosition() == motor->getCurrentPosition()) {
            // reverse directions
            state.direction = !state.direction;
            motor->setTargetPosition(state.direction ? 0: 400);
        }
    }
}

int MotorControl::get_running_program() {
    return state.program;
}

bool MotorControl::is_running() {
    return state.isRunning;
}

void MotorControl::stop_motion() {

    motor->setMaxSpeed(0);
    platform->motor_enable(false);
    // TODO.. we may need a delay between setting the speed to zero and disabling the motor en line
    // this may cause the motion to end too quickly, rather than using a decel ramp.
    // We likely just need a second timer that can identify that we stopped, and then disable the motors afterwards


}

