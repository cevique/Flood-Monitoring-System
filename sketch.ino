// Flood monitor - simple test
const int trigPin = 9;
const int echoPin = 8;
const int ledPin  = 7;
const int buzzerPin = 6;

long duration;  //  time (in microseconds) between the HC‑SR04 sending the ultrasonic pulse and receiving its echo
float distanceCm; //  distance between HC-SR04 and water level
const float thresholdCm = 15.0; // set critical level in cm

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(9600);
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

  if (distanceCm <= thresholdCm) {
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 1000); // 1kHz tone
  } else {
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
  }

  delay(500);
}
