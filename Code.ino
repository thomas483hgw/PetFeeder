#include <ESP32Servo.h>

#define TRIGGER_PIN_ANIMAL 5
#define ECHO_PIN_ANIMAL 18
#define TRIGGER_PIN_FOOD 19
#define ECHO_PIN_FOOD 21
#define SERVO_PIN 13

Servo futterServo;

unsigned long lastActivationTime = 0;
bool servoBlocked = false;

const int SERVO_OPEN_ANGLE = 0;
const int SERVO_CLOSED_ANGLE = 85;
const int SERVO_OPEN_DURATION = 1500; // 1,5 Sekunden
const unsigned long SERVO_BLOCK_DURATION = 300000; // 5 Minuten

float measureDistance(int triggerPin, int echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); // max 30ms
  float distance = duration * 0.0343 / 2.0;
  return distance;
}

void openServo() {
  futterServo.write(SERVO_OPEN_ANGLE);
  Serial.println("🔓 Servo geöffnet.");
  delay(SERVO_OPEN_DURATION);
  futterServo.write(SERVO_CLOSED_ANGLE);
  Serial.println("🔒 Servo geschlossen.");
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN_ANIMAL, OUTPUT);
  pinMode(ECHO_PIN_ANIMAL, INPUT);
  pinMode(TRIGGER_PIN_FOOD, OUTPUT);
  pinMode(ECHO_PIN_FOOD, INPUT);

  futterServo.setPeriodHertz(50);
  futterServo.attach(SERVO_PIN, 500, 2400);
  futterServo.write(SERVO_CLOSED_ANGLE);
}

void loop() {
  float distanceAnimal = measureDistance(TRIGGER_PIN_ANIMAL, ECHO_PIN_ANIMAL);
  float distanceFood = measureDistance(TRIGGER_PIN_FOOD, ECHO_PIN_FOOD);

  Serial.print("Tier: ");
  Serial.print(distanceAnimal);
  Serial.print(" cm | Futter: ");
  Serial.print(distanceFood);
  Serial.print(" cm");

  if (!servoBlocked &&
      distanceAnimal >= 5 && distanceAnimal <= 20 &&
      distanceFood > 13) {

    Serial.println(" ✅ Bedingungen erfüllt – Servo wird aktiviert.");
    openServo();
    servoBlocked = true;
    lastActivationTime = millis();

  } else if (servoBlocked) {
    Serial.println(" ⏳ Wartezeit aktiv – Servo gesperrt.");
  } else {
    Serial.println(" ❌ Bedingungen nicht erfüllt.");
  }

  if (servoBlocked && (millis() - lastActivationTime > SERVO_BLOCK_DURATION)) {
    servoBlocked = false;
    Serial.println("✅ Wartezeit vorbei – Servo wieder freigegeben.");
  }

  delay(1000);
}
