/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/
//

#include "EncoderShim.h"
#include "kiwiDebugLog.h"

EncoderShim::EncoderShim() {

}

void EncoderShim::initForEncoder(uint8_t button, uint8_t enc1, uint8_t enc2) {
    button_pin = button;
    enc1_pin = enc1;
    enc2_pin = enc2;

    switches.init(internalDigitalIo(), SWITCHES_NO_POLLING, true);

    switches.addSwitchListener(button_pin, this, NO_REPEAT, false);
    // HwStateRotaryEncoder is more tolerant of delayed ISR handling than the
    // older edge counter — important on RP2350 where the 20 ms motor tick + UART
    // can postpone TaskManager encoder processing.
    setupStateMachineRotaryEncoder(enc1_pin, enc2_pin, this, HWACCEL_NONE, FULL_CYCLE);
}

void EncoderShim::encoderHasChanged(int newValue) {
    LOG_DEBUG("Encoder Turned");
    LOG_DEBUG("Encoder value:", newValue);

    // Which way did the encoder go...
    bool direction;
    if (newValue < encoderValue) {
        direction = false;
    } else {
        direction = true;
    }

    encoderValue = newValue;

    menuMgr.valueChanged(newValue);
    LOG_TRACE("Check for encoderChangeFn");
    if (encoderChangeFn != nullptr) {
        LOG_TRACE("call encoderChangefn Callback");
        encoderChangeFn(direction, false);
    }

}

void EncoderShim::onPressed(pinid_t pin, bool held) {
    LOG_TRACE("button down");
    if (encoderClickFn != nullptr) {
        encoderClickFn(false, held);
    }
}

void EncoderShim::onReleased(pinid_t pin, bool held) {
    LOG_TRACE("button up");
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
    setupStateMachineRotaryEncoder(enc2_pin, enc1_pin, this, HWACCEL_NONE, FULL_CYCLE);
}

/**
 * Set up as a quarter cycle encoder, be careful to not undo any encoder reversing that may of happened
 */
void EncoderShim::setQuarterCycleEncoder() {
    if (reversed) {
        setupStateMachineRotaryEncoder(enc2_pin, enc1_pin, this, HWACCEL_NONE, QUARTER_CYCLE);

    } else {
        setupStateMachineRotaryEncoder(enc1_pin, enc2_pin, this, HWACCEL_NONE, QUARTER_CYCLE);
    }


}

/**
 * Set up as a half cycle encoder, be careful to not undo any encoder reversing that may of happened
 */
void EncoderShim::setHalfCycleEncoder() {
    if (reversed) {
        setupStateMachineRotaryEncoder(enc2_pin, enc1_pin, this, HWACCEL_NONE, HALF_CYCLE);

    } else {
        setupStateMachineRotaryEncoder(enc1_pin, enc2_pin, this, HWACCEL_NONE, HALF_CYCLE);
    }


}
