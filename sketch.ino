// Flood monitor - debounce + hysteresis
const int trigPin = 9;
const int echoPin = 8;
const int ledPin  = 7;
const int buzzerPin = 6;

long duration;            // time (µs) between send and receive
float distanceCm;         // measured distance from sensor to water
float normalCm = 50.0;    // example baseline normal (set after observing Serial)
const float thresholdCm = 15.0; // critical level (enter alarm) in cm
const float hysteresisCm = 3.0; // stay-in-alarm margin

// debounce / stability settings
const int alarmLimit = 3; // number of consecutive readings required to enter alarm
int alarmCount = 0;
bool alarmState = false;  // true when alarm is active

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

  delay(500);
}
