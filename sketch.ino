// #include <SoftwareSerial.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

// ===== WiFi + ThingsBoard =====
const char* ssid = "HCCS FIRST FLOOR";
const char* password = "HCCSBLOCKA@2016";

const char* mqtt_server = "192.168.18.41";  // your ThingsBoard IP
const int mqtt_port = 1883;
const char* access_token = "0if4opdt9rom99fgz1al";  // copy from device details

WiFiClient espClient;
PubSubClient client(espClient);

// counters for pie chart
int floodPossibleCount = 0;
int floodIncomingCount = 0;
int normalCount = 0;

// Flood monitor - debounce + hysteresis
const int trigPin = 23;    // AJ-SR04M trigger ultrasonic wave 40kHz
const int echoPin = 35;    //  AJ-SR04M receive reflected ultrasonnic wave
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

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // --- WiFi Connect ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // --- MQTT setup ---
  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard...");
    if (client.connect("ESP32Client", access_token, NULL)) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3s");
      delay(3000);
    }
  }
  lcd.init();  // initialize LCD
  Wire.begin();
  Wire.setClock(400000);
  lcd.backlight();  // turn on backlight
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // active LOW
  pinMode(buttonPin2, INPUT_PULLUP);
  Serial.begin(115200);
}

void override() {
  // Button 1 pressed: activate manual override
  if (distanceCm <= thresholdCm) {
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
  }
  // Button 2 pressed: cancel manual override
  if (digitalRead(buttonPin2) == LOW && manualOverride == true) {
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

  duration = pulseIn(echoPin, HIGH);
  if (duration == 0) {
    distanceCm = prevdistanceCm;  // no echo
  } else {
    distanceCm = duration / 58.00;
  }
  return distanceCm;
}

void sendToThingsBoard(float distance, String status) {
  // Build JSON string manually
  String payload = "{\"distance\":";
  payload += String(distance, 2);
  payload += ",\"status\":\"" + status + "\"";

  // add counts for pie chart
  payload += ",\"normalCount\":" + String(normalCount);
  payload += ",\"floodPossibleCount\":" + String(floodPossibleCount);
  payload += ",\"floodIncomingCount\":" + String(floodIncomingCount);
  payload += "}";

  Serial.print("Sending payload: ");
  Serial.println(payload);

  // publish
  if (client.connected()) {
    client.publish("v1/devices/me/telemetry", payload.c_str());
  } else {
    Serial.println("MQTT disconnected, reconnecting...");
    if (client.connect("ESP32Client", access_token, NULL)) {
      client.publish("v1/devices/me/telemetry", payload.c_str());
    }
  }
}


void loop() {
  distanceCm = readSensor();
  if (distanceCm > thresholdCm) {
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);

    lcd.setCursor(0, 0);
    lcd.print("Distance: ");
    lcd.print(distanceCm);
    lcd.print("   ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }

  // hysteresis thresholds
  float enterThreshold = thresholdCm;                 // go into alarm when <= this
  float leaveThreshold = thresholdCm + hysteresisCm;  // exit alarm when > this
  override();

  // Auto cancel manual override when leaving threshold
  if (manualOverride && distanceCm > thresholdCm) {
    manualOverride = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Override AutoOff");
    lcd.setCursor(0, 1);
    lcd.print("Normal Mode     ");
    delay(800);
    lcd.clear();
  }


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

  String status = "Normal";

  // outputs based on alarmState
  if (alarmState) {
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 1000);
    while (distanceCm <= thresholdCm && distanceCm >= thresholdCm2) {
      override();
      lcd.setCursor(0, 0);
      lcd.print("Distance: ");
      lcd.print(distanceCm);
      lcd.print("   ");  // clear leftover chars
      lcd.setCursor(0, 1);
      lcd.print("Possible Flood   ");
      status = "Flood Possible";
      floodPossibleCount++;
      distanceCm = readSensor();
      sendToThingsBoard(distanceCm, status);
      delay(150);
    }

    while (distanceCm < thresholdCm2) {
      override();
      lcd.setCursor(0, 0);
      lcd.print("Distance: ");
      lcd.print(distanceCm);
      lcd.print("   ");
      lcd.setCursor(0, 1);
      lcd.print("Flood Incoming!  ");
      status = "Flood Incoming";
      floodIncomingCount++;
      distanceCm = readSensor();
      sendToThingsBoard(distanceCm, status);
      delay(150);
    }

  } else {
    digitalWrite(ledPin, LOW);
    normalCount++;
    sendToThingsBoard(distanceCm, status);
    noTone(buzzerPin);
  }

  sendToThingsBoard(distanceCm, status);
  client.loop();  // keep MQTT alive


  delay(500);
  prevdistanceCm = distanceCm;
}