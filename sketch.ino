/*
 Flood Management System - Robust Arduino Sketch
 - Ultrasonic sensor measures distance to water (cm)
 - Configurable consecutive-reading debounce: send SMS after N readings (5-20 recommended)
 - If high water persists longer, place a voice call and activate gates/alarms
 - LCD shows system status
 - GSM (SIM800) used for SMS and calls via SoftwareSerial
 - Manual override button (active LOW) resets alarm state

 Wiring (example):
  - Ultrasonic trigger -> digital pin 9
  - Ultrasonic echo  -> digital pin 8
  - LED (alarm)      -> digital pin 7 (through resistor)
  - Buzzer           -> digital pin 6 (use transistor if current > 20mA)
  - Button (override) -> digital pin 5 (wired to GND when pressed; uses INPUT_PULLUP)
  - GSM TX -> Arduino RX (pin 10)
  - GSM RX -> Arduino TX (pin 11)
  - LCD on I2C -> SDA/SCL (A4/A5 on UNO), address 0x27 (adjust if different)
  - Power: SIM800 needs a stable 4V supply with ~2A peak capability

 Notes:
  - Adjust smsTriggerCount between 5 and 20 as required.
  - Adjust sampleIntervalMs to control how often you read sensor (e.g., 1000 ms).
  - This sketch avoids spamming SMS: sends one SMS per alarm activation and then a call if condition persists.
*/

//  including libraries

#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//  including modules
#include "globals.h"
#include "sendSMS.h"
#include "makeCall.h"
#include "checkCallStatus.h"

void setup() {
 // pin
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // active LOW

  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);

  // serials
  Serial.begin(9600);
  sim800.begin(9600);
  sim800.setTimeout(200); // 200 ms read timeout
  sim800.println("AT+COLP=1"); // show connected line
  sim800.println("AT+CLCC=1"); // sometimes needed

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Flood Monitor");
  lcd.setCursor(0,1);
  lcd.print("Initializing...");
  delay(1500);
  lcd.clear();

  Serial.println("Setup complete.");
  delay(2000);
}

void loop() {
  // trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // read echo
  duration = pulseIn(echoPin, HIGH, 30000UL); // timeout 30ms
  if (duration == 0) {
    distanceCm = 999; // no echo
  } else {
    distanceCm = duration * 0.0343 / 2.0;
  }

  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  static float lastShown = -1;
  if (abs(distanceCm - lastShown) > 0.5) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Distance (cm):");
    lcd.setCursor(0,1);
    lcd.print(distanceCm);
    lastShown = distanceCm;
  }


  // hysteresis thresholds
  float enterThreshold = thresholdEnterCm;            // go into alarm when <= this
  float leaveThreshold = thresholdEnterCm + hysteresisCm; // exit alarm when > this
  // manual override: button pressed = safe
  if (digitalRead(buttonPin) == LOW) {
    alarmState = false;   // force safe
    alarmCount = 0;       // reset debounce
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Manual Override");
    delay(2000);
    lcd.clear();
  }

  // debounce + hysteresis logic
  if (!alarmState) {
    // not currently alarming: require consecutive readings <= enterThreshold
    if (distanceCm <= enterThreshold) {
      alarmCount++;
      if (alarmCount >= alarmLimit) {
        alarmState = true;
        alarmCount = 0;
      }
    } else {
      alarmCount = 0;
    }
  } else {
    // currently alarming: stay in alarm until distance > leaveThreshold
    if (distanceCm > leaveThreshold) {
      alarmCount++;
      if (alarmCount >= alarmLimit) {
        alarmState = false;
        alarmCount = 0;
      }
    } else {
      alarmCount = 0;
    }
  }

  // outputs based on alarmState
  if (alarmState) {
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 1000);
    if (prevdistanceCm > thresholdEnterCm && distanceCm <= thresholdEnterCm){
    // Deliver SMS only when water crosses threshold
    sendSMS(authorityNumber, "⚠ Alert! River water level rising above safe limit.");
    delay(500);
  }
  if (prevdistanceCm > thresholdEnterCm2 && distanceCm <= thresholdEnterCm2){
    // Make call only when water crosses threshold
    makeCall(authorityNumber);
  }
  } 
  else {
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
  }
  
  delay(500);
  prevdistanceCm = distanceCm; 
  checkCallStatus();
}

