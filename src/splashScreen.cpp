/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/
//

#include "splashScreen.h"
#include "splash.h" // The compressed splash-screen in PROGMEM
#include <PNGdec.h>

PNG png;
int16_t xPos = 0;
int16_t yPos = 0;


/**
 * Decompress the splash screen, which
 * is stored as a PNG file.  Once decoded, display to the screen, and return
 * to the rest of the startup routine.
 *
 */
void showSplash() {

    int16_t rc = png.openFLASH((uint8_t *)splash, sizeof(splash), pngDraw);
    if (rc == PNG_SUCCESS)
    {
        gfx.startWrite();
        uint32_t dt = millis();
        rc = png.decode(NULL, 0);
        gfx.endWrite();
    }
}

/**
 * Once the png is decompressed, this callback is triggered.  Actually display the PNG to the screen.
 * @param pDraw
 */
void pngDraw(PNGDRAW *pDraw)
{
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    static uint16_t dmaBuffer[MAX_IMAGE_WIDTH]; // static so buffer persists after fn exit

    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    gfx.pushImageDMA(0, 0 + pDraw->y, pDraw->iWidth, 1, lineBuffer, dmaBuffer);
}