/*
  DeltaShield — Hardware Diagnostic Sketch (v3 — Final Pin Map)
  Team FieldPilot (ST17-032) | SolveThe17 2.0

  CHANGES FROM THE PREVIOUS VERSION:
  - HC-SR04 removed entirely (no longer part of the design)
  - Single VL53L1X only — no XSHUT, no address reassignment
  - Relay moved to GPIO4 (was GPIO0 — structurally safer pin)
  - Battery voltage and status LED now share GPIO33

  SETUP:
  1. Install "SparkFun VL53L1X" library via Library Manager
  2. Board: Tools > Board > ESP32 Arduino > AI Thinker ESP32-CAM
  3. Keep PIN_SDA (GPIO3) and PIN_SCL (GPIO1) wires DISCONNECTED
     while flashing — they share the UART lines used to upload code.
     Reconnect after upload finishes, then reopen Serial Monitor.
  4. Open Serial Monitor at 115200 baud.
*/

#include <Wire.h>
#include <SparkFun_VL53L1X.h>
#include "pins.h"

SFEVL53L1X tof;
bool tofReady = false;
bool ledIsOn = false;

// ---------------------------------------------------------------
//  Setup
// ---------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);
  stopMotors();

  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);   // OFF (relay jumper set to H = active-high)

  pinMode(PIN_BATT_LED, OUTPUT);
  ledOff();

  Wire.begin(PIN_SDA, PIN_SCL);

  printMenu();
  initToF();
}

// ---------------------------------------------------------------
//  Main loop
// ---------------------------------------------------------------
void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') return;

    switch (c) {
      case '1': testMotor("LEFT",  PIN_AIN1, PIN_AIN2, 1);  break;
      case '2': testMotor("LEFT",  PIN_AIN1, PIN_AIN2, -1); break;
      case '3': testMotor("RIGHT", PIN_BIN1, PIN_BIN2, 1);  break;
      case '4': testMotor("RIGHT", PIN_BIN1, PIN_BIN2, -1); break;
      case '5': testBothForward(); break;
      case '6': testToF();         break;
      case '7': testPump();        break;
      case '8': testBattery();     break;
      case '0': initToF();         break;
      case 'L': toggleLED();       break;
      case '?': printMenu();       break;
      default:  break;
    }
  }
}

// ---------------------------------------------------------------
//  Menu
// ---------------------------------------------------------------
void printMenu() {
  Serial.println();
  Serial.println(F("=== DeltaShield Hardware Diagnostic (v3) ==="));
  Serial.println(F(" 1 = Left motor forward (1s)"));
  Serial.println(F(" 2 = Left motor reverse (1s)"));
  Serial.println(F(" 3 = Right motor forward (1s)"));
  Serial.println(F(" 4 = Right motor reverse (1s)"));
  Serial.println(F(" 5 = Both motors forward (1s) - straight line test"));
  Serial.println(F(" 6 = Read VL53L1X distance"));
  Serial.println(F(" 7 = Fire pump relay (100ms burst)"));
  Serial.println(F(" 8 = Read battery voltage (briefly borrows the LED pin)"));
  Serial.println(F(" 0 = Re-initialize VL53L1X"));
  Serial.println(F(" L = Toggle status LED"));
  Serial.println(F(" ? = Show this menu"));
  Serial.println();
}

// ---------------------------------------------------------------
//  Motors
// ---------------------------------------------------------------
void testMotor(const char* label, int pinA, int pinB, int dir) {
  Serial.printf("[MOTOR] %s %s for 1 second...\n", label, dir == 1 ? "FORWARD" : "REVERSE");
  if (dir == 1) { digitalWrite(pinA, HIGH); digitalWrite(pinB, LOW); }
  else          { digitalWrite(pinA, LOW);  digitalWrite(pinB, HIGH); }
  delay(1000);
  digitalWrite(pinA, LOW);
  digitalWrite(pinB, LOW);
  Serial.println("[MOTOR] Stopped. Did it spin the correct direction?");
}

void testBothForward() {
  Serial.println("[MOTOR] BOTH forward for 1 second - rover should drive straight...");
  digitalWrite(PIN_AIN1, HIGH); digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, HIGH); digitalWrite(PIN_BIN2, LOW);
  delay(1000);
  stopMotors();
  Serial.println("[MOTOR] Stopped.");
}

void stopMotors() {
  digitalWrite(PIN_AIN1, LOW); digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, LOW); digitalWrite(PIN_BIN2, LOW);
}

// ---------------------------------------------------------------
//  VL53L1X — single sensor, no address reassignment needed
// ---------------------------------------------------------------
void initToF() {
  Serial.println("[ToF] Initializing single VL53L1X...");
  tofReady = false;

  tof.setBus(&Wire);
  if (tof.begin() != 0) {
    Serial.println("[ToF] FAIL - sensor not responding.");
    Serial.println("      Check wiring: VCC->3V3, GND, SDA->GPIO3, SCL->GPIO1.");
    Serial.println("      Confirm XSHUT is tied directly to 3V3, not floating.");
    return;
  }
  tof.startRanging();
  tofReady = true;
  Serial.println("[ToF] OK - sensor ranging at default address 0x29.");
}

void testToF() {
  if (!tofReady) {
    Serial.println("[ToF] Not initialized - press '0' first.");
    return;
  }
  if (tof.checkForDataReady()) {
    int dist = tof.getDistance();
    tof.clearInterrupt();
    Serial.printf("[ToF] Distance = %d mm\n", dist);
  } else {
    Serial.println("[ToF] Not ready yet, try again.");
  }
}

// ---------------------------------------------------------------
//  Relay / Pump
// ---------------------------------------------------------------
void testPump() {
  Serial.println("[PUMP] Firing 100ms burst...");
  digitalWrite(PIN_RELAY, HIGH);
  delay(100);   // blocking delay is fine for this manual bench test only.
                // The real FSM will use a non-blocking millis() timer.
  digitalWrite(PIN_RELAY, LOW);
  Serial.println("[PUMP] Done.");
}

// ---------------------------------------------------------------
//  GPIO33 time-sharing: LED most of the time, ADC briefly on demand
// ---------------------------------------------------------------
void ledOn()  { digitalWrite(PIN_BATT_LED, LOW);  ledIsOn = true;  } // inverted logic
void ledOff() { digitalWrite(PIN_BATT_LED, HIGH); ledIsOn = false; }
void toggleLED() { if (ledIsOn) ledOff(); else ledOn(); }

void testBattery() {
  bool wasOn = ledIsOn;

  pinMode(PIN_BATT_LED, INPUT);      // release the pin from LED duty
  delay(5);
  int raw = analogRead(PIN_BATT_LED);
  float v_adc = (raw / 4095.0) * 3.3;
  float v_batt = v_adc * ((DIVIDER_R1_OHMS + DIVIDER_R2_OHMS) / DIVIDER_R2_OHMS);

  pinMode(PIN_BATT_LED, OUTPUT);      // resume LED duty
  if (wasOn) ledOn(); else ledOff();

  Serial.printf("[BATTERY] Raw ADC=%d  ->  Pack voltage = %.2f V\n", raw, v_batt);
  if (v_batt < BATT_LOW_VOLTAGE) {
    Serial.println("[BATTERY] WARNING: below the 6.5V SAFE_MODE threshold");
  }
  Serial.println("[BATTERY] NOTE: compare against a multimeter reading on the");
  Serial.println("          same battery and adjust the divider values in");
  Serial.println("          pins.h together if they differ by more than ~0.1V.");
}
