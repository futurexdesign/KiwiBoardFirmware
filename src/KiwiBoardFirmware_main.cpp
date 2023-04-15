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
#include "screenServer.h"
#include "icons.h"

PicoPlatform *platform;
MotorControl *motorControl;

bool led = false;

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

void setup()
{

    Serial.begin(115200);
    Serial.println("Booted");

    platform = new PicoPlatform();
    platform->initializePlatform();
    // bring up motor control with configured values
    motorControl = new MotorControl();
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(),
                                       menuIRun.getIntValueIncludingOffset());

    // Init the graphics subsystem and trigger the splash.
    gfx.begin();
    gfx.setRotation(3);

    // turn on the LED if DMA init successfully..
    gfx.initDMA(true);

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
        menuMgr.initForEncoder(&renderer, &menuRunTime, ENC2, ENC1, BUTTON);
    }

    setMenuOptions();

    setTitlePressedCallback(titleBarClick);

    scheduleTasks();
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
        if (ts.seconds != secRemand || ts.minutes != minutes)
        {
            ts.seconds = secRemand;
            ts.minutes = minutes;
            menuRunTime.setTime(ts);
            menuRunTime.changeOccurred(true);
        }
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

            resetIcons();
        }
    }

    // Check for motor status, if overheated or shorted, alert user
    TMC5160::DriverStatus curStatus = motorControl->getDriverStatus();
    Serial.println("Driver Status:");
    Serial.println(curStatus);
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
 * Screen Capture Task
 */
void screenCaptureTask()
{

#ifdef SCREENCAP
    // If EXPANSION1 is high, trigger the screen capture routine.
    if (digitalRead(EXPANSION1) == HIGH)
    {
        screenServer();
    }
#endif
}

/**
 * Show version dialog.
 *
 * @param id
 */
void titleBarClick(int id)
{

    const char error[] PROGMEM = "Free Heap Usage";

    BaseDialog *dlg = renderer.getDialog();
    if (dlg)
    {
        dlg->setButtons(BTNTYPE_NONE, BTNTYPE_CLOSE);
        dlg->show(error, false);
        int freeHeap = rp2040.getFreeHeap();
        char cstr[16];
        itoa(freeHeap, cstr, 10);
        dlg->copyIntoBuffer(cstr);
    }
}

void CALLBACK_FUNCTION wash(int id)
{
    run(0);
    setIconStopped(&menuWash);
}

void CALLBACK_FUNCTION spin(int id)
{
    run(1);
    setIconStopped(&menuSpin);
}

void CALLBACK_FUNCTION dry(int id)
{
    run(2);
    setIconStopped(&menuDry);
}

/**
 * RUN | STOP trigger function.   This is called when the user selects the run/stop menu item.
 *
 * If we are running, force a stop of the current program
 * If we are stopped, trigger a start of the selected program
 */
void run(int program)
{

    if (motorControl->isRunning())
    {
        motorControl->stopMotion();
    }
    else
    {
        motorControl->startProgram(program, getSettings());
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
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(),
                                       menuIRun.getIntValueIncludingOffset());

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
    motorControl->initMotionController(platform, menuGlobalScaler.getIntValueIncludingOffset(),
                                       menuIRun.getIntValueIncludingOffset());

    settingsChanged = true;
}

/**
 * When the backlight value changes, inform the platform.
 */
void CALLBACK_FUNCTION backlightChange(int id)
{
    int newVal = menuBacklight.getIntValueIncludingOffset();
    PicoPlatform::setBacklight(newVal);
}

/**
 * Commit settings to eeprom if they have changed
 */
void commit_if_needed()
{
    Serial.println("checking if settings changed.");
    // TODO Why did this break?
    //  if (settingsChanged)
    //  {
    //      menuMgr.save(0xfadf);
    //      //EEPROM.commit();
    //      settingsChanged = false;
    //      Serial.println("changes committed" );
    //  }
}

void scheduleTasks()
{
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

    // Only schedule the screen capture if SCREENCAP is defined from platformio.ini
#ifdef SCREENCAP
    taskManager.scheduleFixedRate(2, screenCaptureTask, TIME_SECONDS);
#endif
}

/**
 * Setup all the UI options required to make NewUI work
 */
void setMenuOptions()
{

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
     // // here is how we completely redefine the drawing of a specific item, you can also define for submenu or default
    // color_t specialPalette[] { RGB(255, 255, 255), RGB(255, 0, 0), RGB(0, 0, 0), RGB(0, 0, 255) };
    factory.setDrawingPropertiesAllInSub(ItemDisplayProperties::COMPTYPE_ITEM, menuSettings.getId(), settingsMenuPalette,
                                        MenuPadding(4), nullptr, 4, 10, 36,
                                        GridPosition::JUSTIFY_CENTER_WITH_VALUE , MenuBorder(2));

    appTitleMenuItem.setTitleOverridePgm("KiwiCleaner");
    factory.setSelectedColors(RGB(255, 255, 255), RGB(0, 0, 0));


    // title widget... try one..
    renderer.setFirstWidget(&KiwiLogoWidget);
}

void setIconStopped(MenuItem *icon)
{

    stoppedButton = icon; // Keep track of this so we can reset it later.

    const Coord iconSize(72, 72);

    // See what we can do.. can we make it green background?
    MenuPadding perSidePadding(4, 4, 4, 4);

    auto &factory = renderer.getGraphicsPropertiesFactory();
    const color_t activePalette[] = {RGB(255, 0, 0), RGB(255, 255, 255), RGB(0, 0, 255), RGB(255, 0, 0)};
    factory.setDrawingPropertiesForItem(ItemDisplayProperties::COMPTYPE_ACTION, icon->getId(), activePalette, perSidePadding, MenuFontDef(nullptr, 2).fontData, MenuFontDef(nullptr, 7).fontMag, 2, 60, GridPosition::CORE_JUSTIFY_RIGHT, MenuBorder(0));
    factory.addImageToCache(DrawableIcon(icon->getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap4));

    // this is global sadly, only viable if i can constrain navigation.
    factory.setSelectedColors(RGB(255, 255, 255), RGB(255, 0, 0));

    icon->setChanged(true);

    tcgfx::ConfigurableItemDisplayPropertiesFactory::refreshCache();
}

/**
 * Reset all of the icons back to their original state, reset the selecton color value.
 */
void resetIcons()
{

    auto &factory = renderer.getGraphicsPropertiesFactory();
    const Coord iconSize(72, 72);
    
    factory.addImageToCache(DrawableIcon(menuWash.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap0, CycleIconsBitmap3));
    factory.addImageToCache(DrawableIcon(menuSpin.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap1, CycleIconsBitmap3));
    factory.addImageToCache(DrawableIcon(menuDry.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap2));
    factory.addImageToCache(DrawableIcon(menuSettings.getId(), iconSize, DrawableIcon::ICON_XBITMAP, CycleIconsBitmap3));
    MenuPadding perSidePadding(3, 3, 3, 3);

    if (stoppedButton != nullptr)
    {
        MenuPadding buttonPadding(4, 4, 4, 4);

        factory.setDrawingPropertiesForItem(ItemDisplayProperties::COMPTYPE_ACTION, stoppedButton->getId(), darkModeActionPalette, buttonPadding, MenuFontDef(nullptr, 2).fontData, MenuFontDef(nullptr, 7).fontMag, 2, 60, GridPosition::CORE_JUSTIFY_RIGHT, MenuBorder(0));
        stoppedButton = nullptr;
    }

    factory.setSelectedColors(RGB(255, 255, 255), RGB(0, 0, 0)); 

    // // Rerender entire screen
    tcgfx::ConfigurableItemDisplayPropertiesFactory::refreshCache();
}
