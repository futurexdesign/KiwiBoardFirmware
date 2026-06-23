#pragma once

/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * Runtime debug logging via hideakitai/DebugLog.  Use LOG_DEBUG / LOG_INFO etc.
 * for optional output; CLI and errors should continue to use Serial directly.
 *
 * Named kiwiDebugLog.h (not debugLog.h) to avoid shadowing <DebugLog.h> on
 * case-insensitive filesystems.
 *
 * Compile with all log macros enabled; runtime level is set via LOG_SET_LEVEL().
 */
#define DEBUGLOG_DEFAULT_LOG_LEVEL_TRACE
#include <DebugLog.h>

void debugLogInit();
void setDebugLogging(bool enabled);
bool isDebugLogging();
