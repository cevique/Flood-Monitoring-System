#include "globals.h"

// pins
const int trigPin = 9;
const int echoPin = 8;
const int ledPin  = 7;
const int buzzerPin = 6;
const int buttonPin = 5;          // manual override (active LOW)
const int RX = 10;  // Arduino RX (connect to GSM TX)
const int TX = 11;  // Arduino TX (connect to GSM RX)

// sensor / measurement
unsigned long sampleIntervalMs = 1000;    // 1 second between samples (adjustable)
long duration;            // time (µs) between send and receive
float distanceCm = 999.0;
float prevdistanceCm = 16.0;  //  can be anything outside threshold to run code for 1st time

// flood thresholds (measured as distance from sensor to water surface)
// smaller distance -> higher water level
const float thresholdEnterCm = 15.0;       // enter alarm when distance <= this (high water)
const float hysteresisCm = 3.0;            // leave alarm when distance > thresholdEnterCm + hysteresisCm
const float thresholdEnterCm2 = 10.0;      // critical level for call

// debounce / stability settings
const int alarmLimit = 3; // number of consecutive readings required to enter alarm
int alarmCount = 0;
bool alarmState = false;  // true when alarm is active
bool answered = false; // true when call is answered

// (address, En, Rw, Rs, d4, d5, d6, d7, Bl, BlPol)
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
SoftwareSerial sim800(RX, TX); // setting up GSM module

// Emergency contact number
const String authorityNumber = "+923001234567";