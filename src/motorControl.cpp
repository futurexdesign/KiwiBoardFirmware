/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "motorControl.h"
#include "picoPlatform.h"

void MotorControl::initMotionController(PicoPlatform *curPlatform)
{
    this->platform = curPlatform; // Set the current platform.

    // start motor controller. wants to be enabled to talk to it..
    platform->enableMotor(true);

    TMC5160::PowerStageParameters powerStageParams; // defaults.
    TMC5160::MotorParameters motorParams;
    motorParams.globalScaler = 98; // Adapt to your driver and motor (check TMC5160 datasheet - "Selecting sense resistors")
    motorParams.irun = 16;         // 31 is max running current... so cutting in half brought things down to <5w
    motorParams.ihold = 0;         // holding current, 0 enables freewheel

    // TODO Setup any stealth-chop settings we may want changed from defaults.

    motor = new TMC5160_SPI(TMC_SS, TMC5160::DEFAULT_F_CLK, SPISettings(1000000, MSBFIRST, SPI_MODE0), SPI1);

    motor->begin(powerStageParams, motorParams, TMC5160::NORMAL_MOTOR_DIRECTION);
    motor->stop(); // Ensure the controller is initialized with the motor stopped

    // TODO, add in coms check back in?

    // Check if the TMC5160 answers back
    TMC5160_Reg::IOIN_Register ioin = {0};

//    while (ioin.version != motor->IC_VERSION)
//    {
//        ioin.value = motor->readRegister(TMC5160_Reg::IO_INPUT_OUTPUT);
//
//        if (ioin.value == 0 || ioin.value == 0xFFFFFFFF)
//        {
//            Serial.println("No TMC5160 found.");
//            delay(2000);
//        }
//        else
//        {
//            Serial.println("Found a TMC device.");
//            Serial.print("IC version: 0x");
//            Serial.print(ioin.version, HEX);
//            Serial.print(" (");
//            if (ioin.version == motor->IC_VERSION)
//                Serial.println("TMC5160).");
//            else
//                Serial.println("unknown IC !)");
//        }
//    }

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
void MotorControl::startProgram(int programId, SETTINGS currentSettings)
{

    // A start has been requested
    state.program = programId;
    state.run_start = millis();
    state.isStopping = false;
    Serial.print("Program: " );
    Serial.println(state.program);

    if (programId == 0)
    {
        // wash cycle..
        state.run_end = state.run_start + (currentSettings.wash_duration * 60000);
        state.isRunning = true;

        // TODO Need to work out RPM to speed when calling the TMC driver.
        // TODO originally had cycle time configurable for wash, switch config option to number of rotations.

        // Initial prototype had agitate happening in velocity mode, which
        // has some downsides on the acceleration curves, and creates a floor on
        // how fast the cycle time can be (need enough time for it to actually accel/decel
        // should probably just switch back to positioning mode.

        platform->enableMotor(true);

        // ramp definition
        motor->setRampMode(TMC5160::POSITIONING_MODE);
        motor->setMaxSpeed(currentSettings.wash_vmax);
        // REgister for VMAX is expressed in uSteps / t
        // t is 1.398101 at 12mhz
        motor->setAcceleration(currentSettings.wash_amax);

        // Make two rotations
        motor->setTargetPosition(currentSettings.wash_pos); // 200 full steps per revolution
        state.direction = false;
    }
    else if (programId == 1)
    {
        // spin-off..
        state.run_end = state.run_start + (currentSettings.spin_duration * 60000);
        state.isRunning = true;

        // Enable drive, active low
        platform->enableMotor(true);

        // Set motor control to velocity mode, setup accelerations, max desired speed
        motor->setRampMode(TMC5160::VELOCITY_MODE);
        motor->setAcceleration(currentSettings.spin_amax); // TODO Unit?
        motor->setMaxSpeed(currentSettings.spin_vmax);      // Full steps per second

        state.direction = true;
    }
    else
    {
        // dry... which is basically a spin-off with heat
        // spin-off..
        state.run_end = state.run_start + (currentSettings.dry_duration * 60000);
        state.isRunning = true;

        // Enable drive, active low
        platform->enableMotor(true);

        // Enable heat and fan
        platform->enableHeater(true);

        // Set motor control to velocity mode, setup accelerations, max desired speed
        motor->setRampMode(TMC5160::VELOCITY_MODE);
        motor->setAcceleration(1000); // TODO Unit?
        motor->setMaxSpeed(350);      // Full steps per second

        state.direction = true;
    }
}

void MotorControl::exec()
{
    // This is our tick for motor control, Check to see if we are running, need to change directions, or stop

    if (!state.isRunning)
    {
        return;
    }

    // Check to see if we are stopping
    if (state.isStopping)
    {
        // Check to see if motion terminated, so we can disable the motion controller.
        if (motor->getCurrentSpeed() == 0)
        {
            state.isRunning = false;
            state.isStopping = false;

            platform->enableMotor(false);
        }
        else
        {
            // Give it 10 10 ticks to finish before stopping it manually
            if (state.stopping_cnt == 10)
            {
                platform->enableMotor(false);
                state.isStopping = false;
                state.isRunning = false;
            }
            else
            {
                state.stopping_cnt++;
            }
        }
        if (state.program == 2)
        {
            // trigger cooldown
            platform->startCooldown();
        }
    }

    // If past end time...
    if (millis() >= state.run_end)
    {
        motor->setMaxSpeed(0);
        platform->enableMotor(false);
        if (state.program == 2)
        {
            // If we finished a dry cycle, trigger cooldown
            platform->startCooldown();
        }
        state.isRunning = false;
    }
    else if (state.program == 0)
    {
        Serial.println("Check motion");
        Serial.print("Target Pos: ");
        Serial.println(motor->getTargetPosition());
        Serial.print("Current Position: ");
        Serial.println(motor->getCurrentPosition());

        // check if we need to change directions yet
        if (motor->getTargetPosition() == motor->getCurrentPosition())
        {
            Serial.println("Reverse");
            // reverse directions
            state.direction = !state.direction;
            motor->setTargetPosition(state.direction ? 0 : 600);
        }
    }
}

int MotorControl::getRunningProgram()
{
    return state.program;
}

bool MotorControl::isRunning()
{
    return state.isRunning;
}

void MotorControl::stopMotion()
{

    if (state.isRunning)
    {
        if (getRunningProgram() != 0)
        {
            // if we are in a wash cycle... let the current motion finish, otherwise, set speed to zero
            // and let the motion controller ramp down.
            motor->setMaxSpeed(0);
        }

        state.isStopping = true;
        state.stopping_cnt = 0;
    }
}

unsigned long MotorControl::getSecondsRemaining()
{

    unsigned long rtn = 0;
    if (state.isRunning)
    {
        rtn = (state.run_end - millis()) / 1000;
    }

    return rtn;
}
