/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/

#include "settings.h"


SETTINGS getSettings() {

    SETTINGS rtn;
    
    rtn.wash_duration = menuwash_duration.getIntValueIncludingOffset();
    rtn.wash_cycle_time = menuwash_cycle_time.getIntValueIncludingOffset();
    rtn.wash_speed = menuwash_cycle_time.getIntValueIncludingOffset();

    rtn.spin_duration = menuspin_duration.getIntValueIncludingOffset();
    rtn.spin_speed = menuspin_speed.getIntValueIncludingOffset();
    
    rtn.cooldownTime = menucooldownTime.getIntValueIncludingOffset();
    rtn.fanCooldown = menufanCooldown.getBoolean();

    return rtn;
}

