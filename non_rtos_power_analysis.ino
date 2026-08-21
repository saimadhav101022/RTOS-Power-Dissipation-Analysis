#include "esp_sleep.h"

#define TRIG_PIN 23
#define ECHO_PIN 22
#define LED 26

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  delay(2000); // stabilize sensor on boot
  Serial.println("System Started...");
}

void handleLED() {
  unsigned long ledStart = millis();

  digitalWrite(LED, HIGH);
  Serial.println("LED ON");
  delay(1000);
  digitalWrite(LED, LOW);
  Serial.println("LED OFF");

  unsigned long ledEnd = millis();
  Serial.print("LED Operation Time: ");
  Serial.print(ledEnd - ledStart);
  Serial.println(" ms");
}

void handleSensor() {
  unsigned long sensorStart = millis();

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 38000);

  if (duration == 0) {
    Serial.println("No object detected");
  } else {
    float distance = (duration * 0.0343) / 2;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  unsigned long sensorEnd = millis();
  Serial.print("Sensor Operation Time: ");
  Serial.print(sensorEnd - sensorStart);
  Serial.println(" ms");
}

void loop() {
  unsigned long cycleStart = millis();

  Serial.println("=============================");
  Serial.println("--- SYSTEM AWAKE ---");

  // Run LED task
  handleLED();

  // Run Sensor task
  handleSensor();

  unsigned long cycleEnd = millis();
  Serial.print("Total Active Operation Time: ");
  Serial.print(cycleEnd - cycleStart);
  Serial.println(" ms");

  // Enter Light Sleep for 5 seconds
  Serial.println("Entering Light Sleep for 5 seconds...");
  Serial.println("=============================");
  Serial.flush(); // flush serial before sleeping

  esp_sleep_enable_timer_wakeup(5000000); // 5 seconds in microseconds
  esp_light_sleep_start();

  // Execution resumes here after wake
  Serial.println("Woke up from Light Sleep!");
}
