#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include "pins.h"

// Telemetry globals
static int _currentSpeedLeft = 0;
static int _currentSpeedRight = 0;

void motorsInit() {
    ledcAttachChannel(PIN_IN1, PWM_FREQ_HZ, PWM_RES_BITS, LEDC_CH_IN1);
    ledcAttachChannel(PIN_IN2, PWM_FREQ_HZ, PWM_RES_BITS, LEDC_CH_IN2);
    ledcAttachChannel(PIN_IN3, PWM_FREQ_HZ, PWM_RES_BITS, LEDC_CH_IN3);
    ledcAttachChannel(PIN_IN4, PWM_FREQ_HZ, PWM_RES_BITS, LEDC_CH_IN4);

    ledcWrite(PIN_IN1, 0);
    ledcWrite(PIN_IN2, 0);
    ledcWrite(PIN_IN3, 0);
    ledcWrite(PIN_IN4, 0);
}

// speed: -255 (full reverse) to +255 (full forward), 0 = coast
void setLeftMotor(int speed) {
    speed = constrain(speed, -255, 255);
    _currentSpeedLeft = speed;
    if      (speed > 0) { ledcWrite(PIN_IN1, speed); ledcWrite(PIN_IN2, 0);     }
    else if (speed < 0) { ledcWrite(PIN_IN1, 0);     ledcWrite(PIN_IN2, -speed);}
    else                { ledcWrite(PIN_IN1, 0);     ledcWrite(PIN_IN2, 0);     }
}

void setRightMotor(int speed) {
    speed = constrain(speed, -255, 255);
    _currentSpeedRight = speed;
    if      (speed > 0) { ledcWrite(PIN_IN3, speed); ledcWrite(PIN_IN4, 0);     }
    else if (speed < 0) { ledcWrite(PIN_IN3, 0);     ledcWrite(PIN_IN4, -speed);}
    else                { ledcWrite(PIN_IN3, 0);     ledcWrite(PIN_IN4, 0);     }
}

void stopMotors() {
    setLeftMotor(0);
    setRightMotor(0);
}

int getLeftMotorSpeed() { return _currentSpeedLeft; }
int getRightMotorSpeed() { return _currentSpeedRight; }

#endif
