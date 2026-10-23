#ifndef DELTASHIELD_LOGIC_H
#define DELTASHIELD_LOGIC_H

// ============================================================
//  DeltaShield — Pure Decision Logic
//  Team FieldPilot (ST17-032) | SolveThe17 2.0 National Hackathon
// ============================================================

inline bool checkHysteresis(float conf0, float conf1, float conf2,
                             float threshold) {
    int votes = 0;
    if (conf0 >= threshold) votes++;
    if (conf1 >= threshold) votes++;
    if (conf2 >= threshold) votes++;
    return votes >= 2;
}

inline bool checkObstacle(float distance_cm, float triggerDistance_cm) {
    if (distance_cm <= 0.0f) return true;    // fail-closed: sensor error -> treated as obstacle
    return (distance_cm < triggerDistance_cm);
}

inline bool checkBatteryFault(float voltage, float threshold) {
    static int faultCount = 0;
    if (voltage < threshold) {
        faultCount++;
        if (faultCount > 5) return true; // require 5 consecutive low readings
    } else if (voltage > threshold + 0.2f) { 
        // 0.2V hysteresis resume threshold
        faultCount = 0;
    }
    return false;
}

inline int computeSteeringCorrection(float distLeft_mm,
                                     float distRight_mm,
                                     float Kp) {
    float error = distLeft_mm - distRight_mm;
    int correction = (int)(Kp * error);
    if (correction >  80) correction =  80;
    if (correction < -80) correction = -80;
    return correction;
}

inline bool checkSensorTimeout(int consecutiveBadReadings,
                                int maxAllowed) {
    return consecutiveBadReadings >= maxAllowed;
}

#endif
