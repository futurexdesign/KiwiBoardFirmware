#pragma once

/*
 * Shared Trinamic RMS current helpers.
 *
 * Menu GlobalScaler + IRun were designed around TMC_calculations.xlsx for the
 * TMC5160 board (Rsense = 0.075 ohm).  Both drivers should land on the same
 * target RMS current for a given menu pair.
 */

#include <stdint.h>
#include <math.h>

// Sense resistors on each Kiwi motor-driver board.
#ifndef TMC5160_RSENSE_OHM
#define TMC5160_RSENSE_OHM 0.075f
#endif
#ifndef TMC2209_RSENSE_OHM
#define TMC2209_RSENSE_OHM 0.11f
#endif

/**
 * Target run current (mA RMS) implied by the menu GlobalScaler + IRun values.
 * Matches TMC_calculations.xlsx (Vfs=0.325, no Rdson term in the sheet).
 */
inline uint16_t tmcMenuCurrentMa(uint16_t globalScaler, uint16_t iRun,
                                 float rsenseOhm = TMC5160_RSENSE_OHM) {
    uint16_t scaler = globalScaler;
    if (scaler == 0) {
        scaler = 256;
    }
    if (scaler < 32) {
        scaler = 32;
    }
    if (iRun > 31) {
        iRun = 31;
    }

    const float irms =
        ((float)(iRun + 1) / 32.0f) *
        ((float)scaler / 256.0f) *
        (0.325f / rsenseOhm) /
        1.414213562f;

    if (irms <= 0.0f) {
        return 0;
    }
    return (uint16_t)lroundf(irms * 1000.0f);
}
