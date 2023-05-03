/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * Create a shim that will sit in between the tcMenu layer, and the firmware.  This will allow us to monitor the rotary
 * encoder on every cycle to pick up on movement events, and fire any callback events.
*/
//

#include "picoPlatform.h" // Need the platform pin definitions
#include <tcMenu.h>

#ifndef KIWIBOARDFIRMWARE_ENCODERSHIM_H

#define KIWIBOARDFIRMWARE_ENCODERSHIM_H


/** The definition of a callback from EncoderShim object */
typedef void (*EncoderShimFn)(bool direction, bool longPress);

class EncoderShim : EncoderListener, SwitchListener {

    public:
        EncoderShim(MenuManager* manager);

        void initForEncoder();
        void registerChangeCallback(EncoderShimFn callback);
        void registerClickCallback(EncoderShimFn callback);
        void invertEncoderDirection();

    private:
        void encoderHasChanged(int newValue) override;

        void onPressed(pinid_t pin, bool held) override;

        void onReleased(pinid_t pin, bool held) override;

        MenuManager* menuManager;
        EncoderShimFn encoderChangeFn = nullptr;
        EncoderShimFn encoderClickFn = nullptr;

        int encoderValue = 0;// last known encoder value

};


#endif //KIWIBOARDFIRMWARE_ENCODERSHIM_H
