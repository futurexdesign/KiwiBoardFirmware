#pragma once

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * Simple serial command interface for starting motor cycles from a terminal.
 */

class MotorControl;

void serialCliInit(MotorControl *motorControl);
void serialCliTick();
