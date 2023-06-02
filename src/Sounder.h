/*
 * Copyright (c) 2023 Matthew Taylor
*/
//

#pragma once

#include <TaskManagerIO.h>
#include <Arduino.h>
#include "picoPlatform.h"

#define MAX_TONE_SIZE 10

struct BeepPattern {// Initialize the tone array (max 10 notes)

    int tones[MAX_TONE_SIZE] = {0};

    // Tones are either 'dots' or 'dashes' with specific length of time in mS
    uint64_t dot_length, dash_length, space_length, space_length2;
    uint64_t start_millis;
    int curr_tone;
    bool beep_active = false;
    bool finished = false;
    bool activate_beep = false;

};

class BeepHandler : public Executable {

    public:

        BeepHandler();

        BeepHandler(PicoPlatform *platform);

        void beep_activate(int);

        void status_update(int);

        void exec() override;

        void setSoundEnabled(bool enable);

        boolean isSoundEnabled();

    private:
        bool sound_enabled = false;
        BeepPattern beepobj[2];
        PicoPlatform* hardware;


};
