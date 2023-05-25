/*
    The code in this file uses open source libraries provided by thecoderscorner

    DO NOT EDIT THIS FILE, IT WILL BE GENERATED EVERY TIME YOU USE THE UI DESIGNER
    INSTEAD EITHER PUT CODE IN YOUR SKETCH OR CREATE ANOTHER SOURCE FILE.

    All the variables you may need access to are marked extern in this file for easy
    use elsewhere.
 */

#include <tcMenu.h>
#include "KiwiBoardFirmware_menu.h"
#include "ThemeDarkModeModern.h"

// Global variable declarations
const  ConnectorLocalInfo applicationInfo = { "KiwiBoard", "a44877f0-b65e-4c52-9701-aefa48df02b9" };
ArduinoEEPROMAbstraction glArduinoEeprom(&EEPROM);
TFT_eSPI gfx;
TfteSpiDrawable gfxDrawable(&gfx, 100);
GraphicsDeviceRenderer renderer(30, applicationInfo.name, &gfxDrawable);

// Global Menu Item declarations
RENDERING_CALLBACK_NAME_INVOKE(fnVersionRtCall, textItemRenderFn, "Version", -1, NO_CALLBACK)
TextMenuItem menuVersion(fnVersionRtCall, "1.00", 43, 10, NULL);
const BooleanMenuInfo minfomotorTest = { "Motor Test", 44, 0xffff, 1, motortest, NAMING_ON_OFF };
BooleanMenuItem menumotorTest(&minfomotorTest, false, &menuVersion, INFO_LOCATION_PGM);
const BooleanMenuInfo minfoStealthChop = { "StealthChop", 45, 96, 1, stealthChopChange, NAMING_ON_OFF };
BooleanMenuItem menuStealthChop(&minfoStealthChop, true, &menumotorTest, INFO_LOCATION_PGM);
const AnalogMenuInfo minfoIRun = { "IRun", 33, 77, 31, iRunChanged, 0, 1, "" };
AnalogMenuItem menuIRun(&minfoIRun, 17, &menuStealthChop, INFO_LOCATION_PGM);
const AnalogMenuInfo minfoGlobalScaler = { "Global Scaler", 32, 75, 255, GlobalScalerChanged, 0, 1, "" };
AnalogMenuItem menuGlobalScaler(&minfoGlobalScaler, 148, &menuIRun, INFO_LOCATION_PGM);
const BooleanMenuInfo minfoInvertEncoder = { "Invert Encoder", 34, 79, 1, NO_CALLBACK, NAMING_YES_NO };
BooleanMenuItem menuInvertEncoder(&minfoInvertEncoder, false, &menuGlobalScaler, INFO_LOCATION_PGM);
const SubMenuInfo minfoAdvanced = { "Advanced", 30, 0xffff, 0, NO_CALLBACK };
BackMenuItem menuBackAdvanced(&minfoAdvanced, &menuInvertEncoder, INFO_LOCATION_PGM);
SubMenuItem menuAdvanced(&minfoAdvanced, &menuBackAdvanced, NULL, INFO_LOCATION_PGM);
const AnalogMenuInfo minfoMaxPreheat = { "Preheat", 48, 94, 9, settings_changed, 1, 1, "min" };
AnalogMenuItem menuMaxPreheat(&minfoMaxPreheat, 4, NULL, INFO_LOCATION_PGM);
const AnalogMenuInfo minfocooldownTime = { "Cool Time", 19, 21, 9, settings_changed, 1, 1, "min" };
AnalogMenuItem menucooldownTime(&minfocooldownTime, 1, &menuMaxPreheat, INFO_LOCATION_PGM);
const BooleanMenuInfo minfofanCooldown = { "Cooldown", 18, 20, 1, settings_changed, NAMING_ON_OFF };
BooleanMenuItem menufanCooldown(&minfofanCooldown, true, &menucooldownTime, INFO_LOCATION_PGM);
const AnalogMenuInfo minfodry_speed = { "Speed", 17, 18, 100, settings_changed, 50, 1, "rpm" };
AnalogMenuItem menudry_speed(&minfodry_speed, 20, &menufanCooldown, INFO_LOCATION_PGM);
const AnalogMenuInfo minfodry_duration = { "Time", 16, 16, 10, settings_changed, 1, 1, "min" };
AnalogMenuItem menudry_duration(&minfodry_duration, 4, &menudry_speed, INFO_LOCATION_PGM);
const SubMenuInfo minfoDrySettings = { "Dry", 15, 0xffff, 0, NO_CALLBACK };
BackMenuItem menuBackDrySettings(&minfoDrySettings, &menudry_duration, INFO_LOCATION_PGM);
SubMenuItem menuDrySettings(&minfoDrySettings, &menuBackDrySettings, &menuAdvanced, INFO_LOCATION_PGM);
const AnalogMenuInfo minfospinAMAX = { "Accel", 40, 88, 2000, settings_changed, 500, 1, "" };
AnalogMenuItem menuspinAMAX(&minfospinAMAX, 375, NULL, INFO_LOCATION_PGM);
const AnalogMenuInfo minfospin_speed = { "Speed", 14, 14, 950, settings_changed, 50, 1, "rpm" };
AnalogMenuItem menuspin_speed(&minfospin_speed, 50, &menuspinAMAX, INFO_LOCATION_PGM);
const AnalogMenuInfo minfospin_duration = { "Time", 13, 12, 119, settings_changed, 1, 1, "sec" };
AnalogMenuItem menuspin_duration(&minfospin_duration, 44, &menuspin_speed, INFO_LOCATION_PGM);
const SubMenuInfo minfoSpinSettings = { "Spin", 12, 0xffff, 0, NO_CALLBACK };
BackMenuItem menuBackSpinSettings(&minfoSpinSettings, &menuspin_duration, INFO_LOCATION_PGM);
SubMenuItem menuSpinSettings(&minfoSpinSettings, &menuBackSpinSettings, &menuDrySettings, INFO_LOCATION_PGM);
const AnalogMenuInfo minfowashAMAX = { "Accel", 41, 90, 2000, settings_changed, 500, 1, "" };
AnalogMenuItem menuwashAMAX(&minfowashAMAX, 400, NULL, INFO_LOCATION_PGM);
const AnalogMenuInfo minfowash_speed = { "Speed", 11, 10, 350, settings_changed, 50, 1, "rpm" };
AnalogMenuItem menuwash_speed(&minfowash_speed, 175, &menuwashAMAX, INFO_LOCATION_PGM);
const AnalogMenuInfo minfoRotations = { "Rotations", 39, 86, 10, settings_changed, 1, 1, "" };
AnalogMenuItem menuRotations(&minfoRotations, 1, &menuwash_speed, INFO_LOCATION_PGM);
const AnalogMenuInfo minfowash_duration = { "Time", 9, 4, 9, settings_changed, 1, 1, "min" };
AnalogMenuItem menuwash_duration(&minfowash_duration, 4, &menuRotations, INFO_LOCATION_PGM);
const SubMenuInfo minfowashSettings = { "Wash", 8, 0xffff, 0, NO_CALLBACK };
BackMenuItem menuBackwashSettings(&minfowashSettings, &menuwash_duration, INFO_LOCATION_PGM);
SubMenuItem menuwashSettings(&minfowashSettings, &menuBackwashSettings, &menuSpinSettings, INFO_LOCATION_PGM);
const AnalogMenuInfo minfoBacklight = { "Backlight", 36, 84, 7, backlightChange, 1, 1, "" };
AnalogMenuItem menuBacklight(&minfoBacklight, 3, &menuwashSettings, INFO_LOCATION_PGM);
const SubMenuInfo minfoSettings = { "Settings", 7, 0xffff, 0, NO_CALLBACK };
BackMenuItem menuBackSettings(&minfoSettings, &menuBacklight, INFO_LOCATION_PGM);
SubMenuItem menuSettings(&minfoSettings, &menuBackSettings, NULL, INFO_LOCATION_PGM);
const AnyMenuInfo minfoDry = { "Dry", 38, 0xffff, 0, dry };
ActionMenuItem menuDry(&minfoDry, &menuSettings, INFO_LOCATION_PGM);
const AnyMenuInfo minfoSpin = { "Spin", 37, 0xffff, 0, spin };
ActionMenuItem menuSpin(&minfoSpin, &menuDry, INFO_LOCATION_PGM);
AnyMenuInfo minfoWash = { "Wash", 2, 0xffff, 0, wash };
ActionMenuItem menuWash(&minfoWash, &menuSpin, INFO_LOCATION_RAM);
RENDERING_CALLBACK_NAME_INVOKE(fnRunTimeRtCall, timeItemRenderFn, "RunTime", -1, NO_CALLBACK)
TimeFormattedMenuItem menuRunTime(fnRunTimeRtCall, TimeStorage(0, 0, 0, 0), 6, (MultiEditWireType)6, &menuWash);

void setupMenu() {
    // First we set up eeprom and authentication (if needed).
    setSizeBasedEEPROMStorageEnabled(true);
    menuMgr.setEepromRef(&glArduinoEeprom);
    // Now add any readonly, non-remote and visible flags.
    menuVersion.setReadOnly(true);
    menuspinAMAX.setStep(25);
    menuspin_speed.setStep(5);
    menuwashAMAX.setStep(25);
    menuwash_speed.setStep(5);
    menudry_speed.setStep(5);

    // Code generated by plugins.
    gfx.begin();
    gfx.setRotation(3);
    renderer.setUpdatesPerSecond(10);
    menuMgr.initWithoutInput(&renderer, &menuRunTime);
    renderer.setTitleMode(BaseGraphicalRenderer::TITLE_ALWAYS);
    renderer.setUseSliderForAnalog(false);
    installDarkModeModernTheme(renderer, MenuFontDef(nullptr, 4), MenuFontDef(nullptr, 4), false);
}

