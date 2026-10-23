#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

// ============================================================
//  DeltaShield — Relay / Pump Control
//  Team FieldPilot (ST17-032) | SolveThe17 2.0
// ============================================================

#include <Arduino.h>
#include "pins.h"
#include "esp_timer.h"

static bool          _pumpActive   = false;
static esp_timer_handle_t _pump_timer;

static void pump_timer_callback(void* arg) {
    digitalWrite(PIN_RELAY, LOW);
    _pumpActive = false;
}

void relayInit() {
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);   // pump OFF at startup
    _pumpActive = false;

    esp_timer_create_args_t timer_args = {};
    timer_args.callback = &pump_timer_callback;
    timer_args.name = "pump_watchdog";
    esp_timer_create(&timer_args, &_pump_timer);
}

// Begin a timed dose burst — hardware timer enforces shutoff.
void firePump() {
    if (_pumpActive) return;
    digitalWrite(PIN_RELAY, HIGH);
    _pumpActive  = true;
    esp_timer_start_once(_pump_timer, PUMP_BURST_MS * 1000ULL); // microsecs
}

// Call unconditionally every loop() iteration. (Now optional, but kept for compatibility)
void updatePump() {
    // Hardware timer handles shutoff automatically.
}

// Immediate hard stop
void stopPump() {
    esp_timer_stop(_pump_timer);
    digitalWrite(PIN_RELAY, LOW);
    _pumpActive = false;
}

bool isPumpActive() { return _pumpActive; }

#endif
