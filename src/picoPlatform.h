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
#define BUTTON (6u)
#define ENC1 (5u)
#define ENC2 (4u)

/** Expansion Port Pins **/
#define EXPANSION1 (0u)
#define EXPANSION2 (1u)
#define EXPANSION3 (2u)
#define EXPANSION4 (3u)



/** SPI Bus for TMC5160 */
#define TMC_SCLK (10u)
#define TMC_MOSI (11u)
#define TMC_MISO (12u)
#define TMC_SS (13u)

/** TFT Outputs */
#define LCD_MISO (16u)
#define LCD_SCK (18u)
#define LCD_MOSI (19u)
#define LCD_CS (17u)
#define LCD_DC (21u)
#define LCD_RESET (22u);
#define LCD_BACKLIGHT (26)


class PicoPlatform : public Executable {
    public:
        /**
         * Initialize all IO on the Pico to the correct pins
        */
        void initializePlatform();


        /**
         * Enable or disable the heater output based on provided value.
         * The heater is enabled on logic HIGH.
        */
        void enableHeater(bool activate);

        /**
         * Enable or disable the fan output based on provided value.
         * The fan should always run if the heater is enabled.
         * The fan is enabled on logic HIGH.
        */
        void enableFan(bool activate);

        /**
         * Enable or disable the stepper motor controller.  This is attached directly
         * to the EN pin of the stepper board.  This is active LOW.
        */
        void enableMotor(bool activate);

        /**
         * Start an optional cooldown.  Should be triggered by motion control when the dry cycle ends
         *
         * Look at the current configuration, if cooldown is enabled, calculate cooldown end, and start time
         * If cooldown is disabled, just shut the fan off.
         */
        void startCooldown();

        /**
         * Start an optional preheat cycle.
         *
         * Look at the current configuration,  calculate preheat end, and start time.  A call to startCooldown
         * will override any running preheat.
         */
        void startPreheat();

        /**
         * To be called by task loop.  This is charged with updating any state changes necessary with the
         * platform outside of direct motor control.
         *
         * 1: Ensure that if the heater is on... the fan MUST be on... check this every tick
         * 2: If cool-down is enabled, Check to see if the cool-down timer has expired, if it has, cycle off the fan.
         */
        void exec() override;

        /**
         * Set the backlight level.   Accept integer value 1 - 8
         *
         * This will output a PWM signal to LCD_BACKLIGHT
         */
        static void setBacklight(int value);

        bool isMotorEnabled();
        bool isHeaterEnabled();
        bool isFanEnabled();

    private:
        bool in_preheat = false;
        bool in_cooldown = false;
        bool motor_enabled = false;
        bool heater_enabled = false;
        bool fan_enabled = false;
        bool led = false; // used for heartbeat.

        unsigned long cooldown_start; // when did a cooldown start.
        unsigned long cooldown_end; // when should a cooldown stop.
        unsigned long preheat_start;
        unsigned long preheat_end;

};