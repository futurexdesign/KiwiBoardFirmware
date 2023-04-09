#pragma once

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include <TMC5160.h>
#include <TaskManagerIO.h>
#include "picoPlatform.h"
#include "settings.h"

class MotorControl : public Executable
{

    public:

        /**
         * Initialize the TMC5160 motion controller running on the provided platform.
         */
        void initMotionController(PicoPlatform *curPlatform);

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
         * Called by task loop.  This is what will process any motor control tasks necessary to execute programs.
         */
        void exec() override;

        // Track run status of motor control, and times
        struct MOTOR_STATE
        {
            bool isRunning = false;
            bool isStopping = false;
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



};
