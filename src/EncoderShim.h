/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * Create a shim that will sit in between the tcMenu layer, and the firmware.  This will allow us to monitor the rotary
 * encoder on every cycle to pick up on movement events, and fire any callback events.
*/
//

#include <tcMenu.h>
#include <stdint.h>

#ifndef KIWIBOARDFIRMWARE_ENCODERSHIM_H
#define KIWIBOARDFIRMWARE_ENCODERSHIM_H


/** The definition of a callback from EncoderShim object */
typedef void (*EncoderShimFn)(bool direction, bool longPress);

class EncoderShim : EncoderListener, SwitchListener {

    public:
        EncoderShim();

        /**
         * Initialize the encoder and button for the provided GPIO pins. This must be called
         * before setupMenu.
         */
        void initForEncoder(uint8_t button, uint8_t enc1, uint8_t enc2);

        /**
         * Register the function that should be called when the encoder value changes.
         */
        void registerChangeCallback(EncoderShimFn callback);

        /**
         * Register the function that should be called when the encoder button has been pressed.
         */
        void registerClickCallback(EncoderShimFn callback);

        /**
         * Swap the two encoder pins, this will invert the direction of the encoder in all modes.
         */
        void invertEncoderDirection();

        void setQuarterCycleEncoder();

        void setHalfCycleEncoder();

    private:
        void encoderHasChanged(int newValue) override;

        void onPressed(pinid_t pin, bool held) override;

        void onReleased(pinid_t pin, bool held) override;

        // Callback Functions for click and change
        EncoderShimFn encoderChangeFn = nullptr;
        EncoderShimFn encoderClickFn = nullptr;

        uint8_t button_pin = 0;
        uint8_t enc1_pin = 0;
        uint8_t enc2_pin = 0;
        bool reversed = false;

        int encoderValue = 0;

};


#endif //KIWIBOARDFIRMWARE_ENCODERSHIM_H
