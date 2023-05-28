/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/

#include "settings.h"

SETTINGS getSettings() {

    SETTINGS rtn{};
    
    rtn.wash_duration = menuwash_duration.getIntValueIncludingOffset();
    rtn.washRotations = menuRotations.getIntValueIncludingOffset();
    rtn.wash_speed = menuwash_speed.getIntValueIncludingOffset();
    rtn.wash_amax = menuwashAMAX.getIntValueIncludingOffset();

    rtn.spin_duration = menuspin_duration.getIntValueIncludingOffset();
    rtn.spin_speed = menuspin_speed.getIntValueIncludingOffset();
    rtn.spin_amax = menuspinAMAX.getIntValueIncludingOffset();

    rtn.dry_duration = menudry_duration.getIntValueIncludingOffset();
    rtn.dry_speed = menudry_speed.getIntValueIncludingOffset();
    rtn.cooldownTime = menucooldownTime.getIntValueIncludingOffset();
    rtn.fanCooldown = menufanCooldown.getBoolean();

    rtn.preheatTime = menuMaxPreheat.getIntValueIncludingOffset();
  //  rtn.sounder = menusounder.getIntValueIncOffsett();

    return rtn;
}

