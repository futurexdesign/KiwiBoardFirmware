/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/


#ifndef KIWIBOARDFIRMWARE_SPLASHSCREEN_H
#define KIWIBOARDFIRMWARE_SPLASHSCREEN_H

#include <TFT_eSPI.h>

extern TFT_eSPI gfx; // populated by the linker

void showSplash();

#endif //KIWIBOARDFIRMWARE_SPLASHSCREEN_H
