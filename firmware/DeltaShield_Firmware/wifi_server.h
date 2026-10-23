#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <WiFi.h>
#include <WebServer.h>
#include "sensors.h"
#include "relay_control.h"
#include "motor_control.h"

// External FSM hooks defined in DeltaShield_Firmware.ino
extern String getCurrentStateName();
extern void requestEmergencyStop();
extern bool requestStart();
extern void requestManualOverride(); 
extern void requestManualDrive(int leftPWM, int rightPWM); 
extern unsigned long _lastDashboardPing;

extern float getLatestConfidence();
extern int getSonarBadCount();
extern unsigned long getStateEntryMs();

WebServer server(80);

const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>DeltaShield</title>
    <style>
        body { font-family: 'Inter', sans-serif; background: #f9fafb; color: #1f2937; text-align: center; margin: 0; padding: 20px; }
        h1 { color: #16a34a; margin-bottom: 5px; }
        .nav-links { margin-bottom: 20px; }
        .nav-links a { color: #16a34a; text-decoration: none; font-weight: bold; margin: 0 10px; }
        .card { background: #ffffff; padding: 20px; border-radius: 12px; margin-bottom: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.05); border: 1px solid #e5e7eb; }
        .btn { border: none; padding: 15px 30px; font-size: 18px; border-radius: 8px; cursor: pointer; color: white; font-weight: bold; width: 100%; margin: 5px 0; transition: background 0.2s; }
        .btn-green { background: #22c55e; }
        .btn-green:active { background: #16a34a; }
        .btn-red { background: #ef4444; }
        .btn-red:active { background: #dc2626; }
        .btn-blue { background: #3b82f6; }
        .btn-blue:active { background: #2563eb; }
        .dpad { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; max-width: 300px; margin: 20px auto; }
        .dpad button { padding: 20px; font-size: 24px; background: #f3f4f6; border: 1px solid #d1d5db; color: #1f2937; border-radius: 8px; cursor: pointer; transition: background 0.2s; }
        .dpad button:active { background: #e5e7eb; }
        .empty { visibility: hidden; }
        .data-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; text-align: left; margin-top: 15px;}
        .data-item { background: #f9fafb; padding: 10px; border-radius: 6px; font-size: 14px; border: 1px solid #e5e7eb;}
        .value { color: #16a34a; font-weight: bold; }
    </style>
</head>
<body>
    <h1>DeltaShield</h1>
    <div class="nav-links">
        <a href="/">Dashboard</a> | <a href="/debug">Debug</a>
    </div>
    
    <div class="card">
        <h2>Dashboard</h2>
        <div class="data-grid">
            <div class="data-item">State: <span class="value" id="val-state">--</span></div>
            <div class="data-item">Battery: <span class="value" id="val-batt">--</span> V</div>
            <div class="data-item">ToF L: <span class="value" id="val-tofl">--</span> mm</div>
            <div class="data-item">ToF R: <span class="value" id="val-tofr">--</span> mm</div>
            <div class="data-item">Sonar: <span class="value" id="val-sonar">--</span> cm</div>
            <div class="data-item">Pump: <span class="value" id="val-pump">--</span></div>
            <div class="data-item">Motor L: <span class="value" id="val-ml">--</span></div>
            <div class="data-item">Motor R: <span class="value" id="val-mr">--</span></div>
        </div>
    </div>

    <div class="card">
        <h2>Main Controls</h2>
        <button class="btn btn-green" onclick="post('/start')">Start Autonomous</button>
        <button class="btn btn-red" onclick="post('/stop')">EMERGENCY STOP</button>
    </div>

    <div class="card">
        <h2>Manual Override</h2>
        <button class="btn btn-blue" onclick="post('/override')">Enable Manual Mode</button>
        <div class="dpad">
            <div class="empty"></div>
            <button onmousedown="ctrl('fwd')" onmouseup="ctrl('stop')" ontouchstart="ctrl('fwd')" ontouchend="ctrl('stop')">▲</button>
            <div class="empty"></div>
            <button onmousedown="ctrl('left')" onmouseup="ctrl('stop')" ontouchstart="ctrl('left')" ontouchend="ctrl('stop')">◀</button>
            <button onmousedown="ctrl('stop')" onmouseup="ctrl('stop')" ontouchstart="ctrl('stop')" ontouchend="ctrl('stop')">■</button>
            <button onmousedown="ctrl('right')" onmouseup="ctrl('stop')" ontouchstart="ctrl('right')" ontouchend="ctrl('stop')">▶</button>
            <div class="empty"></div>
            <button onmousedown="ctrl('rev')" onmouseup="ctrl('stop')" ontouchstart="ctrl('rev')" ontouchend="ctrl('stop')">▼</button>
            <div class="empty"></div>
        </div>
        <button class="btn btn-blue" onclick="post('/spray')">Manual Spray</button>
    </div>

    <script>
        function post(url) { fetch(url, {method: 'POST'}); }
        function ctrl(cmd) { fetch('/control?dir=' + cmd, {method: 'POST'}); }
        
        setInterval(() => {
            fetch('/status').then(r => r.json()).then(data => {
                document.getElementById('val-state').innerText = data.state;
                document.getElementById('val-batt').innerText = data.battery_boot.toFixed(1);
                document.getElementById('val-pump').innerText = data.pump ? "ON" : "OFF";
                document.getElementById('val-tofl').innerText = data.tofLeft.toFixed(0);
                document.getElementById('val-tofr').innerText = data.tofRight.toFixed(0);
                document.getElementById('val-sonar').innerText = data.ultrasonic.toFixed(1);
                document.getElementById('val-ml').innerText = data.motorLeft;
                document.getElementById('val-mr').innerText = data.motorRight;
            });
        }, 1000);
    </script>
</body>
</html>
)=====";

const char DEBUG_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>DeltaShield Debug</title>
    <style>
        body { font-family: 'Inter', sans-serif; background: #f9fafb; color: #1f2937; text-align: center; margin: 0; padding: 20px; }
        h1 { color: #16a34a; margin-bottom: 5px; }
        .nav-links { margin-bottom: 20px; }
        .nav-links a { color: #16a34a; text-decoration: none; font-weight: bold; margin: 0 10px; }
        .card { background: #ffffff; padding: 20px; border-radius: 12px; margin-bottom: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.05); border: 1px solid #e5e7eb; }
        .data-grid { display: grid; grid-template-columns: 1fr; gap: 8px; text-align: left; }
        .data-item { background: #f9fafb; padding: 8px 12px; border-radius: 6px; font-size: 14px; border: 1px solid #e5e7eb; display: flex; justify-content: space-between; }
        .value { color: #16a34a; font-family: monospace; font-weight: bold; }
    </style>
</head>
<body>
    <h1>System Debug</h1>
    <div class="nav-links">
        <a href="/">Dashboard</a> | <a href="/debug">Debug</a>
    </div>
    
    <div class="card">
        <h2>Raw Telemetry</h2>
        <div class="data-grid" id="debug-data">
            <!-- Populated via JS -->
        </div>
    </div>
    
    <script>
        setInterval(() => {
            fetch('/api/debug').then(r => r.json()).then(data => {
                let html = '';
                for (let key in data) {
                    html += `<div class="data-item"><span>${key}</span> <span class="value">${data[key]}</span></div>`;
                }
                document.getElementById('debug-data').innerHTML = html;
            });
        }, 1000);
    </script>
</body>
</html>
)=====";

void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void handleDebugPage() {
    server.send(200, "text/html", DEBUG_HTML);
}

void handleStatus() {
    _lastDashboardPing = millis();
    char json[256];
    snprintf(json, sizeof(json),
             "{\"state\":\"%s\",\"battery_boot\":%.2f,\"pump\":%s,\"tofLeft\":%.1f,\"tofRight\":%.1f,\"ultrasonic\":%.1f,\"motorLeft\":%d,\"motorRight\":%d}",
             getCurrentStateName().c_str(),
             readBatteryVoltageCached(),
             isPumpActive() ? "true" : "false",
             getCachedDistanceLeft(),
             getCachedDistanceRight(),
             getCachedUltrasonic(),
             getLeftMotorSpeed(),
             getRightMotorSpeed());
    server.send(200, "application/json", json);
}

void handleApiDebug() {
    char json[512];
    snprintf(json, sizeof(json),
             "{"
             "\"System State\":\"%s\","
             "\"Uptime (ms)\":%lu,"
             "\"Free Heap (bytes)\":%u,"
             "\"Battery (V)\":%.2f,"
             "\"ToF Sensors Ready\":%s,"
             "\"ToF Left (mm)\":%.1f,"
             "\"ToF Right (mm)\":%.1f,"
             "\"Sonar Dist (cm)\":%.1f,"
             "\"Sonar Bad Count\":%d,"
             "\"Motor L PWM\":%d,"
             "\"Motor R PWM\":%d,"
             "\"Pump Active\":%s,"
             "\"ML Confidence\":%.3f,"
             "\"State Entry (ms)\":%lu"
             "}",
             getCurrentStateName().c_str(),
             millis(),
             ESP.getFreeHeap(),
             readBatteryVoltageCached(),
             isTofReady() ? "true" : "false",
             getCachedDistanceLeft(),
             getCachedDistanceRight(),
             getCachedUltrasonic(),
             getSonarBadCount(),
             getLeftMotorSpeed(),
             getRightMotorSpeed(),
             isPumpActive() ? "true" : "false",
             getLatestConfidence(),
             getStateEntryMs()
             );
    server.send(200, "application/json", json);
}

void handleStart() {
    if (!server.authenticate("admin", "FieldPilot123")) return server.requestAuthentication();
    if (requestStart()) server.send(200, "text/plain", "Started");
    else server.send(400, "text/plain", "Cannot start from current state");
}

void handleStop() {
    if (!server.authenticate("admin", "FieldPilot123")) return server.requestAuthentication();
    requestEmergencyStop();
    server.send(200, "text/plain", "Stopped");
}

void handleOverride() {
    if (!server.authenticate("admin", "FieldPilot123")) return server.requestAuthentication();
    requestManualOverride();
    server.send(200, "text/plain", "Manual Mode Enabled");
}

void handleSpray() {
    if (!server.authenticate("admin", "FieldPilot123")) return server.requestAuthentication();
    if (getCurrentStateName() == "MANUAL_OVERRIDE") {
        firePump();
        server.send(200, "text/plain", "Spraying");
    } else {
        server.send(400, "text/plain", "Must be in manual mode");
    }
}

void handleControl() {
    if (!server.authenticate("admin", "FieldPilot123")) return server.requestAuthentication();
    if (getCurrentStateName() != "MANUAL_OVERRIDE") {
        server.send(400, "text/plain", "Must be in manual mode");
        return;
    }
    
    if (server.hasArg("dir")) {
        String dir = server.arg("dir");
        if (dir == "fwd") requestManualDrive(CRUISE_SPEED, CRUISE_SPEED);
        else if (dir == "rev") requestManualDrive(-CRUISE_SPEED, -CRUISE_SPEED);
        else if (dir == "left") requestManualDrive(-CRUISE_SPEED, CRUISE_SPEED);
        else if (dir == "right") requestManualDrive(CRUISE_SPEED, -CRUISE_SPEED);
        else if (dir == "stop") requestManualDrive(0, 0);
        else {
            server.send(400, "text/plain", "Unknown direction");
            return;
        }
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing dir arg");
    }
}

void wifiServerInit() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    WiFi.mode(WIFI_AP);
    WiFi.softAP("DeltaShield", "FieldPilot2026!"); 
    
    server.on("/", HTTP_GET, handleRoot);
    server.on("/debug", HTTP_GET, handleDebugPage);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/api/debug", HTTP_GET, handleApiDebug);
    server.on("/start", HTTP_POST, handleStart);
    server.on("/stop", HTTP_POST, handleStop);
    server.on("/override", HTTP_POST, handleOverride);
    server.on("/spray", HTTP_POST, handleSpray);
    server.on("/control", HTTP_POST, handleControl);
    
    server.begin();
    Serial.println("[WiFi] AP Started. IP: 192.168.4.1");
}

void wifiServerUpdate() {
    server.handleClient();
}

#endif
