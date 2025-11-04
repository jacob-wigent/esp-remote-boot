#include <Arduino.h>
#include <arduino_homekit_server.h>

#include "wifi.h"
#include "io.h"

#define STATE_PIN 0
#define BUTTON_PIN 14

#define HEAP_REPORT_INTERVAL 5000 // Interval for heap reporting (ms)
#define IGNORE_CHANGE_PERIOD 5000 // Time after startup/shutdown to ignore HomeKit inputs (ms)
#define TARGET_TIMEOUT 60000 // Time to revert target state if not reached (ms)

// HomeKit Lock States (HAP sections 9.52, 9.56)
#define PC_ON 0
#define PC_OFF 1
#define PC_UNKNOWN 3

// Access HomeKit characteristics defined in homekit_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_lock_current_state;
extern "C" homekit_characteristic_t cha_lock_target_state;

static uint32_t lastHeapMillis = 0;
static uint32_t lastStateChangeMillis = 0;
static uint32_t targetSetMillis = 0;
static uint8_t lastState;

void triggerButtonPress();
uint8_t getState();

void setup() {
  
  pinMode(STATE_PIN, INPUT);
  pinMode(BUTTON_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(BUTTON_PIN, LOW);

  Serial.begin(115200);

  io_setup();
  wifi_initialize();

  cha_lock_target_state.setter = set_state;
  arduino_homekit_setup(&config);

  uint8_t state = getState();
  lastState = state;
  lastStateChangeMillis = millis();
  targetSetMillis = millis();
  cha_lock_current_state.value.int_value = state;
  cha_lock_target_state.value.int_value = state;
  homekit_characteristic_notify(&cha_lock_current_state, cha_lock_current_state.value);
  homekit_characteristic_notify(&cha_lock_target_state, cha_lock_target_state.value);
}

void loop() {
  uint8_t currentState = getState();
  const uint32_t now = millis();

  io_loop();

  // digitalWrite(LED_BUILTIN, currentState == PC_ON ? LOW : HIGH);
  // Serial.println(currentState);

  // If actual state changes, update HomeKit
  if (currentState != lastState) {
    Serial.printf("State changed from %d to %d. Notifying HomeKit.\n", lastState, currentState);
    if (lastState == PC_OFF) {
      lastStateChangeMillis = now;
    }
    lastState = currentState;
    cha_lock_current_state.value.int_value = currentState;
    homekit_characteristic_notify(&cha_lock_current_state, cha_lock_current_state.value);
    cha_lock_target_state.value.int_value = currentState;
    homekit_characteristic_notify(&cha_lock_target_state, cha_lock_target_state.value);
  }

  // Timeout if target hasn't been reached
  if ((cha_lock_target_state.value.int_value != currentState) && (now - targetSetMillis > TARGET_TIMEOUT)) {
    Serial.println("Target timeout reached. Updating target to current state.");
    cha_lock_target_state.value.int_value = currentState;
    homekit_characteristic_notify(&cha_lock_target_state, cha_lock_target_state.value);
  }

  arduino_homekit_loop();
  if (now - lastHeapMillis > HEAP_REPORT_INTERVAL) {
    lastHeapMillis = now;
    Serial.printf("Free heap: %d, HomeKit clients: %d\n", ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
  }

  delay(10);
}

void set_state(const homekit_value_t value) {
  const uint32_t now = millis();

  // Ignore input if within ignore window after state change
  if (now - lastStateChangeMillis < IGNORE_CHANGE_PERIOD) {
    Serial.println("Ignoring HomeKit input during transition period.");
    cha_lock_target_state.value.int_value = getState();
    homekit_characteristic_notify(&cha_lock_target_state, cha_lock_target_state.value);
    return;
  }

  uint8_t setState = value.int_value;
  Serial.printf("Set Target State: %s\n", setState == PC_ON ? "ON" : "OFF");

  cha_lock_target_state.value.int_value = setState;
  homekit_characteristic_notify(&cha_lock_target_state, cha_lock_target_state.value);
  targetSetMillis = now;

  if (setState == getState()) {
    Serial.println("PC already in desired state.");
    return;
  }

  triggerButtonPress();
}

void triggerButtonPress() {
  Serial.println("Triggering button press");
  digitalWrite(BUTTON_PIN, HIGH);
  delay(100);
  digitalWrite(BUTTON_PIN, LOW);
}

uint8_t getState() {
  return digitalRead(STATE_PIN) == LOW ? PC_ON : PC_OFF;
}