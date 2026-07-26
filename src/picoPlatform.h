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

#if defined(ARDUINO_GENERIC_RP2350)
/**
 * RP2350 (generic_rp2350) board revision: encoder/button moved off the
 * RP2040 pins onto GPIO18-20.
 */
#define BUTTON (18u)
#define ENC1 (19u)
#define ENC2 (20u)
#else
#define BUTTON (6u)
#define ENC1 (5u)
#define ENC2 (4u)
#endif

/** Expansion Port Pins **/
#define EXPANSION1 (0u)
#define EXPANSION2 (1u)
#define EXPANSION3 (2u)
#define EXPANSION4 (3u)

/** SPI Bus for TMC5160 (TMC_SS is also half-duplex UART for TMC2209 autodetection) */
#define TMC_SCLK (10u)
#define TMC_MOSI (11u)
#define TMC_MISO (12u)
#define TMC_SS (13u)

#if defined(TMC2209_UART_HALFDUPLEX)
/**
 * TMC2209 single-pin half-duplex UART on the TMC5160 chip-select pin.
 * TMC_SS / GPIO13 = PDN_UART (TX and RX on one pin only — no expansion pins).
 *
 * PDN is hardwired to CS on this board:
 *   GPIO13 -------- PDN_UART
 *
 * RX stays UART0 on GPIO13. TX uses pad OE overrides on the same pin
 * (push-pull bits, then hi-Z to listen).
 */
#define TMC_UART_RX TMC_SS
#define TMC_UART_TX TMC_UART_RX
#elif defined(MOTOR_DRIVER_TMC2209)
/**
 * TMC2209 single-wire UART on the expansion header (hardware UART0).
 * EXPANSION1 / GPIO0 = UART TX
 * EXPANSION2 / GPIO1 = UART RX
 *
 * Wire both MCU pins to the driver PDN_UART pin:
 *   TX (GPIO0) --[1k]--+-- PDN_UART
 *   RX (GPIO1) --------+
 */
#define TMC_UART_TX EXPANSION1
#define TMC_UART_RX EXPANSION2
#endif

/** TFT Outputs */
#if defined(ARDUINO_GENERIC_RP2350)
/**
 * RP2350 (generic_rp2350) board revision: LCD moved onto its own SPI0
 * pin group (GPIO30-39). Keep these in sync with the TFT_* build flags
 * for the rp2350-* environments in platformio.ini.
 */
#define LCD_MISO (36u)
#define LCD_SCK (34u)
#define LCD_MOSI (39u)
#define LCD_CS (30u)
#define LCD_DC (32u)
#define LCD_RESET (31u)
#define LCD_BACKLIGHT (37u)
#else
#define LCD_MISO (16u)
#define LCD_SCK (18u)
#define LCD_MOSI (19u)
#define LCD_CS (17u)
#define LCD_DC (21u)
#define LCD_RESET (22u);
#define LCD_BACKLIGHT (26)
#endif


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
         * Enable or disable the sounder. This is attached to  
         * to the expansion header.  This is active HIGH.
        */

        void enableSounder(bool activate);

        /*
        * Setup PWM for Pico GPIO pins
        */
        uint32_t pwm_set_freq_duty(uint slice_num,
                    uint chan,uint32_t f, int d);

        /*
        * Set new sound level
        */
        void set_audioLevel(int);

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

        uint slice;
        uint channel;

        unsigned long cooldown_start; // when did a cooldown start.
        unsigned long cooldown_end; // when should a cooldown stop.
        unsigned long preheat_start;
        unsigned long preheat_end;

        int sndLevel;

};