/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 */

#include "tmc5160Driver.h"
#include "platform.h"
#include <Arduino.h>

static void printBoolLine(const __FlashStringHelper *label, bool value) {
    Serial.print(label);
    Serial.println(value ? F("true") : F("false"));
}

static uint16_t mresToMicrosteps(uint8_t mres) {
    switch (mres) {
        case 0: return 256;
        case 1: return 128;
        case 2: return 64;
        case 3: return 32;
        case 4: return 16;
        case 5: return 8;
        case 6: return 4;
        case 7: return 2;
        case 8: return 1;
        default: return 0;
    }
}

static int16_t signExtend9(uint16_t value) {
    if (value & 0x100) {
        return (int16_t)(value | 0xfe00);
    }
    return (int16_t)value;
}

static const char *rampModeLabel(uint32_t rampMode) {
    switch (rampMode) {
        case TMC5160_Reg::POSITIONING_MODE:
            return "positioning";
        case TMC5160_Reg::VELOCITY_MODE_POS:
            return "velocity (+)";
        case TMC5160_Reg::VELOCITY_MODE_NEG:
            return "velocity (-)";
        case TMC5160_Reg::HOLD_MODE:
            return "hold";
        default:
            return "unknown";
    }
}

TMC5160Driver::~TMC5160Driver() {
    delete motor;
    motor = nullptr;
}

bool TMC5160Driver::init(Platform *curPlatform, const MOTOR_DRIVER_CONFIG &config) {
    this->platform = curPlatform;
    const PlatformPins &pins = platform->getPins();

    // start motor controller. wants to be enabled to talk to it..
    platform->enableMotor(true);

    TMC5160::PowerStageParameters powerStageParams; // defaults.
    TMC5160::MotorParameters motorParams;

    motorParams.globalScaler = config.globalScaler; // Use Excel file to find correct scaler so that ihold=31 is max desired current
    motorParams.irun = config.iRun;                 // 31 is max running current, adjust accordingly.
    motorParams.ihold = 0;                   // holding current, 0 enables freewheel

    if (motor == nullptr) {
        motor = new TMC5160_SPI(pins.tmc_ss, TMC5160::DEFAULT_F_CLK, SPISettings(1000000, MSBFIRST, SPI_MODE0), SPI1);
    }

    // Library initializes with StealthChop enabled.
    motor->begin(powerStageParams, motorParams, TMC5160::NORMAL_MOTOR_DIRECTION);

    // Disable the transition to SpreadCycle, we don't need accuracy, prefer StealthChop
    if (config.stealthChop) {
        stealth_chop = true;
        motor->writeRegister(TMC5160_Reg::TPWMTHRS, 0);
    } else {
        stealth_chop = false;
        TMC5160_Reg::GCONF_Register gconf = { 0 };
        gconf.en_pwm_mode = false;
        gconf.shaft = TMC5160::NORMAL_MOTOR_DIRECTION;
        motor->writeRegister(TMC5160_Reg::GCONF, gconf.value);
    }

    motor->stop(); // Ensure the controller is initialized with the motor stopped
    platform->enableMotor(false);
    return true;
}

void TMC5160Driver::stop() {
    motor->stop();
}

void TMC5160Driver::setRampMode(RampMode mode) {
    if (mode == RAMP_POSITIONING) {
        motor->setRampMode(TMC5160::POSITIONING_MODE);
    } else {
        motor->setRampMode(TMC5160::VELOCITY_MODE);
    }
}

void TMC5160Driver::setAcceleration(uint32_t amax) {
    motor->setAcceleration(amax);
}

void TMC5160Driver::setMaxSpeed(float vmax) {
    motor->setMaxSpeed(vmax);
}

void TMC5160Driver::setCurrentPosition(int32_t position, bool reset) {
    motor->setCurrentPosition(position, reset);
}

void TMC5160Driver::setTargetPosition(int32_t position) {
    motor->setTargetPosition(position);
}

int32_t TMC5160Driver::getCurrentPosition() {
    return motor->getCurrentPosition();
}

int32_t TMC5160Driver::getTargetPosition() {
    return motor->getTargetPosition();
}

float TMC5160Driver::getCurrentSpeed() {
    return motor->getCurrentSpeed();
}

bool TMC5160Driver::isTargetVelocityReached() {
    return motor->isTargetVelocityReached();
}

bool TMC5160Driver::isTargetPositionReached() {
    return motor->getCurrentPosition() == motor->getTargetPosition();
}

void TMC5160Driver::tick(unsigned long dtMs) {
    // Hardware ramp generator requires no periodic updates.
    (void)dtMs;
}

DriverStatus TMC5160Driver::mapStatus(TMC5160::DriverStatus status) {
    switch (status) {
        case TMC5160::OK:
            return DRIVER_OK;
        case TMC5160::OT:
            return DRIVER_OT;
        case TMC5160::OTPW:
            return DRIVER_OTPW;
        case TMC5160::S2VSA:
            return DRIVER_S2VSA;
        case TMC5160::S2VSB:
            return DRIVER_S2VSB;
        case TMC5160::S2GA:
            return DRIVER_S2GA;
        case TMC5160::S2GB:
            return DRIVER_S2GB;
        case TMC5160::CP_UV:
            return DRIVER_CP_UV;
        default:
            return DRIVER_UNKNOWN;
    }
}

DriverStatus TMC5160Driver::getDriverStatus() {
    return mapStatus(motor->getDriverStatus());
}

void TMC5160Driver::setStealthChop(bool enabled) {
    stealth_chop = enabled;
    if (enabled) {
        TMC5160_Reg::GCONF_Register gconf;
        gconf.value = motor->readRegister(TMC5160_Reg::GCONF);
        gconf.en_pwm_mode = true;
        gconf.shaft = TMC5160::NORMAL_MOTOR_DIRECTION;
        motor->writeRegister(TMC5160_Reg::GCONF, gconf.value);
        motor->writeRegister(TMC5160_Reg::TPWMTHRS, 0);
    } else {
        TMC5160_Reg::GCONF_Register gconf;
        gconf.value = motor->readRegister(TMC5160_Reg::GCONF);
        gconf.en_pwm_mode = false;
        gconf.shaft = TMC5160::NORMAL_MOTOR_DIRECTION;
        motor->writeRegister(TMC5160_Reg::GCONF, gconf.value);
    }
}

bool TMC5160Driver::isStealthChopEnabled() {
    return stealth_chop;
}

void TMC5160Driver::printDriveStatus() {
    if (motor == nullptr) {
        Serial.println(F("TMC5160: driver not initialized"));
        return;
    }

    const TMC5160::DriverStatus status = motor->getDriverStatus();
    const float speedStepsPerSec = motor->getCurrentSpeed();
    const float speedRpm = speedStepsPerSec * 60.0f / 200.0f;

    TMC5160_Reg::IOIN_Register ioin = {0};
    ioin.value = motor->readRegister(TMC5160_Reg::IO_INPUT_OUTPUT);
    TMC5160_Reg::IHOLD_IRUN_Register iholdIrun = {0};
    iholdIrun.value = motor->readRegister(TMC5160_Reg::IHOLD_IRUN);
    const uint32_t globalScalerVal = motor->readRegister(TMC5160_Reg::GLOBAL_SCALER) & 0xFF;
    TMC5160_Reg::CHOPCONF_Register chopconf = {0};
    chopconf.value = motor->readRegister(TMC5160_Reg::CHOPCONF);
    TMC5160_Reg::GCONF_Register gconf = {0};
    gconf.value = motor->readRegister(TMC5160_Reg::GCONF);
    TMC5160_Reg::PWMCONF_Register pwmconf = {0};
    pwmconf.value = motor->readRegister(TMC5160_Reg::PWMCONF);
    TMC5160_Reg::DRV_STATUS_Register drvStatus = {0};
    drvStatus.value = motor->readRegister(TMC5160_Reg::DRV_STATUS);
    TMC5160_Reg::GSTAT_Register gstat = {0};
    gstat.value = motor->readRegister(TMC5160_Reg::GSTAT);
    TMC5160_Reg::RAMP_STAT_Register rampStat = {0};
    rampStat.value = motor->readRegister(TMC5160_Reg::RAMP_STAT);
    TMC5160_Reg::PWM_SCALE_Register pwmScale = {0};
    pwmScale.value = motor->readRegister(TMC5160_Reg::PWM_SCALE);
    const uint32_t mscuractReg = motor->readRegister(TMC5160_Reg::MSCURACT);
    const int16_t curA = signExtend9((uint16_t)(mscuractReg & 0x1FF));
    const int16_t curB = signExtend9((uint16_t)((mscuractReg >> 16) & 0x1FF));

    const uint32_t rampMode = motor->readRegister(TMC5160_Reg::RAMPMODE);
    const uint32_t vactualReg = motor->readRegister(TMC5160_Reg::VACTUAL);
    const uint32_t vmaxReg = motor->readRegister(TMC5160_Reg::VMAX);
    const uint32_t amaxReg = motor->readRegister(TMC5160_Reg::AMAX);
    const uint32_t tstep = motor->readRegister(TMC5160_Reg::TSTEP);

    Serial.println(F("--- TMC5160 Drive Status ---"));

    Serial.println(F("[Chip]"));
    Serial.print(F("  version: 0x"));
    Serial.println((uint32_t)ioin.version, HEX);
    Serial.print(F("  SPI read OK: "));
    Serial.println(motor->isLastReadSuccessful() ? F("true") : F("false"));

    Serial.println(F("[Current Config]"));
    Serial.print(F("  globalScaler: "));
    Serial.println(globalScalerVal);
    Serial.print(F("  irun: "));
    Serial.println((uint32_t)iholdIrun.irun);
    Serial.print(F("  ihold: "));
    Serial.println((uint32_t)iholdIrun.ihold);
    Serial.print(F("  iholddelay: "));
    Serial.println((uint32_t)iholdIrun.iholddelay);
    printBoolLine(F("  en_pwm_mode (stealthChop): "), static_cast<bool>(gconf.en_pwm_mode));
    printBoolLine(F("  stealthChop (configured): "), stealth_chop);
    printBoolLine(F("  pwm_autoscale: "), static_cast<bool>(pwmconf.pwm_autoscale));
    printBoolLine(F("  vsense (high sensitivity): "), static_cast<bool>(chopconf.vsense));
    Serial.print(F("  microsteps: "));
    Serial.println(mresToMicrosteps((uint8_t)chopconf.mres));
    Serial.print(F("  toff: "));
    Serial.println((uint32_t)chopconf.toff);

    Serial.println(F("[Motion]"));
    Serial.print(F("  ramp mode: "));
    Serial.println(rampModeLabel(rampMode));
    Serial.print(F("  speed (full steps/s): "));
    Serial.println(speedStepsPerSec);
    Serial.print(F("  speed (RPM): "));
    Serial.println(speedRpm);
    Serial.print(F("  position: "));
    Serial.println(motor->getCurrentPosition());
    Serial.print(F("  target position: "));
    Serial.println(motor->getTargetPosition());
    Serial.print(F("  VACTUAL (register): "));
    Serial.println(vactualReg);
    Serial.print(F("  VMAX (register): "));
    Serial.println(vmaxReg);
    Serial.print(F("  AMAX (register): "));
    Serial.println(amaxReg);
    Serial.print(F("  TSTEP: "));
    Serial.println(tstep);
    printBoolLine(F("  standstill (stst): "), static_cast<bool>(drvStatus.stst));
    printBoolLine(F("  stealth mode active: "), static_cast<bool>(drvStatus.stealth));
    printBoolLine(F("  velocity_reached: "), static_cast<bool>(rampStat.velocity_reached));
    printBoolLine(F("  position_reached: "), static_cast<bool>(rampStat.position_reached));
    printBoolLine(F("  vzero: "), static_cast<bool>(rampStat.vzero));

    Serial.println(F("[Live Current]"));
    Serial.print(F("  cs_actual: "));
    Serial.println((uint32_t)drvStatus.cs_actual);
    Serial.print(F("  cur_a: "));
    Serial.println(curA);
    Serial.print(F("  cur_b: "));
    Serial.println(curB);
    Serial.print(F("  pwm_scale_sum: "));
    Serial.println((uint32_t)pwmScale.pwm_scale_sum);
    Serial.print(F("  pwm_scale_auto: "));
    Serial.println((uint32_t)pwmScale.pwm_scale_auto);
    Serial.print(F("  sg_result: "));
    Serial.println((uint32_t)drvStatus.sg_result);

    Serial.println(F("[Temperature]"));
    printBoolLine(F("  otpw (pre-warning): "), static_cast<bool>(drvStatus.otpw));
    printBoolLine(F("  ot (shutdown): "), static_cast<bool>(drvStatus.ot));

    Serial.println(F("[Faults]"));
    printBoolLine(F("  s2ga: "), static_cast<bool>(drvStatus.s2ga));
    printBoolLine(F("  s2gb: "), static_cast<bool>(drvStatus.s2gb));
    printBoolLine(F("  s2vsa: "), static_cast<bool>(drvStatus.s2vsa));
    printBoolLine(F("  s2vsb: "), static_cast<bool>(drvStatus.s2vsb));
    printBoolLine(F("  ola (open load A): "), static_cast<bool>(drvStatus.ola));
    printBoolLine(F("  olb (open load B): "), static_cast<bool>(drvStatus.olb));
    printBoolLine(F("  stallguard: "), static_cast<bool>(drvStatus.stallguard));
    Serial.print(F("  driver status: "));
    Serial.println(TMC5160::getDriverStatusDescription(status));

    Serial.println(F("[GSTAT]"));
    printBoolLine(F("  reset: "), static_cast<bool>(gstat.reset));
    printBoolLine(F("  drv_err: "), static_cast<bool>(gstat.drv_err));
    printBoolLine(F("  uv_cp: "), static_cast<bool>(gstat.uv_cp));

    Serial.println(F("[IOIN]"));
    printBoolLine(F("  drv_enn: "), static_cast<bool>(ioin.drv_enn));
    printBoolLine(F("  refl_step: "), static_cast<bool>(ioin.refl_step));
    printBoolLine(F("  refr_dir: "), static_cast<bool>(ioin.refr_dir));
    printBoolLine(F("  sd_mode: "), static_cast<bool>(ioin.sd_mode));
    printBoolLine(F("  swcomp_in: "), static_cast<bool>(ioin.swcomp_in));
    Serial.print(F("  DRV_STATUS: 0x"));
    Serial.println(drvStatus.value, HEX);
}
