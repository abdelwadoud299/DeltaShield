#ifndef DELTASHIELD_PINS_H
#define DELTASHIELD_PINS_H

// ============================================================
//  DeltaShield — Hardware v4 Pin Configuration (Production)
//  Team FieldPilot (ST17-032) | SolveThe17 2.0 National Hackathon
//
//  Verified against the physical AI-Thinker ESP32-CAM schematic.
//  Camera signals (GPIO0,5,18,19,21,22,23,25,26,27,32,34,35,36,39)
//  are hardwired to the OV2640 internally — never use any of them.
// ============================================================

// ---- Motor Driver (DRV8833) --------------------------------
// Speed control: PWM the active direction pin, LOW the other.
// EEP pin on module = SLEEP → tie to 3.3 V (always on).
// ULT pin on module = FAULT → leave unconnected (self-protects).

#define PIN_IN1   2    // Left FWD
#define PIN_IN2   13   // Left REV
#define PIN_IN3   15   // Right FWD
#define PIN_IN4   12   // Right REV

// Motor LEDC channels — explicit, isolated from camera channels 0–1
#define LEDC_CH_IN1   4
#define LEDC_CH_IN2   5
#define LEDC_CH_IN3   6
#define LEDC_CH_IN4   7

// ---- I2C Bus (shared: left VL53L1X + right VL53L1X + PCF8574)
// GPIO1 = U0TXD, GPIO3 = U0RXD.
// Disconnect during UART flashing; reconnect after boot.
#define PIN_SDA    3
#define PIN_SCL    1

// ---- VL53L1X Addresses ------------------------------------
#define ADDR_TOF_LEFT   0x29   // Factory default — left sensor keeps it
#define ADDR_TOF_RIGHT  0x30   // Reassigned via XSHUT during sensorsInit()

// ---- PCF8574 I2C GPIO Expander ----------------------------
#define PCF8574_ADDR     0x27   
#define PCF_XSHUT_RIGHT  0      // P0 → right VL53L1X XSHUT (+ 10 k pull-up → 3V3)
#define PCF_TRIG         1      // P1 → HC-SR04 TRIG (output only, timing not critical)
#define PCF_LED          2      // P2 → status LED cathode (sinking: LOW = ON)

// ---- HC-SR04 Ultrasonic -----------------------------------
#define PIN_ECHO   16

// ---- Relay & Built-in Diagnostic LED ----------------------
#define PIN_RELAY        4
#define PIN_STATUS_LED  33 // Built-in Red LED (Active-Low diagnostic heartbeat)

// ---- Battery ADC ------------------------------------------
#define PIN_BATT_ADC      14
#define DIVIDER_R1_OHMS   100000.0f   // battery+ → divider node
#define DIVIDER_R2_OHMS    47000.0f   // divider node → GND
#define BATT_LOW_VOLTAGE      6.5f    // SAFE_MODE trigger (Volts)
#define BATT_CHECK_INTERVAL  5000     // ms

// ---- PWM --------------------------------------------------
#define PWM_FREQ_HZ    1000
#define PWM_RES_BITS      8   // duty range 0–255

// ---- Operational Constants --------------------------------
#define CRUISE_SPEED          150    // PWM 0–255, tuned to ~0.15 m/s
#define OBSTACLE_TRIGGER_CM   20.0f  // obstacle detected below this
#define CONFIDENCE_THRESHOLD   0.80f // weed detection threshold
#define STEERING_KP            0.4f  // P-gain for row-following

// ---- Pump Timing ------------------------------------------
#define PUMP_BURST_MS      100   // calibrated micro-pulse duration
#define PUMP_WATCHDOG_MS   500   // absolute hardware safety ceiling

#endif
