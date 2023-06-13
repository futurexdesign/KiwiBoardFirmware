/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/
//

#include "EncoderShim.h"

EncoderShim::EncoderShim() {

}

void EncoderShim::initForEncoder() {

    switches.init(internalDigitalIo(), SWITCHES_NO_POLLING, true);

    switches.addSwitchListener(BUTTON, this, NO_REPEAT, false);
    setupRotaryEncoderWithInterrupt(ENC1, ENC2, this, HWACCEL_NONE, FULL_CYCLE);
}

void EncoderShim::encoderHasChanged(int newValue) {
    Serial.println("Encoder Turned");
    Serial.println(newValue);

    // Which way did the encoder go...
    bool direction;
    if (newValue < encoderValue) {
        direction = false;
    } else {
        direction = true;
    }

    encoderValue = newValue;

    menuMgr.valueChanged(newValue);
    Serial.println("Check for encoderChangeFn");
    if (encoderChangeFn != nullptr) {
        Serial.println("call encoderChangefn Callback");
        encoderChangeFn(direction, false);
    }

}

void EncoderShim::onPressed(pinid_t pin, bool held) {
    Serial.println("button down");
    if (encoderClickFn != nullptr) {
        encoderClickFn(false, held);
    }
}

void EncoderShim::onReleased(pinid_t pin, bool held) {
    Serial.println("button up");
    // always send false, since we don't want the normal tcMenu long press reset  behavior, and we are acting
    // on Pressed long press, not release.
    menuMgr.onMenuSelect(false);
}

void EncoderShim::registerChangeCallback(EncoderShimFn callback) {

    encoderChangeFn = callback;

}

void EncoderShim::registerClickCallback(EncoderShimFn callback) {

    encoderClickFn = callback;

}

/**
 * Reinitialize the encoder with a and b pins reversed.
 */
void EncoderShim::invertEncoderDirection() {
    reversed = true;
    setupRotaryEncoderWithInterrupt(ENC2, ENC1, this, HWACCEL_NONE, FULL_CYCLE);
}

/**
 * Set up as a quarter cycle encoder, be careful to not undo any encoder reversing that may of happened
 */
void EncoderShim::setQuarterCycleEncoder() {
    if (reversed) {
        setupRotaryEncoderWithInterrupt(ENC2, ENC1, this, HWACCEL_NONE, QUARTER_CYCLE);

    } else {
        setupRotaryEncoderWithInterrupt(ENC1, ENC2, this, HWACCEL_NONE, QUARTER_CYCLE);
    }


}

