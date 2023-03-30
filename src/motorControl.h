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
        void init_tmc(PicoPlatform*);

        void stop_motion();
        void start_program(int programId, SETTINGS currentSettings);

        /**
         * Return the amount of time remaining in the current program (in ms)
         */
        int get_remaning_time();

        /**
         * Return the currently executing program (if any).  If no program is running, return -1.
         * 0 -> wash
         * 1 -> spin
         * 2 -> dry
         */
        int get_running_program();

        bool is_running();


        /**
         * Called by task loop.  This is what will process any motor control tasks necessary to execute programs.
         */
        void exec() override;

        // Track run status of motor control, and times
        struct MOTOR_STATE
        {
            bool isRunning = false;
            bool startRequired = false;
            bool stopRequired = false;
            int program;    // 0 = agitate; 1 = spin; 2 = dry
            bool direction; // false = CW ; true = CCW
            int rpm;
            unsigned long run_start;    // when did the program phase start
            unsigned long run_end; // How long should this step be, when we excede this, do next aciton.
        };

    private:
        TMC5160_SPI *motor;
        PicoPlatform* platform;
        MOTOR_STATE state;



};
