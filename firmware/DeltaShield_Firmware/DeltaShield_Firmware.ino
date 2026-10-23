/*
  DeltaShield — Autonomous Micro-Targeting Weed Eradication Rover
  Team FieldPilot (ST17-032) | SolveThe17 2.0 National Hackathon
*/

#include <Wire.h>
#include "esp_camera.h"

#include "pins.h"
#include "deltashield_logic.h"
#include "motor_control.h"
#include "pcf8574_driver.h"
#include "sensors.h"
#include "relay_control.h"
#include "ml_inference.h"
#include "wifi_server.h"

// Hardware camera pin map
#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22

enum State { INIT, IDLE, PATROL, OBSTACLE_EVADE, TARGET_LOCK, SAFE_MODE, MANUAL_OVERRIDE };
static State _state = INIT;

static float _conf[3] = { 0.0f, 0.0f, 0.0f };
static unsigned long _stateEntryMs = 0;
static int _sonarBadCount = 0;
static bool _pumpFired = false;
unsigned long _lastDashboardPing = 0;

static volatile bool _stopRequested  = false;
static volatile bool _startRequested = false;
static volatile bool _overrideRequested = false;

// Manual control variables
static volatile int _manualLeftPWM = 0;
static volatile int _manualRightPWM = 0;

static bool cameraInit() {
    camera_config_t cfg;
    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.pin_d0       = Y2_GPIO_NUM;
    cfg.pin_d1       = Y3_GPIO_NUM;
    cfg.pin_d2       = Y4_GPIO_NUM;
    cfg.pin_d3       = Y5_GPIO_NUM;
    cfg.pin_d4       = Y6_GPIO_NUM;
    cfg.pin_d5       = Y7_GPIO_NUM;
    cfg.pin_d6       = Y8_GPIO_NUM;
    cfg.pin_d7       = Y9_GPIO_NUM;
    cfg.pin_xclk     = XCLK_GPIO_NUM;
    cfg.pin_pclk     = PCLK_GPIO_NUM;
    cfg.pin_vsync    = VSYNC_GPIO_NUM;
    cfg.pin_href     = HREF_GPIO_NUM;
    cfg.pin_sscb_sda = SIOD_GPIO_NUM;
    cfg.pin_sscb_scl = SIOC_GPIO_NUM;
    cfg.pin_pwdn     = PWDN_GPIO_NUM;
    cfg.pin_reset    = RESET_GPIO_NUM;
    cfg.xclk_freq_hz = 10000000;

    cfg.pixel_format = PIXFORMAT_RGB565;
    cfg.frame_size   = FRAMESIZE_96X96;
    cfg.fb_count     = 1;
    cfg.fb_location  = CAMERA_FB_IN_DRAM;
    cfg.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) return false;
    return true;
}

// ---- WiFi Server Hooks ----
String getCurrentStateName() {
    switch (_state) {
        case INIT:           return "INIT";
        case IDLE:           return "IDLE";
        case PATROL:         return "PATROL";
        case OBSTACLE_EVADE: return "OBSTACLE_EVADE";
        case TARGET_LOCK:    return "TARGET_LOCK";
        case SAFE_MODE:      return "SAFE_MODE";
        case MANUAL_OVERRIDE:return "MANUAL_OVERRIDE";
        default:             return "UNKNOWN";
    }
}

void requestEmergencyStop() { _stopRequested = true; }

bool requestStart() {
    // Can start from IDLE, SAFE_MODE, or MANUAL_OVERRIDE
    if (_state != IDLE && _state != SAFE_MODE && _state != MANUAL_OVERRIDE) return false;
    if (checkBatteryFault(readBatteryVoltageCached(), BATT_LOW_VOLTAGE)) return false;
    _startRequested = true;
    return true;
}

float getLatestConfidence() { return _conf[0]; }
int getSonarBadCount() { return _sonarBadCount; }
unsigned long getStateEntryMs() { return _stateEntryMs; }

void requestManualOverride() { _overrideRequested = true; }
void requestManualDrive(int left, int right) {
    _manualLeftPWM = left;
    _manualRightPWM = right;
}

void updateStatusLed() {
    static unsigned long _lastToggle = 0;
    static bool _ledState = true; // start HIGH (OFF)
    
    if (_state == TARGET_LOCK) {
        digitalWrite(PIN_STATUS_LED, LOW); // Solid ON
        return;
    }

    int interval = 0;
    switch (_state) {
        case PATROL: interval = 1000; break;
        case MANUAL_OVERRIDE: interval = 500; break;
        case OBSTACLE_EVADE: interval = 250; break;
        case SAFE_MODE: interval = 100; break;
        default: interval = 0; break; // OFF for INIT
    }

    if (interval == 0) {
        digitalWrite(PIN_STATUS_LED, HIGH); // Solid OFF
    } else {
        if (millis() - _lastToggle >= interval) {
            _lastToggle = millis();
            _ledState = !_ledState;
            digitalWrite(PIN_STATUS_LED, _ledState);
        }
    }
}

void setup() {
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH); // OFF

    Serial.begin(115200);
    delay(1000); // Give serial monitor 1 second to attach
    Serial.println("\n\n--- BOOT SEQUENCE START ---");

    // Pre-charge the camera power rail to prevent brownouts on the MB shield
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    digitalWrite(PWDN_GPIO_NUM, HIGH);
    delay(100);
    digitalWrite(PWDN_GPIO_NUM, LOW);
    delay(100);

    Serial.println("1. Initializing Camera...");
    if (!cameraInit()) {
        Serial.println("[Boot] Camera failed, but continuing in degraded mode.");
    } else {
        Serial.println("Camera initialized successfully!");
    }

    Serial.println("2. Initializing I2C... (SERIAL WILL NOW GO BLIND - I2C USES PINS 1 & 3)");
    Serial.flush(); // Wait for the hardware outbox to completely empty before hijacking the wire
    Wire.begin(PIN_SDA, PIN_SCL);
    
    relayInit();

    if (!pcfInit()) {
        // Can't print anymore, continuing degraded
    }

    motorsInit();

    // DO NOT START WIFI HERE. Wait until INIT state to read ADC safely!
    _state = INIT;
}

void loop() {
    updateStatusLed();
    updatePump();
    
    // External LED on PCF8574 (Dashboard Connected Indicator)
    // The dashboard polls every 1000ms. If we heard from it in the last 2.5s, it's connected.
    static bool _dashboardConnected = false;
    bool currentlyConnected = (millis() - _lastDashboardPing < 2500);
    if (currentlyConnected != _dashboardConnected) {
        _dashboardConnected = currentlyConnected;
        if (_dashboardConnected) ledOn();
        else ledOff();
    }
    
    // Only update WiFi if we aren't in INIT (since AP starts at end of INIT)
    if (_state != INIT) {
        wifiServerUpdate();
    }

    if (_stopRequested) {
        _stopRequested = false;
        stopMotors();
        stopPump();
        _state = SAFE_MODE;
        return;
    }

    if (_overrideRequested) {
        _overrideRequested = false;
        stopMotors();
        _manualLeftPWM = 0;
        _manualRightPWM = 0;
        _state = MANUAL_OVERRIDE;
        return;
    }

    switch (_state) {
        case INIT:           stateInit();           break;
        case IDLE:           stateIdle();           break;
        case PATROL:         statePatrol();         break;
        case OBSTACLE_EVADE: stateObstacleEvade();  break;
        case TARGET_LOCK:    stateTargetLock();     break;
        case SAFE_MODE:      stateSafeMode();       break;
        case MANUAL_OVERRIDE:stateManualOverride(); break;
    }
}

void stateInit() {
    bool sensorsOK = sensorsInit();
    
    // SAFE TO READ BATTERY HERE: WiFi IS OFF!
    float batt = readBatteryVoltage();
    Serial.printf("[INIT] Battery: %.2f V | Sensors: %s\n", batt, sensorsOK ? "OK" : "FAIL");

    // NOW START WIFI AFTER ADC READ!
    wifiServerInit();

    if (!sensorsOK) {
        _state = SAFE_MODE;
        return;
    }
    if (checkBatteryFault(batt, BATT_LOW_VOLTAGE)) {
        _state = SAFE_MODE;
        return;
    }

    _conf[0] = _conf[1] = _conf[2] = 0.0f;
    _sonarBadCount = 0;
    _state = IDLE;
}

void stateIdle() {
    stopMotors();
    stopPump();

    if (_startRequested) {
        _startRequested = false;
        _state = PATROL;
    }
}

void statePatrol() {
    float dL = readDistanceLeft();
    float dR = readDistanceRight();

    if (dL > 0.0f && dR > 0.0f) {
        int corr = computeSteeringCorrection(dL, dR, STEERING_KP);
        setLeftMotor (CRUISE_SPEED - corr);
        setRightMotor(CRUISE_SPEED + corr);
    } else {
        setLeftMotor (CRUISE_SPEED);
        setRightMotor(CRUISE_SPEED);
    }

    float dist = readUltrasonicCM();

    if (checkObstacle(dist, OBSTACLE_TRIGGER_CM)) {
        stopMotors();
        _stateEntryMs  = millis();
        _state         = OBSTACLE_EVADE;
        return;
    }

    if (dist < 0.0f) {
        _sonarBadCount++;
        if (checkSensorTimeout(_sonarBadCount, 3)) {
            stopMotors();
            _state = SAFE_MODE;
            return;
        }
    } else {
        _sonarBadCount = 0;
    }

    _conf[2] = _conf[1];
    _conf[1] = _conf[0];
    _conf[0] = getWeedConfidence();

    if (checkHysteresis(_conf[0], _conf[1], _conf[2], CONFIDENCE_THRESHOLD)) {
        stopMotors();
        _pumpFired    = false;
        _stateEntryMs = millis();
        _state        = TARGET_LOCK;
        return;
    }

    // Notice we use readBatteryVoltageCached() instead of reading the ADC!
    if (checkBatteryFault(readBatteryVoltageCached(), BATT_LOW_VOLTAGE)) {
        stopMotors();
        _state = SAFE_MODE;
        return;
    }
}

void stateObstacleEvade() {
    unsigned long elapsed = millis() - _stateEntryMs;

    if (elapsed < 400) {
        setLeftMotor (-CRUISE_SPEED);
        setRightMotor(-CRUISE_SPEED);
    } else if (elapsed < 800) {
        setLeftMotor ( CRUISE_SPEED);
        setRightMotor(-CRUISE_SPEED);
    } else {
        stopMotors();
        _sonarBadCount = 0;
        _conf[0] = _conf[1] = _conf[2] = 0.0f;
        _state = PATROL;
    }
}

void stateTargetLock() {
    if (!_pumpFired) {
        firePump();
        _pumpFired = true;
    }

    bool burstDone    = !isPumpActive();
    bool dwellElapsed = (millis() - _stateEntryMs) >= (unsigned long)(PUMP_BURST_MS + 400);

    if (burstDone && dwellElapsed) {
        _conf[0] = _conf[1] = _conf[2] = 0.0f;
        _state = PATROL;
    }
}

void stateSafeMode() {
    stopMotors();
    stopPump();

    if (_startRequested) {
        _startRequested = false;
        _state = INIT;
    }
}

void stateManualOverride() {
    // In manual mode, we just apply the requested PWM values.
    // Obstacle avoidance is OFF. The operator is fully responsible.
    setLeftMotor(_manualLeftPWM);
    setRightMotor(_manualRightPWM);

    if (_startRequested) {
        _startRequested = false;
        stopMotors();
        _state = INIT;
    }
}
