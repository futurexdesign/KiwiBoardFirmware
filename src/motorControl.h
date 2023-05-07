#pragma once

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include <TMC5160.h>
#include <TaskManagerIO.h>
#include "picoPlatform.h"
#include "settings.h"

/** The definition of a callback from MotorControl object */
typedef void (*MotorCallbackFn)(int program);

class MotorControl : public Executable
{

    public:

        /**
         * Initialize the TMC5160 motion controller running on the provided platform.
         */
        void initMotionController(PicoPlatform *curPlatform, uint16_t globalScaler, uint16_t iRun, bool stealthChop);

        void stopMotion();
        void startProgram(int programId, SETTINGS currentSettings);

        /**
         * Return the number of seconds remaining in the current program.
         * This will be end time - current millis
         */
        unsigned long getSecondsRemaining();

        /**
         * Return the currently executing program (if any).  If no program is running, return -1.
         * 0 -> wash
         * 1 -> spin
         * 2 -> dry
         */
        int getRunningProgram();

        /**
         * Is there currently a cycle running?
         * @return True if a cycle is active
         */
        bool isRunning();

        /**
         * Set the callback to be called when the motor is stopped
         */
         void setStoppedCallback(MotorCallbackFn stopFn);


         /**
          * Set if StealthChop should be enabled, or if we should only use SpreadCycle.
          *
          * @param enabled
          */
         void setStealthChop(bool enabled);

         /**
          * Return the current configured motor speed.
          */
        int getMotorSpeed();

        /**
         * Override the current motor speed.
         */
        void overrideMotorSpeed(int speedRPM);

        /**
         * Called by task loop.  This is what will process any motor control tasks necessary to execute programs.
         */
        void exec() override;

        // Track run status of motor control, and times
        struct MOTOR_STATE
        {
            bool isRunning = false;
            bool isStopping = false;
            bool isTesting = false;
            int program;    // 0 = agitate; 1 = spin; 2 = dry
            bool direction; // false = CW ; true = CCW
            int washSteps; // whole steps configured for wash cycle
            unsigned int stopping_cnt = 0;

            int rpm;
            unsigned long run_start;    // when did the program phase start
            unsigned long run_end; // How long should this step be, when we exceed this, do next action.
        };

        /*
         * Return the current driver status
         */
        TMC5160::DriverStatus getDriverStatus();

    private:
        TMC5160_SPI *motor;
        PicoPlatform* platform;
        MOTOR_STATE state;

        // Callback function to call when motor stops
        MotorCallbackFn stoppedCallback;

        const float MICROSTEP_PER_REV = 51200.0;
        const float TIME_CONST = 1.398101;

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
        float rpmToVmax(float rpm);



};
