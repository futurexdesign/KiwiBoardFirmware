/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */
#include <Arduino.h>
#include "picoPlatform.h"
#include "motorControl.h"
#include "KiwiBoardFirmware_menu.h"
#include <BaseDialog.h>
#include "splashScreen.h"

PicoPlatform *platform;
MotorControl *motorControl;

bool led = false;

// Track if we need to save setting changes.
bool settingsChanged = false;

void ui_tick();
void motorErrorDialog(TMC5160::DriverStatus);
void titleBarClick(int id);

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

void setup()
{

    Serial.begin(115200);
    Serial.println("Booted");

    platform = new PicoPlatform();
    platform->initializePlatform();
    // bring up motor control with configured values
    motorControl = new MotorControl();
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(), menuIRun.getIntValueIncludingOffset());

    // Init the graphics subsystem and trigger the splash.
    gfx.begin();
    gfx.setRotation(3);

    // turn on the LED if DMA init successfully..
    bool DMA = gfx.initDMA(true);
    if (DMA)
    {
        // Activate onboard LED to provide some sort of boot status
        pinMode(LED_BUILTIN, OUTPUT);
        digitalWrite(LED_BUILTIN, 1);
        led = true;
    }

    showSplash();

    delay(2000);
    gfx.fillScreen(TFT_BLACK);

    menuRunTime.setReadOnly(true);
    setupMenu();
    menuMgr.load(0xfadf, NULL);

    // Check for encoder inversion..
    if (menuInvertEncoder.getBoolean())
    {
        // Inversion selected, reinitialize the encoder plugin with the pins reversed.
        Serial.println("Encoder inversion requested.. reinit menuMgr with encoder flipped");
        menuMgr.initForEncoder(&renderer, &menuProgram, ENC2, ENC1, BUTTON);
    }

    setTitlePressedCallback(titleBarClick);

    // Setup tasks
    // Platform task is relatively low priority
    // Schedule the platform task every 200ms
    taskManager.scheduleFixedRate(200, platform, TIME_MILLIS);

    // Motor task is the highest priority.
    // Schedule the motor task for every 50ms
    taskManager.scheduleFixedRate(150, motorControl, TIME_MILLIS);

    // UI update task is medium priority.  We are updating the countdown on the display.
    // Every half a second should be plenty for a smooth looking countdown
    taskManager.scheduleFixedRate(200, ui_tick, TIME_MILLIS);

    // To prevent thrashing of the EEPROM, only save settings periodically if things were touched.
    taskManager.scheduleFixedRate(60, commit_if_needed, TIME_SECONDS);
}

/**
 * Main loop, we are delegating to the Task library to trigger all of our main processing, so pump the event bus
 */
void loop()
{
    taskManager.runLoop();
}

/**
 * Perform UI updates when the task manager calls us.
 *
 * 1: If we are running, update the time remaining.
 * 2: If we just finished a program, show the complete dialog (?)
 * 3: Update the menu bar icons if the heater or fan are active (need those icons)
 */
void ui_tick()
{

    led = !led;

    digitalWrite(LED_BUILTIN, led);
    // Update the remaining time...
    if (motorControl->isRunning())
    {
        unsigned long secRemaining = motorControl->getSecondsRemaining();

        int minutes = secRemaining / 60;
        int secRemand = secRemaining % 60;

        struct TimeStorage ts = menuRunTime.getTime();
        ts.seconds = secRemand;
        ts.minutes = minutes;
        menuRunTime.setTime(ts);
        menuRunTime.changeOccurred(true);
    }
    else
    {
        struct TimeStorage ts = menuRunTime.getTime();
        if (ts.seconds != 0)
        {
            ts.seconds = 0;
            ts.minutes = 0;
            menuRunTime.setTime(ts);
            menuRunTime.changeOccurred(true);
        }
    }

    // Check for motor status, if overheated or shorted, alert user
    TMC5160::DriverStatus curStatus = motorControl->getDriverStatus();

    if (curStatus != TMC5160::DriverStatus::OK)
    {
        // launch error dialog
        motorErrorDialog(curStatus);
    }
}

/**
 * An error has been found.  Present a dialog to the user so they know what happened.
 */
void motorErrorDialog(TMC5160::DriverStatus status)
{
    Serial.println("Stepper drive error detected, reports not ok");
    Serial.print("Current drive status: ");
    Serial.println(status);

    const char error[] PROGMEM = "Stepper Drive Error";
    char msg[50]; // 200 character string
    switch (status)
    {
    case TMC5160::OT:
    case TMC5160::OTPW:
        strcpy(msg, "Driver is overheating");
        break;
    case TMC5160::S2VSA:
        strcpy(msg, "A Phase short to 12v");
        break;
    case TMC5160::S2VSB:
        strcpy(msg, "B Phase short to 12v");
        break;
    case TMC5160::S2GA:
        strcpy(msg, "A Phase short to gnd");
        break;
    case TMC5160::S2GB:
        strcpy(msg, "B Phase short to gnd");
        break;
    case TMC5160::CP_UV:
        strcpy(msg, "CP Under-volt");
        break;
    case TMC5160::OTHER_ERR:
        strcpy(msg, "Unknown TMC Error");
        break;
    }

    BaseDialog *dlg = renderer.getDialog();
    if (dlg)
    {
        dlg->setButtons(BTNTYPE_NONE, BTNTYPE_CLOSE);
        dlg->show(error, false);
        dlg->copyIntoBuffer(msg);
    }
}

/**
 * Show version dialog.
 *
 * @param id
 */
void titleBarClick(int id)
{
}

void CALLBACK_FUNCTION progChange(int id)
{
}

/**
 * RUN | STOP trigger function.   This is called when the user selects the run/stop menu item.
 *
 * If we are running, force a stop of the current program
 * If we are stopped, trigger a start of the selected program
 */
void CALLBACK_FUNCTION run(int id)
{

    if (motorControl->isRunning())
    {
        motorControl->stopMotion();
    }
    else
    {
        motorControl->startProgram(menuProgram.getCurrentValue(), getSettings());
    }
}

void CALLBACK_FUNCTION settings_changed(int id)
{

    settingsChanged = true;
}

/**
 * Callback when the user changes global scaler.   The motor control needs to be fully stopped,
 * and the initialized again with new value.  This shouldn't be done with the motor running..
 *
 * @param id
 */
void CALLBACK_FUNCTION GlobalScalerChanged(int id)
{
    Serial.println("GlobalScaler changed... e-stop motor control, and reinit ");

    // Shut off the TMC
    platform->enableMotor(false);
    delay(100); // wait for everything to settle...
    // init
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(), menuIRun.getIntValueIncludingOffset());

    settingsChanged = true;
}

/**
 * Callback when the user changes global scaler.   The motor control needs to be fully stopped,
 * and the initialized again with new value.  This shouldn't be done with the motor running..
 *
 * @param id
 */
void CALLBACK_FUNCTION iRunChanged(int id)
{
    Serial.println("iRun changed... e-stop motor control, and reinit ");

    // Shut off the TMC
    platform->enableMotor(false);
    delay(100); // wait for everything to settle...
    // init
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(), menuIRun.getIntValueIncludingOffset());

    settingsChanged = true;
}

void CALLBACK_FUNCTION backlightChange(int id)
{
    Serial.println("Backlight change request");
}
/**
 * Commit settings to eeprom if they have changed
 */
void commit_if_needed()
{
    Serial.println("checking if settings changed.");
    // if (settingsChanged)
    // {
    //     menuMgr.save(0xfadf);
    //     //EEPROM.commit();
    //     settingsChanged = false;
    //     Serial.println("changes committed" );
    // }
}
