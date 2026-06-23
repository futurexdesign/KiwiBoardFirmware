/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "serialCli.h"
#include "motorControl.h"
#include "settings.h"
#include "kiwiDebugLog.h"
#include <Arduino.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static MotorControl *cli_motor = nullptr;
static char line_buffer[80];
static uint8_t line_pos = 0;

static void printHelp() {
    Serial.println(F("CLI commands:"));
    Serial.println(F("  wash <rpm> <accel>  Start wash (accel = full steps/s^2)"));
    Serial.println(F("  spin <rpm> <accel>  Start spin (accel = full steps/s^2)"));
    Serial.println(F("  stealth on|off      Enable/disable StealthChop (runtime)"));
    Serial.println(F("  stop                Stop the current cycle"));
    Serial.println(F("  status              Show motor state"));
    Serial.println(F("  status tmc          Show full TMC driver diagnostics"));
    Serial.println(F("  debug on|off        Enable/disable debug logging"));
    Serial.println(F("  help                Show this help"));
}

static void trimInPlace(char *text) {
    while (*text == ' ' || *text == '\t') {
        text++;
    }

    char *end = text + strlen(text);
    while (end > text && (end[-1] == ' ' || end[-1] == '\t')) {
        end--;
    }
    *end = '\0';
}

static bool equalsIgnoreCase(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static void printStatus() {
    if (cli_motor == nullptr) {
        Serial.println(F("CLI: motor control not ready"));
        return;
    }

    Serial.print(F("running: "));
    Serial.println(cli_motor->isRunning() ? F("yes") : F("no"));
    Serial.print(F("stealthChop: "));
    Serial.println(cli_motor->isStealthChopEnabled() ? F("on") : F("off"));
    Serial.print(F("debug: "));
    Serial.println(isDebugLogging() ? F("on") : F("off"));

    if (!cli_motor->isRunning()) {
        return;
    }

    Serial.print(F("program: "));
    Serial.println(cli_motor->getRunningProgram());
    Serial.print(F("rpm: "));
    Serial.println(cli_motor->getMotorSpeed());
    Serial.print(F("seconds remaining: "));
    Serial.println(cli_motor->getSecondsRemaining());
}

static void startCycle(int programId, int speedRpm, int accel) {
    if (cli_motor == nullptr) {
        Serial.println(F("CLI: motor control not ready"));
        return;
    }

    if (cli_motor->isRunning()) {
        Serial.println(F("CLI: motor busy, send 'stop' first"));
        return;
    }

    if (speedRpm <= 0 || accel <= 0) {
        Serial.println(F("CLI: speed and accel must be positive"));
        return;
    }

    SETTINGS settings = getSettings();

    if (programId == 0) {
        settings.wash_speed = speedRpm;
        settings.wash_amax = accel;
        Serial.print(F("CLI: starting wash, rpm="));
        Serial.print(speedRpm);
        Serial.print(F(", amax="));
        Serial.println(accel);
    } else if (programId == 1) {
        settings.spin_speed = speedRpm;
        settings.spin_amax = accel;
        Serial.print(F("CLI: starting spin, rpm="));
        Serial.print(speedRpm);
        Serial.print(F(", amax="));
        Serial.println(accel);
    } else {
        Serial.println(F("CLI: unsupported program"));
        return;
    }

    cli_motor->startProgram(programId, settings);
}

static void setStealthChop(bool enabled) {
    if (cli_motor == nullptr) {
        Serial.println(F("CLI: motor control not ready"));
        return;
    }

    cli_motor->setStealthChop(enabled);
    Serial.print(F("CLI: stealthChop "));
    Serial.print(enabled ? F("on") : F("off"));
    Serial.println(F(" (runtime only)"));
}

static void setDebugLoggingCli(bool enabled) {
    setDebugLogging(enabled);
    Serial.print(F("CLI: debug logging "));
    Serial.print(enabled ? F("on") : F("off"));
    Serial.println();
}

static void handleCommand(char *line) {
    trimInPlace(line);

    if (line[0] == '\0') {
        return;
    }

    if (equalsIgnoreCase(line, "help") || equalsIgnoreCase(line, "?")) {
        printHelp();
        return;
    }

    char cmd[16] = {0};
    char arg[16] = {0};
    if (sscanf(line, "%15s %15s", cmd, arg) >= 1 && equalsIgnoreCase(cmd, "status")) {
        if (arg[0] == '\0') {
            printStatus();
        } else if (equalsIgnoreCase(arg, "tmc")) {
            if (cli_motor == nullptr) {
                Serial.println(F("CLI: motor control not ready"));
                return;
            }
            cli_motor->printDriveStatus();
        } else {
            Serial.println(F("CLI: usage: status [tmc]"));
        }
        return;
    }

    if (equalsIgnoreCase(line, "stop")) {
        if (cli_motor == nullptr) {
            Serial.println(F("CLI: motor control not ready"));
            return;
        }

        if (!cli_motor->isRunning()) {
            Serial.println(F("CLI: not running"));
            return;
        }

        cli_motor->stopMotion();
        Serial.println(F("CLI: stop requested"));
        return;
    }

    int speedRpm = 0;
    int accel = 0;

    if (sscanf(line, "%15s %15s", cmd, arg) == 2) {
        if (equalsIgnoreCase(cmd, "stealth")) {
            if (equalsIgnoreCase(arg, "on")) {
                setStealthChop(true);
            } else if (equalsIgnoreCase(arg, "off")) {
                setStealthChop(false);
            } else {
                Serial.println(F("CLI: usage: stealth on|off"));
            }
            return;
        }

        if (equalsIgnoreCase(cmd, "debug")) {
            if (equalsIgnoreCase(arg, "on")) {
                setDebugLoggingCli(true);
            } else if (equalsIgnoreCase(arg, "off")) {
                setDebugLoggingCli(false);
            } else {
                Serial.println(F("CLI: usage: debug on|off"));
            }
            return;
        }
    }

    if (sscanf(line, "%15s %d %d", cmd, &speedRpm, &accel) == 3) {
        if (equalsIgnoreCase(cmd, "wash")) {
            startCycle(0, speedRpm, accel);
        } else if (equalsIgnoreCase(cmd, "spin")) {
            startCycle(1, speedRpm, accel);
        } else {
            Serial.println(F("CLI: unknown command (try 'help')"));
        }
        return;
    }

    Serial.println(F("CLI: unknown command (try 'help')"));
}

void serialCliInit(MotorControl *motorControl) {
    cli_motor = motorControl;
    line_pos = 0;
    line_buffer[0] = '\0';

    Serial.println();
    Serial.println(F("Serial CLI ready. Type 'help' for commands."));
    Serial.print(F("> "));
}

void serialCliTick() {
    while (Serial.available() > 0) {
        const char c = (char)Serial.read();

        if (c == '\r') {
            continue;
        }

        if (c == '\b' || c == 0x7f) {
            if (line_pos > 0) {
                line_pos--;
                Serial.print(F("\b \b"));
            }
            continue;
        }

        if (c == '\n') {
            Serial.println();
            line_buffer[line_pos] = '\0';
            line_pos = 0;
            handleCommand(line_buffer);
            Serial.print(F("> "));
            continue;
        }

        if (line_pos < (sizeof(line_buffer) - 1)) {
            line_buffer[line_pos++] = c;
            Serial.write(c);
        }
    }
}
