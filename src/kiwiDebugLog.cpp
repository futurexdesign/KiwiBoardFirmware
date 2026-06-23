/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include <Arduino.h>
#include "kiwiDebugLog.h"

void debugLogInit() {
    LOG_ATTACH_SERIAL(Serial);
    LOG_SET_LEVEL(DebugLogLevel::LVL_NONE);
}

void setDebugLogging(bool enabled) {
    LOG_SET_LEVEL(enabled ? DebugLogLevel::LVL_TRACE : DebugLogLevel::LVL_NONE);
}

bool isDebugLogging() {
    return LOG_GET_LEVEL() != DebugLogLevel::LVL_NONE;
}
