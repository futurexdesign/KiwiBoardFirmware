#pragma once

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * Single-pin half-duplex UART helper for TMC2209 PDN_UART.
 * TX uses PIO (any GPIO); RX uses hardware UART0 on pins that support it
 * (including TMC_SS / GPIO13).
 */

#include <Arduino.h>

#if defined(TMC2209_UART_HALFDUPLEX)

Stream &tmcHalfDuplexStream();
void tmcHalfDuplexBegin(unsigned long baud = 115200);
void tmcHalfDuplexEnd();
void tmcHalfDuplexReleaseBus();

#endif
