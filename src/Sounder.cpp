/*
 * Copyright (c) 2023 Matthew Taylor
*/
//

#include "Sounder.h"
#include "picoPlatform.h"

PicoPlatform *hardware;

// Called by taskManager()
BeepHandler::BeepHandler(PicoPlatform* platform) {

    this->hardware = platform;
    beepobj[0].dot_length=110;
    beepobj[0].dash_length=1000;
    beepobj[0].space_length=130;
    beepobj[0].space_length2=200;
    beepobj[0].tones[0] = {1}; // dot
    beepobj[0].tones[1] = {3}; // space
    beepobj[0].tones[2] = {1}; // dot
    beepobj[0].tones[3] = {4}; // space_longer
    beepobj[0].tones[4] = {2}; // dash
    beepobj[0].tones[5] = {0}; // END
    beepobj[0].curr_tone = 0;
    beepobj[0].start_millis = 0;
    Serial.println("Initialized Sounder tone 0");
    beepobj[0].activate_beep = false;
    beepobj[0].beep_active = false;

    beepobj[1].dot_length = 50;
    beepobj[1].dash_length = 0;
    beepobj[1].space_length = 0;
    beepobj[1].space_length2 = 0;
    beepobj[1].tones[0] = {1}; // dot
    beepobj[1].tones[1] = {0}; // End
    beepobj[1].tones[2] = {0}; //
    beepobj[1].tones[3] = {0}; //
    beepobj[1].tones[4] = {0}; //
    beepobj[1].tones[5] = {0}; //
    beepobj[1].curr_tone = 0;
    beepobj[1].start_millis = 0;
    Serial.println("Initialized Sounder tone 1");
    beepobj[1].activate_beep = false;
    beepobj[1].beep_active = false;

}

void BeepHandler::beep_activate(int tone) {

    int beep = tone;

    if(sound_enabled) {

        beepobj[beep].activate_beep = true;
        Serial.println("BeepPattern activated");

    }

}
// regularly checks on any beep tasks

void BeepHandler::exec() {

    status_update(0); // Update tone 0 (end of cycle)
    status_update(1); // Update tone 1 (button press)

};

void BeepHandler::status_update(int tone) {

    // Turn on sounder for first time
    if(beepobj[tone].activate_beep && !beepobj[tone].beep_active) {

        hardware->enableSounder(1);
        beepobj[tone].beep_active = true;
        beepobj[tone].activate_beep = false; // We've started so we don't want to start again
        beepobj[tone].start_millis = time_us_64() / 1000;

    }

    // Now to update status of sounder

    if(beepobj[tone].beep_active && !beepobj[tone].activate_beep) {

        uint64_t time = 0;

        // is current beepobj[tone] a 'dot'?
        int cur_tone  = beepobj[tone].tones[beepobj[tone].curr_tone];

        if( cur_tone == 1)
            time = beepobj[tone].dot_length;

        // is current tone a 'dash'?
        else if(cur_tone == 2)
            time = beepobj[tone].dash_length;

        // is current tone a 'space'?
        else if(cur_tone == 3)
            time = beepobj[tone].space_length;

        // is current tone a 'longer space'?
        else if(cur_tone == 4)
            time = beepobj[tone].space_length2;

        // has tone finished?
        if(((time_us_64() / 1000) - beepobj[tone].start_millis) >= time) {

            hardware->enableSounder(false);
            beepobj[tone].start_millis = time_us_64() / 1000;
            Serial.println("SOUNDER OFF");

            // get next tone
            beepobj[tone].curr_tone++;

            // stop if this was the last one
            if(beepobj[tone].tones[beepobj[tone].curr_tone] == 0) {

                beepobj[tone].curr_tone = 0;
                beepobj[tone].start_millis = 0;
                beepobj[tone].beep_active = false;
                return;

            }

            if(beepobj[tone].tones[beepobj[tone].curr_tone] < 3 ) //space
                hardware->enableSounder(true);
            else
                hardware->enableSounder(false);

        }
    }
}

void BeepHandler::setSoundEnabled(bool enable) {

    this->sound_enabled = enable;

}

boolean BeepHandler::isSoundEnabled() {
    return this->sound_enabled;
}


