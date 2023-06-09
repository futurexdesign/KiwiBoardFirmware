/*
 * This Screen capture routine originated from TFT_eSPI examples.  See copyright notice
*/
#include"screenServer.h"
#ifdef SCREENCAP
#warning Screen Capture Active
// Reads a screen image off the TFT and send it to a processing client sketch
// over the serial port. Use a high baud rate, e.g. for an ESP8266:
// Serial.begin(921600);

// At 921600 baud a 320 x 240 image with 16 bit colour transfers can be sent to the
// PC client in ~1.67s and 24 bit colour in ~2.5s which is close to the theoretical
// minimum transfer time.

// This sketch has been created to work with the TFT_eSPI library here:
// https://github.com/Bodmer/TFT_eSPI

// Created by: Bodmer 27/1/17
// Updated by: Bodmer 10/3/17
// Version: 0.07

// MIT licence applies, all text above must be included in derivative works

//====================================================================================
//                           Screen server call with no filename
//====================================================================================
// Start a screen dump server (serial or network) - no filename specified
boolean screenServer(void)
{
    // With no filename the screenshot will be saved with a default name e.g. tft_screen_#.xxx
    // where # is a number 0-9 and xxx is a file type specified below
    return screenServer(DEFAULT_FILENAME);
}

//====================================================================================
//                           Screen server call with filename
//====================================================================================
// Start a screen dump server (serial or network) - filename specified
boolean screenServer(String filename)
{
    Serial.end();                 // Stop the serial port (clears buffers too)
    Serial.begin(DUMP_BAUD_RATE); // Force baud rate to be high
    delay(0); // Equivalent to yield() for ESP8266;

    boolean result = serialScreenServer(filename); // Screenshot serial port server
    //boolean result = wifiScreenServer(filename);   // Screenshot WiFi UDP port server (WIP)

    Serial.end();                 // Stop the serial port (clears buffers too)
    Serial.begin(BAUD_RATE);      // Return baud rate to normal
    delay(0); // Equivalent to yield() for ESP8266;

    //Serial.println();
    //if (result) Serial.println(F("Screen dump passed :-)"));
    //else        Serial.println(F("Screen dump failed :-("));

    return result;
}

//====================================================================================
//                Serial server function that sends the data to the client
//====================================================================================
boolean serialScreenServer(String filename)
{
    // Precautionary receive buffer garbage flush for 50ms
    uint32_t clearTime = millis() + 50;
    while ( millis() < clearTime && Serial.read() >= 0) delay(0); // Equivalent to yield() for ESP8266;

    boolean wait = true;
    uint32_t lastCmdTime = millis();     // Initialise start of command time-out

    // Wait for the starting flag with a start time-out
    while (wait)
    {
        delay(0); // Equivalent to yield() for ESP8266;
        // Check serial buffer
        if (Serial.available() > 0) {
            // Read the command byte
            uint8_t cmd = Serial.read();
            // If it is 'S' (start command) then clear the serial buffer for 100ms and stop waiting
            if ( cmd == 'S' ) {
                // Precautionary receive buffer garbage flush for 50ms
                clearTime = millis() + 50;
                while ( millis() < clearTime && Serial.read() >= 0) delay(0); // Equivalent to yield() for ESP8266;

                wait = false;           // No need to wait anymore
                lastCmdTime = millis(); // Set last received command time

                // Send screen size etc using a simple header with delimiters for client checks
                sendParameters(filename);
            }
        }
        else
        {
            // Check for time-out
            if ( millis() > lastCmdTime + START_TIMEOUT) return false;
        }
    }

    uint8_t color[3 * NPIXELS]; // RGB and 565 format color buffer for N pixels

    // Send all the pixels on the whole screen
    for ( uint32_t y = 0; y < gfx.height(); y++)
    {
        // Increment x by NPIXELS as we send NPIXELS for every byte received
        for ( uint32_t x = 0; x < gfx.width(); x += NPIXELS)
        {
            delay(0); // Equivalent to yield() for ESP8266;

            // Wait here for serial data to arrive or a time-out elapses
            while ( Serial.available() == 0 )
            {
                if ( millis() > lastCmdTime + PIXEL_TIMEOUT) return false;
                delay(0); // Equivalent to yield() for ESP8266;
            }

            // Serial data must be available to get here, read 1 byte and
            // respond with N pixels, i.e. N x 3 RGB bytes or N x 2 565 format bytes
            if ( Serial.read() == 'X' ) {
                // X command byte means abort, so clear the buffer and return
                clearTime = millis() + 50;
                while ( millis() < clearTime && Serial.read() >= 0) delay(0); // Equivalent to yield() for ESP8266;
                return false;
            }
            // Save arrival time of the read command (for later time-out check)
            lastCmdTime = millis();

#if defined BITS_PER_PIXEL && BITS_PER_PIXEL >= 24
            // Fetch N RGB pixels from x,y and put in buffer
      tft.readRectRGB(x, y, NPIXELS, 1, color);
      // Send buffer to client
      Serial.write(color, 3 * NPIXELS); // Write all pixels in the buffer
#else
            // Fetch N 565 format pixels from x,y and put in buffer
            gfx.readRect(x, y, NPIXELS, 1, (uint16_t *)color);
            // Send buffer to client
            Serial.write(color, 2 * NPIXELS); // Write all pixels in the buffer
#endif
        }
    }

    Serial.flush(); // Make sure all pixel bytes have been despatched

    return true;
}

//====================================================================================
//    Send screen size etc using a simple header with delimiters for client checks
//====================================================================================
void sendParameters(String filename)
{
    Serial.write('W'); // Width
    Serial.write(gfx.width()  >> 8);
    Serial.write(gfx.width()  & 0xFF);

    Serial.write('H'); // Height
    Serial.write(gfx.height() >> 8);
    Serial.write(gfx.height() & 0xFF);

    Serial.write('Y'); // Bits per pixel (16 or 24)
    Serial.write(BITS_PER_PIXEL);

    Serial.write('?'); // Filename next
    Serial.print(filename);

    Serial.write('.'); // End of filename marker

    Serial.write(FILE_EXT); // Filename extension identifier

    Serial.write(*FILE_TYPE); // First character defines file type j,b,p,t
}
#endif
