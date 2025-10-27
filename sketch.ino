// #include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Flood monitor - debounce + hysteresis
const int trigPin = 23;    // AJ-SR04M trigger ultrasonic wave 40kHz
const int echoPin = 8;     //  AJ-SR04M receive reflected ultrasonnic wave
const int ledPin = 19;     //  Control LED ON/OFF state
const int buzzerPin = 18;  //  Control Buzzer ON/OFF state
// const int RX = 10; //  Arduino receive signal transmitted by GSM
// const int TX = 11;  //  Arduino transmits signal received by GSM
const int buttonPin = 5;  // push button input
const int buttonPin2 = 4;
long duration;                    // time (µs) between send and receive
float distanceCm;                 // measured distance from sensor to water
float prevdistanceCm = 31.0;      //  can be anything outside threshold to run code for 1st time
const float thresholdCm = 30.0;   // critical level (enter alarm) in cm also sms
const float hysteresisCm = 3.0;   // stay-in-alarm margin
const float thresholdCm2 = 25.0;  // critical level for call

// debounce / stability settings
const int alarmLimit = 3;  // number of consecutive readings required to enter alarm
int alarmCount = 0;
bool alarmState = false;      // true when alarm is active
bool answered = false;        // true when call is answered
bool manualOverride = false;  // false = normal mode, true = override active

// (address, En, Rw, Rs, d4, d5, d6, d7, Bl, BlPol)
LiquidCrystal_I2C lcd(0x27, 16, 2);
// SoftwareSerial sim800(RX, TX); // setting up GSM module

void setup() {
  lcd.init();       // initialize LCD
  lcd.backlight();  // turn on backlight
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // active LOW
  pinMode(buttonPin2, INPUT_PULLUP);
  Serial.begin(115200);
  // sim800.begin(9600); // initializing GSM module0
  // sim800.setTimeout(200); // 200 ms read timeout
  // sim800.println("AT+COLP=1"); // show connected line
  // sim800.println("AT+CLCC=1"); // sometimes needed

  // delay(2000);
}

// void sendSMS(String number, String text) {
//   // AT commands to send SMS.
//   sim800.println("AT+CMGF=1");
//   delay(1000);
//   sim800.print("AT+CMGS=\"");
//   sim800.print(number);
//   sim800.println("\"");
//   delay(1000);
//   sim800.print(text);
//   delay(500);
//   sim800.write(26);
//   lcd.setCursor(0,0);
//   lcd.print("SMS Sent!");
//   delay(5000);
//   lcd.clear();
// }

// void makeCall(String number) {
//   answered = false;   // reset flag before starting

//   while (!answered && alarmState) {   // keep trying until answered or safe
//     // Start call
//     sim800.print("ATD");
//     sim800.print(number);
//     sim800.println(";");
//     lcd.clear();
//     lcd.setCursor(0,0);
//     lcd.print("Dialing...");

//     unsigned long start = millis();

//     // Check status for 20 seconds
//     while (millis() - start < 20000 && !answered && alarmState) {
//       checkCallStatus();
//     }

//     // Hang up after timeout or if answered
//     sim800.println("ATH");

//     if (answered) {
//       lcd.clear();
//       lcd.setCursor(0,0);
//       lcd.print("Call answered!");
//       delay(5000);
//       lcd.clear();
//       break;   // exit since picked
//     }
//     else if (alarmState) {  // only redial if still unsafe
//       lcd.clear();
//       lcd.setCursor(0,0);
//       lcd.print("No answer");
//       lcd.setCursor(0,1);
//       lcd.print("Redialing...");
//       delay(5000);  // wait before retry
//       lcd.clear();
//       // loop continues → will redial automatically
//     }
//   }

//   // If alarm ended while calling
//   if (!alarmState && !answered) {
//     lcd.clear();
//     lcd.setCursor(0,0);
//     lcd.print("Safe now!");
//     delay(3000);
//     lcd.clear();
//   }
// }


// void checkCallStatus() {
//   while (sim800.available()) {
//     String response = sim800.readStringUntil('\n');
//     response.trim();
//     if (response.length() == 0) continue;  // skip empty lines

//     Serial.println("URC: " + response);

//     if (response.indexOf("VOICE CALL: BEGIN") != -1) {
//       answered = true;
//     }
//     else if (response.indexOf("VOICE CALL: END") != -1) {
//       answered = false;
//     }
//     else if (response.indexOf("NO CARRIER") != -1 || response.indexOf("BUSY") != -1) {
//       answered = false;
//     }
//   }
// }




void override() {
  // --- Button 1 pressed: activate manual override ---
  if (digitalRead(buttonPin) == LOW) {
    manualOverride = true;
    alarmState = false;
    alarmCount = 0;

    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Manual Override");
    delay(800);
    lcd.clear();
  }

  // --- Button 2 pressed: cancel manual override ---
  if (digitalRead(buttonPin2) == LOW) {
    manualOverride = false;

    // reset counters and state so system restarts fresh
    alarmState = false;
    alarmCount = 0;

    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 1000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Override Cancel");
    lcd.setCursor(0, 1);
    lcd.print("Auto Resumed");
    delay(1000);
    lcd.clear();
  }
}

double readSensor() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);  // timeout 30ms
  if (duration == 0) {
    distanceCm = prevdistanceCm;  // no echo
  } else {
    distanceCm = duration / 58.00;
  }
  return distanceCm;
}

void loop() {
  distanceCm = readSensor();
  if (distanceCm > thresholdCm) {
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Distance: ");
    lcd.print(distanceCm);
  }

  // hysteresis thresholds
  float enterThreshold = thresholdCm;                 // go into alarm when <= this
  float leaveThreshold = thresholdCm + hysteresisCm;  // exit alarm when > this
  // manual override: button pressed = safe
  // Skip alarm logic if manual override is active
  // If manual override is active, skip alarm logic entirely
  override();

  if (manualOverride) {
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);

    lcd.setCursor(0, 0);
    lcd.print("Override Active ");
    lcd.setCursor(0, 1);
    lcd.print("System Paused   ");

    delay(300);
    return;  // skip the rest of the loop
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
    while (distanceCm <= thresholdCm && distanceCm >= thresholdCm2) {
      override();
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print("Distance: ");
      lcd.print(distanceCm);
      lcd.setCursor(0, 1);
      lcd.print("Possible Flood");  //  rising trend detected
      distanceCm = readSensor();
      delay(150);
      //   // Deliver SMS only when water crosses threshold
      //   sendSMS("+1234567890", "Alert! Possible Flood Detected.");
      //   delay(500);
    }

    while (distanceCm < thresholdCm2) {
      override();
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print("Distance: ");
      lcd.print(distanceCm);
      lcd.setCursor(0, 1);
      lcd.print("Flood Incoming!");  //  threshold exceeded
      distanceCm = readSensor();
      delay(150);
      //   // Make call only when water crosses threshold
      //   makeCall("+1234567890");
    }
  } else {
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
  }

  delay(500);
  prevdistanceCm = distanceCm;
  // checkCallStatus();
}
