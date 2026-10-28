/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/
//

#include "splashScreen.h"
#include "splashRaw.h"

/**
 * Blit the pre-expanded RGB565 splash from flash and return to startup.
 */
void showSplash() {
    bool wasSwapped = gfx.getSwapBytes();
    gfx.setSwapBytes(true);
    gfx.pushImage(0, 0, SPLASH_RAW_WIDTH, SPLASH_RAW_HEIGHT, splashRaw);
    gfx.setSwapBytes(wasSwapped);
}
