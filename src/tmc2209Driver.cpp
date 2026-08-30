/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "tmc2209Driver.h"
#include "picoPlatform.h"
#include "kiwiDebugLog.h"
#include <Arduino.h>

// TMC2209 internal clock: v[usteps/s] = VACTUAL * (fCLK / 2^24)
#define TMC2209_FCLK_HZ 12000000.0f
#define TMC2209_VACTUAL_SCALE (TMC2209_FCLK_HZ / 16777216.0f)

TMC2209Driver::~TMC2209Driver() {
    delete motor;
    motor = nullptr;
}

int32_t TMC2209Driver::vmaxToVactual(float vmax) const {
    // vmax is in full steps per second; VACTUAL is in microsteps per clock tick.
    const float microstepsPerSec = vmax * (float)TMC2209_MICROSTEPS;
    return (int32_t)lroundf(microstepsPerSec / TMC2209_VACTUAL_SCALE);
}

void TMC2209Driver::applyVelocity(float vmax) {
    const int32_t vactual = vmaxToVactual(vmax);
    if (vactual != last_vactual) {
        // TMC2209 VACTUAL: write zero when reversing sign to avoid a brief stall.
        if (last_vactual != 0 && vactual != 0 &&
            ((last_vactual > 0 && vactual < 0) || (last_vactual < 0 && vactual > 0))) {
            motor->VACTUAL(0);
            last_vactual = 0;
        }
        motor->VACTUAL((uint32_t)vactual);
        last_vactual = vactual;
    }
}

void TMC2209Driver::applyStealthChop(bool enabled) {
    stealth_chop = enabled;
    if (enabled) {
        motor->en_spreadCycle(false);
        motor->pwm_autoscale(true);
    } else {
        motor->en_spreadCycle(true);
    }
}

bool TMC2209Driver::init(PicoPlatform *curPlatform, const MOTOR_DRIVER_CONFIG &config) {
    this->platform = curPlatform;

    // start motor controller. wants to be enabled to talk to it..
    platform->enableMotor(true);

    if (motor == nullptr) {
        motor = new TMC2209Stepper(&Serial1, TMC2209_RSENSE, TMC2209_DRIVER_ADDRESS);
    }

    Serial.print(F("TMC2209 UART: TX GPIO"));
    Serial.print(TMC_UART_TX);
    Serial.print(F(" / RX GPIO"));
    Serial.println(TMC_UART_RX);
    Serial1.setTX(TMC_UART_TX);
    Serial1.setRX(TMC_UART_RX);
    Serial1.begin(115200);
    delay(100);

    motor->begin();
    motor->pdn_disable(true);
    motor->mstep_reg_select(true);
    motor->toff(4);
    motor->blank_time(24);
    motor->I_scale_analog(false);
    motor->internal_Rsense(false);
    motor->microsteps(TMC2209_MICROSTEPS);
    motor->TPWMTHRS(0);
    motor->semin(0);
    motor->TPOWERDOWN(20);
    motor->iholddelay(2);

    uint8_t irun = config.iRun;
    if (irun > 31) {
        irun = 31;
    }
    if (irun < 1) {
        irun = 1;
    }

    // IHOLD_IRUN.irun is the 5-bit current scale (CS, 0-31) from the datasheet.
    // Same field as TMC5160 irun; TMC2209 has no GLOBAL_SCALER register.
    motor->irun(irun);
    motor->ihold(irun / 2);
    motor->vsense(irun < 16);

    if (config.stealthChop) {
        applyStealthChop(true);
    } else {
        applyStealthChop(false);
    }

    motor->push();

    motor->VACTUAL(0);
    last_vactual = 0;
    planner.reset();
    last_tick_ms = millis();

    const uint8_t version = motor->version();
    if (version != 0x21) {
        LOG_WARN("TMC2209 version mismatch");
    }

    printDriveStatus();

    platform->enableMotor(false);
    return true;
}

void TMC2209Driver::stop() {
    planner.requestStop();
    applyVelocity(0.0f);
}

void TMC2209Driver::setRampMode(RampMode mode) {
    ramp_mode = mode;
}

void TMC2209Driver::setAcceleration(uint32_t amax) {
    planner.setAcceleration(amax);
}

void TMC2209Driver::setMaxSpeed(float vmax) {
    planner.setCruiseSpeed(vmax);
    if (ramp_mode == RAMP_VELOCITY) {
        planner.setVelocityMode(vmax);
    }
}

void TMC2209Driver::setCurrentPosition(int32_t position, bool reset) {
    if (reset) {
        planner.reset();
    }
    planner.setPosition(position);
    target_position = position;
}

void TMC2209Driver::setTargetPosition(int32_t position) {
    target_position = position;
    if (ramp_mode == RAMP_POSITIONING) {
        planner.setPositionMode(position);
        applyVelocity(planner.getCurrentSpeed());
    }
}

int32_t TMC2209Driver::getCurrentPosition() {
    return planner.getCurrentPosition();
}

int32_t TMC2209Driver::getTargetPosition() {
    return target_position;
}

float TMC2209Driver::getCurrentSpeed() {
    return planner.getCurrentSpeed();
}

bool TMC2209Driver::isTargetVelocityReached() {
    return planner.isTargetVelocityReached();
}

bool TMC2209Driver::isTargetPositionReached() {
    return planner.isTargetPositionReached();
}

void TMC2209Driver::tick(unsigned long dtMs) {
    if (dtMs == 0) {
        const unsigned long now = millis();
        dtMs = now - last_tick_ms;
        last_tick_ms = now;
    }

    planner.tick(dtMs);
    applyVelocity(planner.getCurrentSpeed());
}

DriverStatus TMC2209Driver::getDriverStatus() {
    if (motor->ot()) {
        return DRIVER_OT;
    }
    if (motor->otpw()) {
        return DRIVER_OTPW;
    }
    if (motor->s2ga()) {
        return DRIVER_S2GA;
    }
    if (motor->s2gb()) {
        return DRIVER_S2GB;
    }
    if (motor->s2vsa()) {
        return DRIVER_S2VSA;
    }
    if (motor->s2vsb()) {
        return DRIVER_S2VSB;
    }

    return DRIVER_OK;
}

void TMC2209Driver::setStealthChop(bool enabled) {
    applyStealthChop(enabled);
}

bool TMC2209Driver::isStealthChopEnabled() {
    return stealth_chop;
}

static void printBoolLine(const __FlashStringHelper *label, bool value) {
    Serial.print(label);
    Serial.println(value ? F("true") : F("false"));
}

static const char *driverStatusLabel(DriverStatus status) {
    switch (status) {
        case DRIVER_OK:
            return "OK";
        case DRIVER_OT:
            return "OT (overtemperature)";
        case DRIVER_OTPW:
            return "OTPW (overtemperature prewarning)";
        case DRIVER_S2VSA:
            return "S2VSA (short to supply, phase A)";
        case DRIVER_S2VSB:
            return "S2VSB (short to supply, phase B)";
        case DRIVER_S2GA:
            return "S2GA (short to ground, phase A)";
        case DRIVER_S2GB:
            return "S2GB (short to ground, phase B)";
        case DRIVER_CP_UV:
            return "CP_UV (charge pump undervoltage)";
        default:
            return "UNKNOWN";
    }
}

void TMC2209Driver::printDriveStatus() {
    if (motor == nullptr) {
        Serial.println(F("TMC2209: driver not initialized"));
        return;
    }

    const float speedStepsPerSec = planner.getCurrentSpeed();
    const float speedRpm = speedStepsPerSec * 60.0f / 200.0f;
    const uint8_t csActual = motor->cs_actual();
    const DriverStatus status = getDriverStatus();

    Serial.println(F("--- TMC2209 Drive Status ---"));

    Serial.println(F("[Chip]"));
    Serial.print(F("  version: 0x"));
    Serial.println(motor->version(), HEX);
    Serial.print(F("  UART CRC error (last read): "));
    Serial.println(motor->CRCerror ? F("true") : F("false"));

    Serial.println(F("[Current Config]"));
    Serial.print(F("  irun: "));
    Serial.println(motor->irun());
    Serial.print(F("  ihold: "));
    Serial.println(motor->ihold());
    Serial.print(F("  iholddelay: "));
    Serial.println(motor->iholddelay());
    Serial.print(F("  expected RMS mA (irun): "));
    Serial.println(motor->cs2rms(motor->irun()));
    printBoolLine(F("  i_scale_analog: "), motor->I_scale_analog());
    printBoolLine(F("  vsense (high sensitivity): "), motor->vsense());
    Serial.print(F("  microsteps: "));
    Serial.println(motor->microsteps());
    Serial.print(F("  toff: "));
    Serial.println(motor->toff());
    printBoolLine(F("  stealthChop (configured): "), stealth_chop);
    printBoolLine(F("  spreadCycle enabled: "), motor->en_spreadCycle());
    printBoolLine(F("  pwm_autoscale: "), motor->pwm_autoscale());
    Serial.print(F("  VACTUAL (commanded): "));
    Serial.println(last_vactual);

    Serial.println(F("[Motion]"));
    Serial.print(F("  speed (full steps/s): "));
    Serial.println(speedStepsPerSec);
    Serial.print(F("  speed (RPM): "));
    Serial.println(speedRpm);
    Serial.print(F("  position: "));
    Serial.println(planner.getCurrentPosition());
    Serial.print(F("  target position: "));
    Serial.println(target_position);
    printBoolLine(F("  standstill (stst): "), motor->stst());
    printBoolLine(F("  stealth mode active: "), motor->stealth());

    Serial.println(F("[Live Current]"));
    Serial.print(F("  cs_actual: "));
    Serial.println(csActual);
    Serial.print(F("  cs_actual RMS mA: "));
    Serial.println(motor->cs2rms(csActual));
    Serial.print(F("  cur_a: "));
    Serial.println(motor->cur_a());
    Serial.print(F("  cur_b: "));
    Serial.println(motor->cur_b());
    Serial.print(F("  pwm_scale_sum: "));
    Serial.println(motor->pwm_scale_sum());
    Serial.print(F("  pwm_scale_auto: "));
    Serial.println(motor->pwm_scale_auto());

    Serial.println(F("[Temperature]"));
    printBoolLine(F("  otpw (pre-warning): "), motor->otpw());
    printBoolLine(F("  ot (shutdown): "), motor->ot());
    printBoolLine(F("  t120 threshold: "), motor->t120());
    printBoolLine(F("  t143 threshold: "), motor->t143());
    printBoolLine(F("  t150 threshold: "), motor->t150());
    printBoolLine(F("  t157 threshold: "), motor->t157());

    Serial.println(F("[Faults]"));
    printBoolLine(F("  s2ga: "), motor->s2ga());
    printBoolLine(F("  s2gb: "), motor->s2gb());
    printBoolLine(F("  s2vsa: "), motor->s2vsa());
    printBoolLine(F("  s2vsb: "), motor->s2vsb());
    printBoolLine(F("  ola (open load A): "), motor->ola());
    printBoolLine(F("  olb (open load B): "), motor->olb());
    Serial.print(F("  driver status: "));
    Serial.println(driverStatusLabel(status));

    Serial.println(F("[GSTAT]"));
    printBoolLine(F("  reset: "), motor->reset());
    printBoolLine(F("  drv_err: "), motor->drv_err());
    printBoolLine(F("  uv_cp: "), motor->uv_cp());

    Serial.println(F("[IOIN]"));
    printBoolLine(F("  enn: "), motor->enn());
    printBoolLine(F("  ms1: "), motor->ms1());
    printBoolLine(F("  ms2: "), motor->ms2());
    printBoolLine(F("  diag: "), motor->diag());
    printBoolLine(F("  pdn_uart: "), motor->pdn_uart());
    printBoolLine(F("  step: "), motor->step());
    printBoolLine(F("  dir: "), motor->dir());
    printBoolLine(F("  spread_en: "), motor->spread_en());
}
