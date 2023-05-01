#ifndef KIWIBOARDFIRMWARE_SCREENSERVER_H
#define KIWIBOARDFIRMWARE_SCREENSERVER_H
#ifdef SCREENCAP

#include<Arduino.h>
#include"KiwiBoardFirmware_menu.h"


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
//                                  Definitions
//====================================================================================
#define BAUD_RATE 250000      // Maximum Serial Monitor rate for other messages
#define DUMP_BAUD_RATE 921600 // Rate used for screen dumps

#define PIXEL_TIMEOUT 100     // 100ms Time-out between pixel requests
#define START_TIMEOUT 10000   // 10s Maximum time to wait at start transfer

#define BITS_PER_PIXEL 16     // 24 for RGB colour format, 16 for 565 colour format

// File names must be alpha-numeric characters (0-9, a-z, A-Z) or "/" underscore "_"
// other ascii characters are stripped out by client, including / generates
// sub-directories
#define DEFAULT_FILENAME "tft_screenshots/screenshot" // In case none is specified
#define FILE_TYPE "png"       // jpg, bmp, png, tif are valid

// Filename extension
// '#' = add 0-9, '@' = add timestamp, '%' add millis() timestamp, '*' = add nothing
// '@' and '%' will generate new unique filenames, so beware of cluttering up your
// hard drive with lots of images! The PC client sketch is set to limit the number of
// saved images to 1000 and will then prompt for a restart.
#define FILE_EXT  '%'

// Number of pixels to send in a burst (minimum of 1), no benefit above 8
// NPIXELS values and render times: 1 = 5.0s, 2 = 1.75s, 4 = 1.68s, 8 = 1.67s
#define NPIXELS 8  // Must be integer division of both TFT width and TFT height

//====================================================================================
//                           Screen server call with no filename
//====================================================================================
// Start a screen dump server (serial or network) - no filename specified
boolean screenServer(void);
//====================================================================================
//                           Screen server call with filename
//====================================================================================
// Start a screen dump server (serial or network) - filename specified
boolean screenServer(String filename);

//====================================================================================
//                Serial server function that sends the data to the client
//====================================================================================
boolean serialScreenServer(String filename);
//====================================================================================
//    Send screen size etc using a simple header with delimiters for client checks
//====================================================================================
void sendParameters(String filename);
#endif
#endif //KIWIBOARDFIRMWARE_SCREENSERVER_H
