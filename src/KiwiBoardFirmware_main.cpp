/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/
#include <Arduino.h>
#include "picoPlatform.h"
#include "motorControl.h"
#include "KiwiBoardFirmware_menu.h"

PicoPlatform *platform;
MotorControl *motorControl;

// Track if we need to save setting changes. 
bool settingsChanged = false;

void ui_tick();

// Display
U8G2_SH1107_128X128_2_HW_I2C gfx(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

/**
 * Initialize the platform.   For the RP2040, we want to make sure that we configure all of the GPIO ports
 * are set to the correct configuration, and that the I2C and SPI hardware is mapped to the correct pins
 * as it is highly configurable.
 *
 * Once the MCU is configured, configure and initialize the stepper motion controller.  Set motor power
 * staging and stealth chop parameters.
 *
 * Finally configure and initialize the menu system.
 */

void setup() {

    platform = new PicoPlatform();
    platform->init_platform();

    motorControl = new MotorControl();
    motorControl->init_tmc(platform);

    setupMenu();
    menuMgr.load(0xfade, NULL);

    // Setup tasks
    // Platform task is relatively low priority
    // Schedule the platform task every 200ms
    taskManager.scheduleFixedRate(200, platform, TIME_MILLIS);

    // Motor task is the highest priority.
    // Schedule the motor task for every 50ms
    taskManager.scheduleFixedRate(50, motorControl, TIME_MILLIS);

    // UI update task is medium priority.  We are updating the countdown on the display.
    // Every half a second should be plenty for a smooth looking countdown
    taskManager.scheduleFixedRate(500, ui_tick, TIME_MILLIS);

    // To prevent thrashing of the EEPROM, only save settings periodically if things were touched.
    taskManager.scheduleFixedRate(60, commit_if_needed, TIME_SECONDS);

}

/**
 * Main loop, we are delegating to the Task library to trigger all of our main processing, so pump the event bus
*/
void loop() {

    taskManager.runLoop();

}

/**
 * Perform UI updates when the task manager calls us.
 *
 * 1: If we are running, update the time remaining.
 * 2: If we just finished a program, show the complete dialog (?)
 * 3: Update the menu bar icons if the heater or fan are active (need those icons)
 */
void ui_tick() {

}

void CALLBACK_FUNCTION progChange(int id) {

}

/**
 * RUN | STOP trigger function.   This is called when the user selects the run/stop menu item.
 * 
 * If we are running, force a stop of the current program 
 * If we are stopped, trigger a start of the selected program
*/
void CALLBACK_FUNCTION run(int id);

void run(int id) {

}


void CALLBACK_FUNCTION settings_changed(int id) {

    settingsChanged = true;

}
