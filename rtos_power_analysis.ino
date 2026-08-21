With_RTOS_-_Task_Scheduling___Light_Sleep__Idle_Hook
  //CODE
  #include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"

#define TRIG_PIN 23
#define ECHO_PIN 22
#define LED      26

#define NO_OBJECT_SLEEP_MS  10000  // go to light sleep after 10 sec no object
#define IDLE_SLEEP_MS        5000  // idle mode after 5 sec no activity
#define LIGHT_SLEEP_US    5000000  // light sleep duration = 5 sec

unsigned long lastObjectTime  = 0;  // last time object was detected
unsigned long lastActivityTime = 0; // last time any activity happened
bool objectPresent            = false;
bool inLightSleep             = false;

SemaphoreHandle_t serialMutex;

// ─── Sensor Task ────────────────────────────────────────────
void sensorTask(void *pvParameters) {
  while (1) {

    // Trigger ultrasonic pulse
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(4);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 38000);
    unsigned long now = millis();

    if (duration == 0) {
      // ── No Object ──
      objectPresent = false;
      digitalWrite(LED, LOW);

      unsigned long noObjectDuration = now - lastObjectTime;

      xSemaphoreTake(serialMutex, portMAX_DELAY);
      Serial.print("["); Serial.print(now);
      Serial.print(" ms] No Object | LED OFF | No object for: ");
      Serial.print(noObjectDuration / 1000);
      Serial.println(" sec");
      xSemaphoreGive(serialMutex);

      // ── Check Idle Mode (5 sec no activity) ──
      if (now - lastActivityTime >= IDLE_SLEEP_MS) {
        xSemaphoreTake(serialMutex, portMAX_DELAY);
        Serial.println("--------------------------------------------");
        Serial.print("["); Serial.print(now);
        Serial.println(" ms] No activity for 5 sec --> IDLE MODE");
        Serial.println("--------------------------------------------");
        xSemaphoreGive(serialMutex);

        // Idle mode - slow down sensor check
        vTaskDelay(2000 / portTICK_PERIOD_MS);
      }

      // ── Check Light Sleep (10 sec no object) ──
      if (noObjectDuration >= NO_OBJECT_SLEEP_MS) {
        xSemaphoreTake(serialMutex, portMAX_DELAY);
        Serial.println("============================================");
        Serial.print("["); Serial.print(now);
        Serial.println(" ms] No object for 10 sec --> LIGHT SLEEP");
        Serial.println("Sensor will wake on next trigger cycle...");
        Serial.println("============================================");
        Serial.flush();
        xSemaphoreGive(serialMutex);

        digitalWrite(LED, LOW);
        inLightSleep = true;

        // Enter light sleep - wakes by timer
        esp_sleep_enable_timer_wakeup(LIGHT_SLEEP_US); // 5 sec sleep
        esp_light_sleep_start();

        // ── Woke up from sleep ──
        inLightSleep     = false;
        lastObjectTime   = millis(); // reset timer after wake
        lastActivityTime = millis();

        xSemaphoreTake(serialMutex, portMAX_DELAY);
        Serial.println("============================================");
        Serial.print("["); Serial.print(millis());
        Serial.println(" ms] WOKE UP from Light Sleep!");
        Serial.println("Checking for object...");
        Serial.println("============================================");
        xSemaphoreGive(serialMutex);
      }

    } else {
      // ── Object Detected ──
      objectPresent    = true;
      lastObjectTime   = now;
      lastActivityTime = now;

      float distance = (duration * 0.0343) / 2;
      digitalWrite(LED, HIGH);

      xSemaphoreTake(serialMutex, portMAX_DELAY);
      Serial.print("["); Serial.print(now);
      Serial.print(" ms] Object Detected at ");
      Serial.print(distance);
      Serial.println(" cm | LED ON");
      xSemaphoreGive(serialMutex);
    }

    // Fast check when object present, slow when absent
    if (objectPresent) {
      vTaskDelay(500 / portTICK_PERIOD_MS);  // check every 500ms
    } else {
      vTaskDelay(1000 / portTICK_PERIOD_MS); // check every 1000ms
    }
  }
}

// ─── LED Task ────────────────────────────────────────────────
void ledTask(void *pvParameters) {
  while (1) {
    if (!objectPresent) {
      digitalWrite(LED, LOW);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// ─── Monitor Task ────────────────────────────────────────────
void monitorTask(void *pvParameters) {
  while (1) {
    unsigned long now           = millis();
    unsigned long noObjectSec   = (now - lastObjectTime)   / 1000;
    unsigned long noActivitySec = (now - lastActivityTime) / 1000;

    xSemaphoreTake(serialMutex, portMAX_DELAY);
    Serial.println("--------------------------------------------");
    Serial.print("["); Serial.print(now);
    Serial.print(" ms] STATUS | Object: ");
    Serial.print(objectPresent ? "YES" : "NO");
    Serial.print(" | No object: ");
    Serial.print(noObjectSec);
    Serial.print(" sec | Idle: ");
    Serial.print(noActivitySec);
    Serial.print(" sec | Sleep: ");
    Serial.println(inLightSleep ? "YES" : "NO");
    Serial.println("--------------------------------------------");
    xSemaphoreGive(serialMutex);

    vTaskDelay(3000 / portTICK_PERIOD_MS); // print status every 3 sec
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(LED, LOW);
  delay(2000);

  lastObjectTime   = millis();
  lastActivityTime = millis();

  serialMutex = xSemaphoreCreateMutex();

  Serial.println("============================================");
  Serial.println("   ESP32 Smart Sensor - Object Detection   ");
  Serial.println("============================================");
  Serial.println("  LED ON  → Object Detected                ");
  Serial.println("  LED OFF → No Object                      ");
  Serial.println("  IDLE    → No activity for 5 sec          ");
  Serial.println("  SLEEP   → No object for 10 sec           ");
  Serial.println("  WAKE    → Auto wake every 5 sec to check ");
  Serial.println("============================================");

  xTaskCreatePinnedToCore(sensorTask,  "Sensor Task",  4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(ledTask,     "LED Task",     2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(monitorTask, "Monitor Task", 4096, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
