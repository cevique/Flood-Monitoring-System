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

SoftwareSerial sim800(gsmRx, gsmTx);

long duration;            // time (µs) between send and receive
float distanceCm;         // measured distance from sensor to water
float prevdistanceCm;
const float thresholdCm = 15.0; // critical level (enter alarm) in cm also sms
const float hysteresisCm = 3.0; // stay-in-alarm margin
const float thresholdCm2 = 10.0; // critical level for call

// debounce / stability settings
const int alarmLimit = 3; // number of consecutive readings required to enter alarm
int alarmCount = 0;
bool alarmState = false;  // true when alarm is active

void setup() {
  lcd.init();           // initialize LCD
  lcd.backlight();      // turn on backlight
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(9600);
  sim800.begin(9600);
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
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Distance (cm):");
  lcd.setCursor(0,1);
  lcd.print(distanceCm);

  // hysteresis thresholds
  float enterThreshold = thresholdCm;            // go into alarm when <= this
  float leaveThreshold = thresholdCm + hysteresisCm; // exit alarm when > this

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
  } else {
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
  }

  if (prevdistanceCm > thresholdCm && distanceCm <= thresholdCm){
    // Deliver SMS only if water has reached threshold
    sendSMS("+1234567890", "Alert! Possible Flood Detected.");
    delay(500);

  }
  if (prevdistanceCm > thresholdCm2 && distanceCm <= thresholdCm2){
    // Make call only if water has reached threshold
    makeCall("+1234567890");
    delay(15000);
    hangUp();
  }
  
  delay(500);
  prevdistanceCm = distanceCm; 
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
  answered = false;   // reset flag

  // Start call
  sim800.print("ATD");
  sim800.print(number);
  sim800.println(";");
  lcd.setCursor(0,0);
  lcd.print("Dialing...");

  unsigned long start = millis();

  // Check status for 20 seconds
  while (millis() - start < 20000) {
    checkCallStatus();

    if (answered) {
      lcd.clear()
      lcd.setCursor(0,0);
      lcd.print("Call answered!");
      delay(5000);
      lcd.clear();
      break;  // exit early if picked
    }
  }

  // Hang up after timeout or if answered
  sim800.println("ATH");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Call ended.");

  // Redial logic
  if (!answered) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("No answer");
    lcd.setCursor(0,1);
    lcd.print("Redialing...");
    delay(5000);  // wait before retry
    lcd.clear();
    makeCall(number);
  }
}

void checkCallStatus() {
  sim800.println("AT+CLCC");   // Ask for call status
  delay(500);

  while (sim800.available()) {
    String response = sim800.readString();

  if (response.indexOf(",0,") != -1) {
    // Call active (answered)
    answered = true;
  }
}
