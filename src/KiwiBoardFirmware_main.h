/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * Callback tasks for the main process (ui related)
*/

#include <TMC5160.h>

#ifndef KIWIBOARDFIRMWARE_KIWIBOARDFIRMWARE_MAIN_H
#define KIWIBOARDFIRMWARE_KIWIBOARDFIRMWARE_MAIN_H

const color_t darkModeActionPalette[] = {RGB(255, 255, 255), RGB(0,0,0), RGB(20,45,110), RGB(255,255,255)};


// Callback Definitions.
/**
 * Primary UI related task.  When called, we update menu items that we want tcMenu to redraw, or otherwise update.
 * This is NOT for raw drawing on the TFT, We'll need to do that separately with a drawing delegate
 */
void ui_tick();
void motorErrorDialog(TMC5160::DriverStatus);
void titleBarClick(int id);

/**
 * Task to be called periodically that will check to see if the screen capture pin is pulled high.
 * If it is, start the TFT_eSPI provided Screen Capture function.
 *
 * NOTE: When screen capture is active, it blocks the entire application until it exits.
 */
void screenCaptureTask();

/**
 * Setup the task schedule
 */
void scheduleTasks();

/**
 * Setup all of the various tcMenu expanded options, like setting the home screen to the grid of icons, color settings
 * etc, anything we can't do from the designer ui.
 */
void setMenuOptions();

/**
 * Launch the requested program.
*/
void run(int program);


void setIconStopped(MenuItem* icon); 

void resetIcons();

void renderTimer(unsigned int encoderValue, RenderPressMode clicked);


#endif //KIWIBOARDFIRMWARE_KIWIBOARDFIRMWARE_MAIN_H
