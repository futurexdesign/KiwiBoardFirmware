/*
    The code in this file uses open source libraries provided by thecoderscorner

    DO NOT EDIT THIS FILE, IT WILL BE GENERATED EVERY TIME YOU USE THE UI DESIGNER
    INSTEAD EITHER PUT CODE IN YOUR SKETCH OR CREATE ANOTHER SOURCE FILE.

    All the variables you may need access to are marked extern in this file for easy
    use elsewhere.
 */

#ifndef MENU_GENERATED_CODE_H
#define MENU_GENERATED_CODE_H

#include <Arduino.h>
#include <tcMenu.h>
#include "tcMenuTfteSpi.h"
#include <RuntimeMenuItem.h>
#include <IoAbstraction.h>
#include <EepromItemStorage.h>
#include <ArduinoEEPROMAbstraction.h>
#include "picoPlatform.h"

// variables we declare that you may need to access
extern const PROGMEM ConnectorLocalInfo applicationInfo;
extern TFT_eSPI gfx;
extern TfteSpiDrawable gfxDrawable;
extern GraphicsDeviceRenderer renderer;

// Any externals needed by IO expanders, EEPROMs etc


// Global Menu Item exports
extern AnalogMenuItem menuIRun;
extern AnalogMenuItem menuGlobalScaler;
extern BooleanMenuItem menuInvertEncoder;
extern BackMenuItem menuBackAdvanced;
extern SubMenuItem menuAdvanced;
extern AnalogMenuItem menucooldownTime;
extern BooleanMenuItem menufanCooldown;
extern AnalogMenuItem menudry_speed;
extern AnalogMenuItem menudry_duration;
extern BackMenuItem menuBackDrySettings;
extern SubMenuItem menuDrySettings;
extern AnalogMenuItem menuspinAMAX;
extern AnalogMenuItem menuspin_speed;
extern AnalogMenuItem menuspin_duration;
extern BackMenuItem menuBackSpinSettings;
extern SubMenuItem menuSpinSettings;
extern AnalogMenuItem menuwashAMAX;
extern AnalogMenuItem menuwash_speed;
extern AnalogMenuItem menuRotations;
extern AnalogMenuItem menuwash_duration;
extern BackMenuItem menuBackwashSettings;
extern SubMenuItem menuwashSettings;
extern AnalogMenuItem menuBacklight;
extern BackMenuItem menuBackSettings;
extern SubMenuItem menuSettings;
extern ActionMenuItem menuDry;
extern ActionMenuItem menuSpin;
extern AnyMenuInfo minfoWash;
extern ActionMenuItem menuWash;
extern TimeFormattedMenuItem menuRunTime;

// Provide a wrapper to get hold of the root menu item and export setupMenu
inline MenuItem& rootMenuItem() { return menuRunTime; }
void setupMenu();

// Callback functions must always include CALLBACK_FUNCTION after the return type
#define CALLBACK_FUNCTION

void CALLBACK_FUNCTION GlobalScalerChanged(int id);
void CALLBACK_FUNCTION backlightChange(int id);
void CALLBACK_FUNCTION dry(int id);
void CALLBACK_FUNCTION iRunChanged(int id);
void CALLBACK_FUNCTION settings_changed(int id);
void CALLBACK_FUNCTION spin(int id);
void CALLBACK_FUNCTION wash(int id);

#endif // MENU_GENERATED_CODE_H
