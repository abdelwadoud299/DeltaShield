#ifndef PCF8574_DRIVER_H
#define PCF8574_DRIVER_H

#include <Wire.h>
#include "pins.h"

static uint8_t _pcf_state = 0xFF;

static bool pcfWrite() {
    Wire.beginTransmission(PCF8574_ADDR);
    Wire.write(_pcf_state);
    return Wire.endTransmission() == 0;
}

void pcfSet(uint8_t pin, bool high) {
    if (high) _pcf_state |=  (1 << pin);
    else      _pcf_state &= ~(1 << pin);
    pcfWrite();
}

bool pcfGetLastState(uint8_t pin) {
    return (_pcf_state >> pin) & 0x01;
}

void ledOn()     { pcfSet(PCF_LED, false); }
void ledOff()    { pcfSet(PCF_LED, true);  }
void ledToggle() {
    bool current = (_pcf_state & (1 << PCF_LED)) == 0;
    pcfSet(PCF_LED, current);
}

bool pcfInit() {
    _pcf_state = 0xFF;
    pcfSet(PCF_TRIG,        false);
    pcfSet(PCF_LED,         true); 
    pcfSet(PCF_XSHUT_RIGHT, false);

    Wire.beginTransmission(PCF8574_ADDR);
    bool ok = (Wire.endTransmission() == 0);

    if (!ok) {
        Serial.printf("[PCF8574] ERROR: no ACK at address 0x%02X —\n", PCF8574_ADDR);
        Serial.println("          Check wiring: VCC=3V3, GND, SDA=GPIO3,");
        Serial.println("          SCL=GPIO1, A0/A1/A2 all to GND.");
    } else {
        Serial.printf("[PCF8574] OK at 0x%02X\n", PCF8574_ADDR);
    }
    return ok;
}

#endif
