#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

// pins
extern const int trigPin;
extern const int echoPin;
extern const int ledPin;
extern const int buzzerPin;
extern const int buttonPin;
extern const int RX;
extern const int TX;

// sensor / measurement
extern unsigned long sampleIntervalMs;
extern long duration;
extern float distanceCm;
extern float prevdistanceCm;

// flood thresholds
extern const float thresholdEnterCm;
extern const float hysteresisCm;
extern const float thresholdEnterCm2;

// debounce / stability settings
extern const int alarmLimit;
extern int alarmCount;

// system state
extern bool alarmState;
extern bool answered;

// LCD and SoftwareSerial
extern LiquidCrystal_I2C lcd;
extern SoftwareSerial sim800;

// emergency contact number
extern const String authorityNumber;

#endif