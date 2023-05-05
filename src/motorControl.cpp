/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "motorControl.h"
#include "picoPlatform.h"

bool motorTest = 0;

void MotorControl::initMotionController(PicoPlatform *curPlatform, uint16_t globalScaler, uint16_t iRun, uint16_t transition) {
    this->platform = curPlatform; // Set the current platform.

    // start motor controller. wants to be enabled to talk to it..
    platform->enableMotor(true);

    TMC5160::PowerStageParameters powerStageParams; // defaults.
    TMC5160::MotorParameters motorParams;

    motorParams.globalScaler = globalScaler; // Use Excel file to find correct scaler so that ihold=31 is max desired current
    motorParams.irun = iRun;                 // 31 is max running current, adjust accordingly.
    motorParams.ihold = 0;                   // holding current, 0 enables freewheel


    // Library initializes with StealthChop enabled.
    motor = new TMC5160_SPI(TMC_SS, TMC5160::DEFAULT_F_CLK, SPISettings(1000000, MSBFIRST, SPI_MODE0), SPI1);
    motor->begin(powerStageParams, motorParams, TMC5160::NORMAL_MOTOR_DIRECTION);

    // Disable the transition to SpreadCycle, we don't need accuracy, prefer StealthChop
   // motor->writeRegister(TMC5160_Reg::TPWMTHRS, 0);
    motor->writeRegister(TMC5160_Reg::TPWMTHRS, transition);



    motor->stop(); // Ensure the controller is initialized with the motor stopped

    // Check if the TMC5160 answers back
    TMC5160_Reg::IOIN_Register ioin = {0};

    // while (ioin.version != motor->IC_VERSION)
    // {
    //     ioin.value = motor->readRegister(TMC5160_Reg::IO_INPUT_OUTPUT);

    //     if (ioin.value == 0 || ioin.value == 0xFFFFFFFF)
    //     {
    //         Serial.println("No TMC5160 found.");
    //         delay(2000);
    //     }
    //     else
    //     {
    //         Serial.println("Found a TMC device.");
    //         Serial.print("IC version: 0x");
    //         Serial.print(ioin.version, HEX);
    //         Serial.print(" (");
    //         if (ioin.version == motor->IC_VERSION)
    //             Serial.println("TMC5160).");
    //         else
    //             Serial.println("unknown IC !)");
    //     }
    // }

    platform->enableMotor(false);
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
    Serial.print("Program: ");
    Serial.println(state.program);

    if (programId == 0) {
        // wash cycle..
        state.run_end = state.run_start + (currentSettings.wash_duration * 60000);
        state.isRunning = true;


        platform->enableMotor(true);

        // Reset Motion Controller start position to 0.
        motor->setCurrentPosition(0, true);

        // ramp definition
        motor->setRampMode(TMC5160::POSITIONING_MODE);

        // Set speed in full steps per second.
        motor->setMaxSpeed(rpmToVmax(currentSettings.wash_speed));

        // Register for VMAX is expressed in uSteps / t
        // t is 1.398101 at 12mhz

        // Given
        motor->setAcceleration(currentSettings.wash_amax);

        // Rotate the configured number of full steps
        // 200 full steps per rotation
        state.washSteps = currentSettings.washRotations * 200;
        // target position is set using FULL-STEPS (motor library multiplies by uSteps internally)..
        motor->setTargetPosition(state.washSteps);

        state.direction = false;
    } else if (programId == 1) {
        // spin-off..
        state.run_end = state.run_start + (currentSettings.spin_duration * 60000);
        state.isRunning = true;

        // Enable drive, active low
        platform->enableMotor(true);
        motor->setCurrentPosition(0, true); // Reset position
        motor->setTargetPosition(0);

        // Set motor control to velocity mode, setup accelerations, max desired speed
        motor->setRampMode(TMC5160::VELOCITY_MODE);

        motor->setAcceleration(currentSettings.spin_amax);

        motor->setMaxSpeed(rpmToVmax(currentSettings.spin_speed));     // Full steps per second

        state.direction = true;
        
    } else if (programId == 9) {
        // test motor ramping..
        motorTest = true;
        state.isRunning = true;
        
        // Enable drive, active low
        platform->enableMotor(true);
       
        // Set motor control to velocity mode, setup accelerations, max desired speed
        // Set low accel, high rpm, check back to see if reached speed - yes? then decellerate to zero
        motor->setRampMode(TMC5160::VELOCITY_MODE);
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
        motor->setRampMode(TMC5160::VELOCITY_MODE);
        motor->setAcceleration(300);
        motor->setMaxSpeed(rpmToVmax(currentSettings.dry_speed));

        state.direction = true;
    }
}

void MotorControl::exec() {
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
            motorTest = false;
            platform->enableMotor(false);

            if (stoppedCallback != nullptr) {
                stoppedCallback(state.program);
            }
        } else {
            // Give it 10 10 ticks to finish before stopping it manually
            if (!motorTest && state.stopping_cnt == 10) {
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
    if (motorTest) {
        // testing motor ramp
        Serial.println("Motor testing......");
        Serial.print("Current Speed: ");
        Serial.println(motor->getCurrentSpeed());
        if(motor->isTargetVelocityReached()) 
            stopMotion();
    }

    // If past end time...
    if (!motorTest && millis() >= state.run_end) {

        stopMotion();

    } else if (state.program == 0) {
        Serial.println("Check motion");
        Serial.print("Target Pos: ");
        Serial.println(motor->getTargetPosition());
        Serial.print("Current Position: ");
        Serial.println(motor->getCurrentPosition());

        // check if we need to change directions yet
        if (motor->getTargetPosition() == motor->getCurrentPosition()) {
            Serial.println("Reverse");
            // reverse directions
            state.direction = !state.direction;
            motor->setTargetPosition(state.direction ? 0 : state.washSteps);
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
        rtn = (state.run_end - millis()) / 1000;
    }

    return rtn;
}

/**
 * Return the current DriverStatus from the underlying motor control
 */
TMC5160::DriverStatus MotorControl::getDriverStatus() {
    return motor->getDriverStatus();
}

void MotorControl::setStoppedCallback(MotorCallbackFn stopFn) {

    stoppedCallback = stopFn;
}

/**
* Convert the provided RPM to a VMAX value that the library understands, which is rot/sec * 200
*
*
* Process:
* - Convert RPM to Rot/Sec (/60)
* - vmax =  deg/sec *200
*
* @param rpm
* @return
*/
float MotorControl::rpmToVmax(float rpm) {

    // Fullstep rotation per second..
    float vmax = (rpm / 60.0f) * 200;

    return vmax;
}

void MotorControl::setPwmTransitionTime(int transitionTime) {

    // Disable TMC, Enable, Wait to stabalize, and set the value
    platform->enableMotor(false);
    delay(10);
    platform->enableMotor(true);
    delay(10);
    motor->writeRegister(TMC5160_Reg::TPWMTHRS, transitionTime);
}
