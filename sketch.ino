#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// I2C address 0x27, 16 column, 2 row LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Flood monitor - debounce + hysteresis
const int trigPin = 9;
const int echoPin = 8;
const int ledPin  = 7;
const int buzzerPin = 6;
const int gsmRx = 10;
const int gsmTx = 11;
const int buttonPin = 5;  // push button input

SoftwareSerial sim800(gsmRx, gsmTx); // setting up GSM module

long duration;            // time (µs) between send and receive
float distanceCm;         // measured distance from sensor to water
float prevdistanceCm = 16.0;
const float thresholdCm = 15.0; // critical level (enter alarm) in cm also sms
const float hysteresisCm = 3.0; // stay-in-alarm margin
const float thresholdCm2 = 10.0; // critical level for call

// debounce / stability settings
const int alarmLimit = 3; // number of consecutive readings required to enter alarm
int alarmCount = 0;
bool alarmState = false;  // true when alarm is active
bool answered = false; // true when call is answered

void setup() {
  lcd.init();           // initialize LCD
  lcd.backlight();      // turn on backlight
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // active LOW

  Serial.begin(9600);
  sim800.begin(9600); // initializing GSM module
  sim800.setTimeout(200); // 200 ms read timeout
  sim800.println("AT+COLP=1"); // show connected line
  sim800.println("AT+CLCC=1"); // sometimes needed

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
  float enterThreshold = thresholdCm;            // go into alarm when <= this
  float leaveThreshold = thresholdCm + hysteresisCm; // exit alarm when > this
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
    if (prevdistanceCm > thresholdCm && distanceCm <= thresholdCm){
    // Deliver SMS only when water crosses threshold
    sendSMS("+1234567890", "Alert! Possible Flood Detected.");
    delay(500);
  }
  if (prevdistanceCm > thresholdCm2 && distanceCm <= thresholdCm2){
    // Make call only when water crosses threshold
    makeCall("+1234567890");
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

void sendSMS(String number, String text) {
  // AT commands to send SMS.
  sim800.println("AT+CMGF=1");    
  delay(1000);
  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(1000);
  sim800.print(text);
  delay(500);
  sim800.write(26);
  lcd.setCursor(0,0);
  lcd.print("SMS Sent!");
  delay(5000);
  lcd.clear();
}

void makeCall(String number) {
  answered = false;   // reset flag before starting

  while (!answered && alarmState) {   // keep trying until answered or safe
    // Start call
    sim800.print("ATD");
    sim800.print(number);
    sim800.println(";");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Dialing...");

    unsigned long start = millis();

    // Check status for 20 seconds
    while (millis() - start < 20000 && !answered && alarmState) {
      checkCallStatus();
    }

    // Hang up after timeout or if answered
    sim800.println("ATH");

    if (answered) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Call answered!");
      delay(5000);
      lcd.clear();
      break;   // exit since picked
    } 
    else if (alarmState) {  // only redial if still unsafe
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("No answer");
      lcd.setCursor(0,1);
      lcd.print("Redialing...");
      delay(5000);  // wait before retry
      lcd.clear();
      // loop continues → will redial automatically
    }
  }

  // If alarm ended while calling
  if (!alarmState && !answered) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Safe now!");
    delay(3000);
    lcd.clear();
  }
}


void checkCallStatus() {
  while (sim800.available()) {
    String response = sim800.readStringUntil('\n');
    response.trim();
    if (response.length() == 0) continue;  // skip empty lines

    Serial.println("URC: " + response);

    if (response.indexOf("VOICE CALL: BEGIN") != -1) {
      answered = true;
    }
    else if (response.indexOf("VOICE CALL: END") != -1) {
      answered = false;
    }
    else if (response.indexOf("NO CARRIER") != -1 || response.indexOf("BUSY") != -1) {
      answered = false;
    }
  }
}
