/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/


#ifndef KIWIBOARDFIRMWARE_SPLASHSCREEN_H
#define KIWIBOARDFIRMWARE_SPLASHSCREEN_H

#include <TFT_eSPI.h>
#include <PNGdec.h>

extern TFT_eSPI gfx; // populated by the linker

#define MAX_IMAGE_WIDTH 340 // Adjust for your images


void showSplash();
void pngDraw(PNGDRAW *pDraw);



#endif //KIWIBOARDFIRMWARE_SPLASHSCREEN_H
