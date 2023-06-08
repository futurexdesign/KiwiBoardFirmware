/*
 * Copyright (c) 2023 Matthew Taylor
*/
//

#pragma once
#include <TaskManagerIO.h>
#include "picoPlatform.h"

#define SIZE 10


class BeepHandler : public Executable

{

    public:

    void beep_activate(int, bool);
    void status_update(int);
    void exec() override;
    void set_menuSound(bool);
    bool get_menuSound();
    void set_sndLevel(int);
    
    BeepHandler(PicoPlatform *platform); // constructor declaration (receives pointer to platform object)

    struct tone {
        // Initialize the tone array (max 10 notes)

        int tones[SIZE] = {0}; 

        // Tones are either 'dots' or 'dashes' with specific length of time in mS

        uint64_t dot_length, dash_length, space_length, space_length2;
        int curr_tone;
        uint64_t start_millis;
        bool sounderactive;
        bool finished;
        bool beep_activate;
    };

    private: 
    
    bool menuSound;
    int sndLevel;
    PicoPlatform *priv_platform;

};
