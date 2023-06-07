/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */
#include <Arduino.h>
#include "picoPlatform.h"
#include "motorControl.h"
#include "KiwiBoardFirmware_menu.h"
#include <BaseDialog.h>
#include "splashScreen.h"
#include "KiwiBoardFirmware_main.h"
#include "EncoderShim.h"
#include "heat.h"
#include "Sounder.h"

#ifdef SCREENCAP
#include "screenServer.h"
#endif

#include "icons.h"
#include "MenuChangeObserver.h"

// Version Number
const char VERSION_NUM[] PROGMEM = "1.1.3";


PicoPlatform *platform;
MotorControl *motorControl = nullptr;
MenuChangeObserver *observer;
EncoderShim *encoderShim;
BeepHandler *sounderOps; // Declare sounderOps (based on class BeepHandler)

// Error occurred, in HALT state.
bool HALT = false;
bool DIALOG = false;

// Track if we need to save setting changes.
bool settingsChanged = false;

MenuItem *stoppedButton;

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

    Serial.begin(115200);
    Serial.println("Booted");

    platform = new PicoPlatform();
    platform->initializePlatform();

    // Init the graphics subsystem and trigger the splash.
    gfx.begin();
    gfx.setRotation(3);
    gfx.initDMA(true);

    showSplash();

    delay(2000);
    gfx.fillScreen(TFT_BLACK);

    // Setup Sounder
    sounderOps = new BeepHandler(platform); // Instantiate object sounderOps based on BeepHandler

    // Setup switches and encoder?
    encoderShim = new EncoderShim();
    encoderShim->initForEncoder();
    encoderShim->registerChangeCallback(handleEncoderMove);
    encoderShim->registerClickCallback(checkLongPress);

    menuRunTime.setReadOnly(true);
    setupMenu();
    menuMgr.load(0xfadf, nullptr);

    // bring up motor control with configured values
    motorControl = new MotorControl();
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(),
                                       menuIRun.getIntValueIncludingOffset(),
                                       menuStealthChop.getBoolean());

    motorControl->setStoppedCallback(stoppedCallback);

    // restore backlight setting
    PicoPlatform::setBacklight(menuBacklight.getIntValueIncludingOffset());

    // Check for encoder inversion..
    if (menuInvertEncoder.getBoolean()) {
        // Inversion selected, reinitialize the encoder plugin with the pins reversed.
        encoderShim->invertEncoderDirection();
        // reinit menu system so encoder is configured properly.
        setupMenu();
    }

    // Get saved values for sounder and sound level..
    sounderOps->set_menuSound(menusounder.getBoolean()); 
    sounderOps->set_sndLevel(menuSoundLevel.getIntValueIncludingOffset());  

    observer = new MenuChangeObserver(&menuMgr, &menuRunTime, &menuWash);
    menuMgr.addChangeNotification(observer);

    menuVersion.setTextValue(VERSION_NUM, true);

    setMenuOptions();
    scheduleTasks();

}

/**
 * Main loop, we are delegating to the Task library to trigger all of our main processing, so pump the event bus
 */
void loop() {
    if (!HALT) {
        taskManager.runLoop();
    }

}

void stoppedCallback(int pgm) {

    // Stopped happened.
    sounderOps->beep_activate(0, false); // 0 = End of cycle tone
    resetIcons();
    observer->resetConstraint();
}

/**
 * Perform UI updates when the task manager calls us.
 *
 * 1: If we are running, update the time remaining.
 * 2: If we just finished a program, show the complete dialog (?)
 * 3: Update the menu bar icons if the heater or fan are active (need those icons)
 */
void ui_tick() {

    // Check for motor status, if overheated or shorted, alert user
    TMC5160::DriverStatus curStatus = motorControl->getDriverStatus();
    if (!DIALOG && curStatus != TMC5160::DriverStatus::OK) {

        // launch error dialog
        motorErrorDialog(curStatus);
    } else {

        // Update the remaining time...
        if (motorControl->isRunning()) {
            unsigned long secRemaining = motorControl->getSecondsRemaining();

            int minutes = secRemaining / 60;
            int secRemand = secRemaining % 60;

            struct TimeStorage ts = menuRunTime.getTime();
            if (ts.seconds != secRemand || ts.minutes != minutes) {
                ts.seconds = secRemand;
                ts.minutes = minutes;
                menuRunTime.setTime(ts);
                menuRunTime.changeOccurred(true);
            }
        } else {
            struct TimeStorage ts = menuRunTime.getTime();
            if (ts.seconds != 0) {
                ts.seconds = 0;
                ts.minutes = 0;
                menuRunTime.setTime(ts);
                menuRunTime.changeOccurred(true);
            }
        }


        // Draw the Logo, Centered in the title bar, but only on the main screen
        // If currentSubMenu is null, we are on the "home" screen... Show the centered icon.
        if (menuMgr.getCurrentSubMenu() == nullptr) {
            auto drawable = renderer.getDeviceDrawable();
            drawable->setColors(RGB(255, 255, 255), RGB(0, 0, 0));
            drawable->startDraw();
            drawable->drawXBitmap(Coord(135, 0), Coord(50, 50), KiwiLogoWidIcon0);
            drawable->endDraw();

            drawable = renderer.getDeviceDrawable();
            drawable->setColors(RGB(0, 0, 0), RGB(0, 0, 0));
            drawable->drawBox(Coord(0, 238), Coord(320,2), true);
            drawable->endDraw();

        }

        // Check for heat icon, if the heater is on, show the icon,  otherwise swap in the blank widget
        if (platform->isHeaterEnabled()) {
            if (HeatWidget.getCurrentState() != 1) {
                HeatWidget.setCurrentState(1);
            }
        } else {
            if (HeatWidget.getCurrentState() != 0) {
                HeatWidget.setCurrentState(0);
            }
        }
    }

}

/**
 * An error has been found.  Present a dialog to the user so they know what happened.
 */
void motorErrorDialog(TMC5160::DriverStatus status) {
    DIALOG = true; // showing an error dialog, set so we dont keep trying to render the dialog over and over
    Serial.println("Stepper drive error detected, reports not ok");
    Serial.print("Current drive status: ");
    Serial.println(status);

    // Terminate Platform Services
    // Turn off motor and heater, leave fan running
    platform->enableMotor(false);
    platform->enableHeater(false);
    platform->enableFan(true); // die with the fan running

    const char error[] PROGMEM = "Stepper Drive Error";
    char msg[50]; // 200 character string
    switch (status) {
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
        default:
            strcpy(msg, "Unknown TMC Error");
    }

    BaseDialog *dlg = renderer.getDialog();
    if (dlg) {
        dlg->setButtons(BTNTYPE_NONE, BTNTYPE_CLOSE);
        dlg->show(error, false, doErrorHalt);
        dlg->copyIntoBuffer(msg);
    }
}

void CALLBACK_FUNCTION wash(int id) {
    run(0, &menuWash);
}

void CALLBACK_FUNCTION spin(int id) {
    run(1, &menuSpin);
}

void CALLBACK_FUNCTION dry(int id) {
    run(2, &menuDry);
}

void CALLBACK_FUNCTION motortest(int id) {
    run(9, &menumotorTest);
}

/**
 * RUN | STOP trigger function.   This is called when the user selects the run/stop menu item.
 *
 * If we are running, force a stop of the current program
 * If we are stopped, trigger a start of the selected program
 */
void run(int program, MenuItem *icon) {

    if (motorControl->isRunning()) {
        motorControl->stopMotion();
    } else {
        motorControl->startProgram(program, getSettings());
        setIconStopped(icon);
        observer->constrainToStopButton(icon);
    }
}

void CALLBACK_FUNCTION soundLevel(int id) {

    // Get changes to sound level and then set
    // beep for every turn of the encoder when setting sound level

    sounderOps->set_sndLevel(menuSoundLevel.getIntValueIncludingOffset());
    sounderOps->beep_activate(1, true); // Short beep, override soundset var
    settingsChanged = true; // Save settings

}

void CALLBACK_FUNCTION settings_changed(int id) {
    // TODO Look for actual changes in the setting values rather than just saving settings any time
    // someone went to the settings menu.... Or add a save button?

    settingsChanged = true;
}

/**
 * Callback when the user changes global scaler.   The motor control needs to be fully stopped,
 * and the initialized again with new value.  This shouldn't be done with the motor running..
 *
 * @param id
 */
void CALLBACK_FUNCTION GlobalScalerChanged(int id) {
    Serial.println("GlobalScaler changed... e-stop motor control, and reinit ");

    // Shut off the TMC
    platform->enableMotor(false);
    delay(100); // wait for everything to settle...
    // init
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(),
                                       menuIRun.getIntValueIncludingOffset(),
                                       menuStealthChop.getBoolean());

    settingsChanged = true;
}

/**
 * Callback when the user changes global scaler.   The motor control needs to be fully stopped,
 * and the initialized again with new value.  This shouldn't be done with the motor running..
 *
 * @param id
 */
void CALLBACK_FUNCTION iRunChanged(int id) {
    Serial.println("iRun changed... e-stop motor control, and reinit ");

    // Shut off the TMC
    platform->enableMotor(false);
    delay(100); // wait for everything to settle...
    // init
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(),
                                       menuIRun.getIntValueIncludingOffset(),
                                       menuStealthChop.getBoolean());

    settingsChanged = true;
}

/**
 * When the backlight value changes, inform the platform.
 */
void CALLBACK_FUNCTION backlightChange(int id) {
    int newVal = menuBacklight.getIntValueIncludingOffset();
    PicoPlatform::setBacklight(newVal);
}


/**
 * Callback for when the user changes the StealthChop setting.
 *
 * @param id
 */
void CALLBACK_FUNCTION stealthChopChange(int id) {

    // Shut off the TMC
    platform->enableMotor(false);
    delay(100); // wait for everything to settle...
    // init
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(),
                                       menuIRun.getIntValueIncludingOffset(),
                                       menuStealthChop.getBoolean());

    settingsChanged = true;
}

/**
 * Callback when the user changes the sound setting.  
 *
 * @param id
 */
void CALLBACK_FUNCTION soundChanged(int id) {
    Serial.println("Sound changed...  ");

    sounderOps->set_menuSound(menusounder.getBoolean());

    settingsChanged = true;
}

/**
 * Commit settings to eeprom if they have changed.  Try not to thrash the EEPROM with too many
 * calls.
 */
void commit_if_needed() {
    Serial.println("checking if settings changed.");
    if (settingsChanged) {
        menuMgr.save(0xfadf);
        EEPROM.commit();
        settingsChanged = false;
        Serial.println("changes committed");
    }
}

/**
 * Schedule of the background tasks that need to run in the main task loop.   The TaskManager
 * will then trigger these once their timers expire.
 *
 * - Platform Background tasks
 * - Motor Control Task
 * - UI Update task
 * - Setting Saving task
 * - Optional Screen Capture task.
 */
void scheduleTasks() {
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

    // Sounder operation needs to be non-blocking so we update the status regularly
    // Schedule sounder updates for every 20ms to ensure granularity for short beeps
    taskManager.scheduleFixedRate(20, sounderOps, TIME_MILLIS); 

    // Only schedule the screen capture if SCREENCAP is defined from platformio.ini
#ifdef SCREENCAP
    taskManager.scheduleFixedRate(2, screenCaptureTask, TIME_SECONDS);
#endif
}

/**
 * Setup all the UI options required to make NewUI work.
 *
 * - Swap out the wash/spin/dry/settings menu options for a grid row of icons.
 * - Set the palette for the timer row (which is inverse of everything else.
 * - Set the palette for the grid row.
 * - Set the palette for the settings menus (Maybe this should jus tbe the default palette... override just the home screen entries
 */
void setMenuOptions() {

    // first we get the graphics factory
    auto &factory = renderer.getGraphicsPropertiesFactory();

    // don't do the periodic reset, which causes some awkward redraw flashes.
    renderer.turnOffResetLogic();

    // now we add the icons that we want to use with certain menu items
    const Coord iconSize(72, 72);
    factory.addImageToCache(
            DrawableIcon(menuWash.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap0, CycleIconsBitmap3));
    factory.addImageToCache(
            DrawableIcon(menuSpin.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap1, CycleIconsBitmap3));
    factory.addImageToCache(DrawableIcon(menuDry.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap2));
    factory.addImageToCache(
            DrawableIcon(menuSettings.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap3));

    // and now we define that row 3 of the main menu will have three columns, drawn as icons
    factory.addGridPosition(&menuWash, GridPosition(GridPosition::DRAW_AS_ICON_ONLY,
                                                    GridPosition::JUSTIFY_CENTER_NO_VALUE, 4, 1, 2, 80));
    factory.addGridPosition(&menuSpin, GridPosition(GridPosition::DRAW_AS_ICON_ONLY,
                                                    GridPosition::JUSTIFY_CENTER_NO_VALUE, 4, 2, 2, 80));
    factory.addGridPosition(&menuDry, GridPosition(GridPosition::DRAW_AS_ICON_ONLY,
                                                   GridPosition::JUSTIFY_CENTER_NO_VALUE, 4, 3, 2, 80));
    factory.addGridPosition(&menuSettings, GridPosition(GridPosition::DRAW_AS_ICON_ONLY,
                                                        GridPosition::JUSTIFY_CENTER_NO_VALUE, 4, 4, 2, 80));

    const color_t settingsMenuPalette[] = {RGB(255, 255, 255), RGB(0, 0, 0), RGB(43, 43, 43), RGB(65, 65, 65)};

    MenuPadding perSidePadding(15, 4, 15, 0);

    const color_t timmerPalette[] = {RGB(0, 0, 0), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255)};

    // Settings for the timer
    factory.setDrawingPropertiesForItem(ItemDisplayProperties::COMPTYPE_ITEM, menuRunTime.getId(), timmerPalette,
                                        perSidePadding, MenuFontDef(nullptr, 7).fontData,
                                        MenuFontDef(nullptr, 7).fontMag, 2, 100,
                                        GridPosition::JUSTIFY_CENTER_VALUE_ONLY, MenuBorder(0));

    // Settings for the Settings menu
    factory.setDrawingPropertiesAllInSub(ItemDisplayProperties::COMPTYPE_ITEM, menuSettings.getId(),
                                         settingsMenuPalette,
                                         MenuPadding(4), nullptr, 4, 2, 36,
                                         GridPosition::JUSTIFY_TITLE_LEFT_VALUE_RIGHT, MenuBorder(0));

    factory.setDrawingPropertiesAllInSub(ItemDisplayProperties::COMPTYPE_ITEM, menuwashSettings.getId(),
                                         settingsMenuPalette,
                                         MenuPadding(4), nullptr, 4, 2, 36,
                                         GridPosition::JUSTIFY_TITLE_LEFT_VALUE_RIGHT, MenuBorder(0));

    factory.setDrawingPropertiesAllInSub(ItemDisplayProperties::COMPTYPE_ITEM, menuSpinSettings.getId(),
                                         settingsMenuPalette,
                                         MenuPadding(4), nullptr, 4, 2, 36,
                                         GridPosition::JUSTIFY_TITLE_LEFT_VALUE_RIGHT, MenuBorder(0));

    factory.setDrawingPropertiesAllInSub(ItemDisplayProperties::COMPTYPE_ITEM, menuDrySettings.getId(),
                                         settingsMenuPalette,
                                         MenuPadding(4), nullptr, 4, 10, 36,
                                         GridPosition::JUSTIFY_CENTER_WITH_VALUE, MenuBorder(0));

    factory.setDrawingPropertiesAllInSub(ItemDisplayProperties::COMPTYPE_ITEM, menuAdvanced.getId(),
                                         settingsMenuPalette,
                                         MenuPadding(4), nullptr, 4, 10, 36,
                                         GridPosition::JUSTIFY_CENTER_WITH_VALUE, MenuBorder(0));

    // Blank the title bar so that we can render the logo overtop of it. (Save a draw call to blank the rectangle)
    appTitleMenuItem.setTitleOverridePgm("");

    // Setup heat icon?
    renderer.setFirstWidget(&HeatWidget);

    // Black on white cursor
    factory.setSelectedColors(RGB(255, 255, 255), RGB(0, 0, 0));

    // Home the user to the wash menu option
    menuMgr.activateMenuItem(&menuWash);

    // We changed out icons and whatnot, so we need to force a redraw
    tcgfx::ConfigurableItemDisplayPropertiesFactory::refreshCache();
}

void setIconStopped(MenuItem *icon) {
    // renderer.takeOverDisplay(renderTimer);

    stoppedButton = icon; // Keep track of this so we can reset it later.

    const Coord iconSize(72, 72);

    MenuPadding perSidePadding(0, 0, 0, 0);

    auto &factory = renderer.getGraphicsPropertiesFactory();

    const color_t activePalette[] = {RGB(255, 0, 0), RGB(255, 255, 255), RGB(0, 0, 255), RGB(255, 0, 0)};
    factory.setDrawingPropertiesForItem(ItemDisplayProperties::COMPTYPE_ACTION, icon->getId(), activePalette,
                                        perSidePadding, MenuFontDef(nullptr, 2).fontData,
                                        MenuFontDef(nullptr, 7).fontMag, 0, 60, GridPosition::CORE_JUSTIFY_RIGHT,
                                        MenuBorder(0));

    factory.addImageToCache(DrawableIcon(icon->getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap4));

    factory.setSelectedColors(RGB(255, 255, 255), RGB(255, 0, 0));
    icon->setChanged(true);

    tcgfx::ConfigurableItemDisplayPropertiesFactory::refreshCache();
}

void checkLongPress(bool direction, bool held) {
    
    // Button pressed, so we beep
    sounderOps->beep_activate(1, false); // Short beep, no soundset override
    
    // Check for a long press... no idea what menu ... but whatever?
    if (held) {
        // what are we long pressing on?
        if (menuMgr.findCurrentActive()->getId() == menuSpin.getId() ||
            menuMgr.findCurrentActive()->getId() == menuWash.getId()) {
            if (platform->isHeaterEnabled()) {
                // cancel preheat
                platform->enableHeater(false);
            } else {
                platform->startPreheat();
            }
        }
    }
}

/**
 * Reset all of the icons back to their original state, reset the selection color value.
 */
void resetIcons() {

    auto &factory = renderer.getGraphicsPropertiesFactory();
    const Coord iconSize(72, 72);

    factory.addImageToCache(
            DrawableIcon(menuWash.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap0, CycleIconsBitmap3));
    factory.addImageToCache(
            DrawableIcon(menuSpin.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap1, CycleIconsBitmap3));
    factory.addImageToCache(DrawableIcon(menuDry.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap2));
    factory.addImageToCache(
            DrawableIcon(menuSettings.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap3));

    if (stoppedButton != nullptr) {
        MenuPadding buttonPadding(4, 4, 4, 4);

        factory.setDrawingPropertiesForItem(ItemDisplayProperties::COMPTYPE_ACTION, stoppedButton->getId(),
                                            darkModeActionPalette, buttonPadding, MenuFontDef(nullptr, 2).fontData,
                                            MenuFontDef(nullptr, 7).fontMag, 2, 60, GridPosition::CORE_JUSTIFY_RIGHT,
                                            MenuBorder(0));
        stoppedButton = nullptr;
    }

    factory.setSelectedColors(RGB(255, 255, 255), RGB(0, 0, 0));

    // // Rerender entire screen
    tcgfx::ConfigurableItemDisplayPropertiesFactory::refreshCache();
}

/**
 * Screen Capture Task
 */
void screenCaptureTask() {

#ifdef SCREENCAP
    // If EXPANSION1 is high, trigger the screen capture routine.
    if (digitalRead(EXPANSION1) == HIGH)
    {
        screenServer();
    }
#endif
}

/**
 * Handle an encoder movement.  Only do anything if we are actively running a program, otherwise we don't care
 *
 * @param direction
 * @param held
 */
void handleEncoderMove(bool direction, bool held) {

//    Serial.println("handleEncoderMove...");
//    if (motorControl != nullptr && motorControl->isRunning()) {
//        Serial.println("program running.. change speed");
//        // We are running, track left / right as speed changes..
//
//        int curSpeed = motorControl->getMotorSpeed();
//
//        // TODO Add bounds...
//        if (!direction) {
//
//            curSpeed = curSpeed - 10;
//        } else {
//            curSpeed = curSpeed + 10;
//        }
//
//        motorControl->overrideMotorSpeed(curSpeed);
//        Serial.println(curSpeed);


//        BaseDialog *dlg = renderer.getDialog();
//        if (dlg) {
//            dlg->setButtons(BTNTYPE_CLOSE, BTNTYPE_NONE);
//            dlg->show("Motor Speed Changed", false);
////            char cstr[16];
////            itoa(curSpeed, cstr, 10);
////
////            dlg->copyIntoBuffer(cstr);
//        }

    //}

}

/**
 * Function to be called once the user tries to dismiss the error dialog.  Set halt flag.
 */
void doErrorHalt(ButtonType buttonPressed, void *yourData) {
    HALT = true; // Stop the run loop.

}
