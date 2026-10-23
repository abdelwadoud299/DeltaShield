#ifndef SENSORS_H
#define SENSORS_H

#include <SparkFun_VL53L1X.h>
#include "pins.h"
#include "pcf8574_driver.h"

static SFEVL53L1X _tofLeft;
static SFEVL53L1X _tofRight;
static bool       _tofReady = false;

static float         _battCached    = 8.4f;   
static unsigned long _battLastMs    = 0;

// Telemetry Globals
static float _lastDistLeft = -1.0f;
static float _lastDistRight = -1.0f;
static float _lastDistUltra = -1.0f;

bool sensorsInit() {
    pcfSet(PCF_XSHUT_RIGHT, false);
    delay(10);


    if (_tofLeft.begin() != 0) {
        Serial.println("[Sensors] FAIL: left VL53L1X not responding at 0x29");
        return false;
    }
    Serial.println("[Sensors] OK: left VL53L1X at 0x29");

    pcfSet(PCF_XSHUT_RIGHT, true);
    delay(10);  


    _tofRight.setI2CAddress(ADDR_TOF_RIGHT); 

    if (_tofRight.begin() != 0) {
        Serial.println("[Sensors] FAIL: right VL53L1X not responding after reassignment");
        return false;
    }
    Serial.println("[Sensors] OK: right VL53L1X reassigned to 0x30");

    _tofLeft.startRanging();
    _tofRight.startRanging();
    _tofReady = true;

    pinMode(PIN_ECHO, INPUT);

    Serial.println("[Sensors] Both ToF sensors ranging");
    return true;
}

// FSM calls these. They read AND clear interrupts.
float readDistanceLeft() {
    if (!_tofReady) return -1.0f;
    if (!_tofLeft.checkForDataReady()) return -1.0f;
    float d = (float)_tofLeft.getDistance();
    _tofLeft.clearInterrupt();
    _lastDistLeft = d;
    return d;
}

float readDistanceRight() {
    if (!_tofReady) return -1.0f;
    if (!_tofRight.checkForDataReady()) return -1.0f;
    float d = (float)_tofRight.getDistance();
    _tofRight.clearInterrupt();
    _lastDistRight = d;
    return d;
}

float readUltrasonicCM() {
    pcfSet(PCF_TRIG, false);
    delayMicroseconds(2);
    pcfSet(PCF_TRIG, true);
    delayMicroseconds(10);
    pcfSet(PCF_TRIG, false);

    long duration = pulseIn(PIN_ECHO, HIGH, 30000UL);
    if (duration == 0) {
        _lastDistUltra = -1.0f;
        return -1.0f;
    }
    float dist = (float)duration * 0.0343f / 2.0f;
    _lastDistUltra = dist;
    return dist;
}

// Telemetry reads these. Non-destructive.
float getCachedDistanceLeft() { return _lastDistLeft; }
float getCachedDistanceRight() { return _lastDistRight; }
float getCachedUltrasonic() { return _lastDistUltra; }
bool isTofReady() { return _tofReady; }

// ADC2 battery read. ONLY CALL THIS BEFORE WIFI IS ENABLED!
float readBatteryVoltage() {
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(PIN_BATT_ADC);
    }
    float avg_raw = sum / 10.0f;
    float v_adc = (avg_raw / 4095.0f) * 3.3f;
    _battCached = v_adc * ((DIVIDER_R1_OHMS + DIVIDER_R2_OHMS) / DIVIDER_R2_OHMS);
    return _battCached;
}

// Web Server calls this safely.
float readBatteryVoltageCached() {
    return _battCached;
}

#endif
