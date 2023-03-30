#pragma once
#include <TaskManagerIO.h>
/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/


/** Pin definitions **/
/** GPIO */
#define MOTOR_EN (9u)
#define FAN_CTL (14u)
#define HEATER_CTL (15u)
#define BUTTON (18u)
#define ENC1 (17u)
#define ENC2 (16u)

/** SPI Bus for TMC5160 */
#define TMC_SCLK (10u)
#define TMC_MOSI (11u)
#define TMC_MISO (12u)
#define TMC_SS (13u)

/** i2c Bus for OLED */
#define OLED_SCL (21u)
#define OLED_SDA (20u)


class PicoPlatform : public Executable {
    public:
        /**
         * Initialize all IO on the Pico to the correct pins
        */
        void init_platform();


        /**
         * Enable or disable the heater output based on provided value.
         * The heater is enabled on logic HIGH.
        */
        void heater_enable(bool activate);

        /**
         * Enable or disable the fan output based on provided value.
         * The fan should always run if the heater is enabled.
         * The fan is enabled on logic HIGH.
        */
        void fan_enable(bool activate);

        /**
         * Enable or disable the stepper motor controller.  This is attached directly
         * to the EN pin of the stepper board.  This is active LOW.
        */
        void motor_enable(bool activate);

        /**
         * Start an optional cooldown.  Should be triggered by motion control when the dry cycle ends
         *
         * Look at the current configuration, if cooldown is enabled, calculate cooldown end, and start time
         * If cooldown is disabled, just shut the fan off.
         */
        void start_cooldown();

        /**
         * To be called by task loop.  This is charged with updating any state changes necessary with the
         * platform outside of direct motor control.
         *
         * 1: Ensure that if the heater is on... the fan MUST be on... check this every tick
         * 2: If cool-down is enabled, Check to see if the cool-down timer has expired, if it has, cycle off the fan.
         */
        void exec() override;

        bool is_motor_enabled();
        bool is_heater_enabled();
        bool is_fan_enabled();

    private:
        bool in_cooldown = false;
        bool motor_enabled = false;
        bool heater_enabled = false;
        bool fan_enabled = false;

        unsigned long cooldown_start; // when did a cooldown start.
        unsigned long cooldown_end; // when should a cooldown stop.

};