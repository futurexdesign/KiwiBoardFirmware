/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/
#pragma once
#include "KiwiBoardFirmware_menu.h"

/**
 * Structure for the various configurable settings.  These are managed and stored in EEPROM by 
 * the tcMenu backend, however, we do want to be able to pass them into the motorControl class. 
 * 
*/
struct SETTINGS
{
    int wash_duration; 
    int wash_speed;
    int wash_cycle_time;

    int spin_duration;
    int spin_speed;

    int dry_duration;
    int dry_speed;
    int cooldownTime;
    bool fanCooldown;
    
};

/**
 * Map settings out of the tcMenu variables, and into the struct fields. 
*/
SETTINGS getSettings();

void commit_if_needed();
