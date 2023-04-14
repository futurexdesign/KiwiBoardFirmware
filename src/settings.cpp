/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/

#include "settings.h"

SETTINGS getSettings() {

    SETTINGS rtn{};
    
    rtn.wash_duration = menuwash_duration.getIntValueIncludingOffset();
    rtn.wash_cycle_time = menuwash_cycle_time.getIntValueIncludingOffset();
    rtn.wash_speed = menuwash_cycle_time.getIntValueIncludingOffset();
    rtn.wash_amax = menuwashAMAX.getLargeNumber()->getWhole();
    rtn.wash_vmax = menuwashVMAX.getLargeNumber()->getWhole();
    rtn.wash_pos = menuwashPos.getLargeNumber()->getWhole();


    rtn.spin_duration = menuspin_duration.getIntValueIncludingOffset();
    rtn.spin_speed = menuspin_speed.getIntValueIncludingOffset();
    rtn.spin_amax = menuspinAMAX.getLargeNumber()->getWhole();
    rtn.spin_vmax = menuspinVMAX.getLargeNumber()->getWhole();
    
    rtn.dry_duration = menudry_duration.getIntValueIncludingOffset();
    rtn.dry_speed = menudry_speed.getIntValueIncludingOffset();
    rtn.cooldownTime = menucooldownTime.getIntValueIncludingOffset();
    rtn.fanCooldown = menufanCooldown.getBoolean();

    return rtn;
}

