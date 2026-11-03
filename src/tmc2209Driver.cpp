/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "tmc2209Driver.h"
#include "platform.h"
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

    // Keep UART current + microstep register select intact across mode flips
    // (UberKiwi backport — leaving i_scale_analog set causes screech / no motion).
    motor->I_scale_analog(false);
    motor->internal_Rsense(false);
    motor->pdn_disable(true);
    motor->mstep_reg_select(true);
    motor->multistep_filt(true);

    // Explicit StealthChop PWM setup (datasheet / Klipper reset-ish defaults).
    motor->pwm_ofs(36);
    motor->pwm_grad(14);
    motor->pwm_freq(1); // 2/683 * fCLK ≈ 35 kHz @ 12 MHz
    motor->pwm_autoscale(true);
    motor->pwm_autograd(true);
    motor->pwm_reg(8);
    motor->pwm_lim(12);

    if (enabled) {
        // Always-StealthChop (TPWMTHRS=0): no Stealth↔Spread handoff jerk on the basket.
        motor->en_spreadCycle(false);
        motor->TCOOLTHRS(0);
        motor->TPWMTHRS(0);
    } else {
        motor->en_spreadCycle(true);
        motor->TPWMTHRS(0);
    }
}

bool TMC2209Driver::init(Platform *curPlatform, const MOTOR_DRIVER_CONFIG &config) {
    this->platform = curPlatform;
    const PlatformPins &pins = platform->getPins();

    // start motor controller. wants to be enabled to talk to it..
    platform->enableMotor(true);

    if (motor == nullptr) {
        motor = new TMC2209Stepper(&Serial1, TMC2209_RSENSE, TMC2209_DRIVER_ADDRESS);
    }

    Serial.print(pins.tmc_uart_tx);
    Serial.println(pins.tmc_uart_rx);
    Serial1.setTX(pins.tmc_uart_tx);
    Serial1.setRX(pins.tmc_uart_rx);
    Serial1.begin(115200);
    delay(100);

    motor->begin();
    motor->pdn_disable(true);
    motor->mstep_reg_select(true);
    motor->multistep_filt(true);
    // Klipper defaults: toff=3, hstrt=5, hend=0, tbl=2. Explicit hysteresis —
    // omitting hstrt/hend makes SpreadCycle unusable and leaves StealthChop
    // poorly conditioned after OTP/stealth transitions (UberKiwi backport).
    motor->toff(3);
    motor->hstrt(5);
    motor->hend(0);
    motor->blank_time(36); // tbl=2
    motor->I_scale_analog(false);
    motor->internal_Rsense(false);
    motor->microsteps(TMC2209_MICROSTEPS);
    motor->semin(0);
    motor->TPOWERDOWN(20);
    // iholddelay=8 matches Klipper; 2 was aggressive for standstill blend.
    motor->iholddelay(8);

    // Translate menu GlobalScaler+IRun (5160 spreadsheet model) into the same
    // RMS target on this 2209 board. Do not program raw IRun — vsense(irun<16)
    // made low IRun values fall off the 5160 current curve.
    configured_scaler = config.globalScaler;
    configured_irun = config.iRun;
    if (configured_irun > 31) {
        configured_irun = 31;
    }
    target_run_ma = tmcMenuCurrentMa(configured_scaler, configured_irun);
    if (target_run_ma < 50) {
        target_run_ma = 50;
    }
    // holdMultiplier 0.5 keeps prior 2209 ihold≈irun/2 standstill behaviour.
    motor->rms_current(target_run_ma, 0.5f);

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
    Serial.print(F("  menu globalScaler: "));
    Serial.println(configured_scaler);
    Serial.print(F("  menu irun: "));
    Serial.println(configured_irun);
    Serial.print(F("  target RMS mA (5160-equivalent): "));
    Serial.println(target_run_ma);
    Serial.print(F("  irun (CS): "));
    Serial.println(motor->irun());
    Serial.print(F("  ihold: "));
    Serial.println(motor->ihold());
    Serial.print(F("  iholddelay: "));
    Serial.println(motor->iholddelay());
    Serial.print(F("  expected RMS mA (irun/CS): "));
    Serial.println(motor->cs2rms(motor->irun()));
    printBoolLine(F("  i_scale_analog: "), motor->I_scale_analog());
    printBoolLine(F("  vsense (high sensitivity): "), motor->vsense());
    Serial.print(F("  microsteps: "));
    Serial.println(motor->microsteps());
    Serial.print(F("  toff: "));
    Serial.println(motor->toff());
    Serial.print(F("  hstrt: "));
    Serial.println(motor->hstrt());
    Serial.print(F("  hend: "));
    Serial.println(motor->hend());
    Serial.print(F("  blank_time (tbl clocks): "));
    Serial.println(motor->blank_time());
    printBoolLine(F("  stealthChop (configured): "), stealth_chop);
    printBoolLine(F("  spreadCycle enabled: "), motor->en_spreadCycle());
    printBoolLine(F("  multistep_filt: "), motor->multistep_filt());
    Serial.print(F("  pwm_ofs: "));
    Serial.println(motor->pwm_ofs());
    Serial.print(F("  pwm_grad: "));
    Serial.println(motor->pwm_grad());
    printBoolLine(F("  pwm_autoscale: "), motor->pwm_autoscale());
    printBoolLine(F("  pwm_autograd: "), motor->pwm_autograd());
    Serial.print(F("  pwm_freq: "));
    Serial.println(motor->pwm_freq());
    Serial.print(F("  pwm_reg: "));
    Serial.println(motor->pwm_reg());
    Serial.print(F("  pwm_lim: "));
    Serial.println(motor->pwm_lim());
    Serial.print(F("  TPWMTHRS: "));
    Serial.println(motor->TPWMTHRS());
    Serial.print(F("  TCOOLTHRS: "));
    Serial.println(motor->TCOOLTHRS());
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
