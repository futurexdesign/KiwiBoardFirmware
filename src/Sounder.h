/*
 * Copyright (c) 2023 Matthew Taylor
*/
//

#pragma once
#include <TaskManagerIO.h>

#define SIZE 10


class BeepHandler : public Executable

{

    public:

    void beep_activate(int);
    void status_update(int);
    void exec() override;
    void set_menuSound(bool);
    bool get_menuSound();
    BeepHandler(); // constructor declaration

    struct tone {
        // Initialize the tone array (max 10 notes)

        constexpr static int sizearray = SIZE;
        int tones[sizearray] = {0}; 

        // Tones are either 'dots' or 'dashes' with specific length of time in mS

        uint64_t dot_length, dash_length, space_length, space_length2;
        int curr_tone;
        uint64_t start_millis;
        bool sounderactive;
        bool finished;
        bool beep_activate;
    };

    private: 
    
    static bool menuSound;

};
