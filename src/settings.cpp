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

void commit_if_needed() {
//    if (settingsChanged)
//    {
//        Serial.println("Commit settings to Flash");
//        EEPROM.commit();
//        settingsChanged = false;
//
//        BaseDialog *dlg = renderer.getDialog();
//        if (dlg)
//        {
//            dlg->setButtons(BTNTYPE_NONE, BTNTYPE_CLOSE);
//            dlg->show(allSavedPgm, false); // false = shows only on device
//            dlg->copyIntoBuffer("Committed to FLASH");
//        }
//    }
}