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
#include "tcMenuU8g2.h"
#include <RuntimeMenuItem.h>
#include <IoAbstraction.h>
#include <EepromItemStorage.h>
#include <ArduinoEEPROMAbstraction.h>
#include "picoPlatform.h"

// variables we declare that you may need to access
extern const PROGMEM ConnectorLocalInfo applicationInfo;
//extern U8G2_SH1107_128X128_F_HW_I2C gfx;
extern U8G2_SH1107_PIMORONI_128X128_F_HW_I2C gfx;
extern GraphicsDeviceRenderer renderer;

// Any externals needed by IO expanders, EEPROMs etc


// Global Menu Item exports
extern AnalogMenuItem menucooldownTime;
extern BooleanMenuItem menufanCooldown;
extern AnalogMenuItem menudry_speed;
extern AnalogMenuItem menudry_duration;
extern BackMenuItem menuBackDrySettings;
extern SubMenuItem menuDrySettings;
extern AnalogMenuItem menuspin_speed;
extern AnalogMenuItem menuspin_duration;
extern BackMenuItem menuBackSpinSettings;
extern SubMenuItem menuSpinSettings;
extern AnalogMenuItem menuwash_speed;
extern AnalogMenuItem menuwash_cycle_time;
extern AnalogMenuItem menuwash_duration;
extern BackMenuItem menuBackwashSettings;
extern SubMenuItem menuwashSettings;
extern BackMenuItem menuBackSettings;
extern SubMenuItem menuSettings;
extern TimeFormattedMenuItem menuRunTime;
extern AnyMenuInfo minfoRunStop;
extern ActionMenuItem menuRunStop;
extern EnumMenuItem menuProgram;

// Provide a wrapper to get hold of the root menu item and export setupMenu
inline MenuItem& rootMenuItem() { return menuProgram; }
void setupMenu();

// Callback functions must always include CALLBACK_FUNCTION after the return type
#define CALLBACK_FUNCTION

void CALLBACK_FUNCTION progChange(int id);
void CALLBACK_FUNCTION run(int id);
void CALLBACK_FUNCTION settings_changed(int id);

#endif // MENU_GENERATED_CODE_H
