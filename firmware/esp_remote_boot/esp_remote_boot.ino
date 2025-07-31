#include <Arduino.h>
#include <arduino_homekit_server.h>

#include "wifi.h"

#define STATUS_PIN 4
#define BUTTON_PIN 5

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

bool lastStatus;

void setup() {
  pinMode(STATUS_PIN, INPUT);
  pinMode(BUTTON_PIN, OUTPUT);
  digitalWrite(BUTTON_PIN, LOW);

  Serial.begin(115200);

  wifi_initialize();
  homekit_setup();
}

void loop() {
  digitalWrite(LED_BUILTIN, getStatus() ? LOW : HIGH);
  homekit_loop();
  delay(10);
}

bool getStatus() {
  return !digitalRead(STATUS_PIN);
}

//==============================
// HomeKit setup and loop
//==============================

// access your HomeKit characteristics defined in homekit_accessory.h
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_switch_on;

static uint32_t next_heap_millis = 0;

//Called when the switch value is changed by iOS Home APP
void cha_switch_on_setter(const homekit_value_t value) {
	bool on = value.bool_value;
	LOG_D("Switch: %s", on ? "ON" : "OFF");

  if (on == getStatus()) {
    Serial.println("PC already in selected state.");
  }else {
    digitalWrite(BUTTON_PIN, HIGH);
    delay(200);
    digitalWrite(BUTTON_PIN, LOW);
  }

  cha_switch_on.value.bool_value = getStatus();
	homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void homekit_setup() {
	cha_switch_on.setter = cha_switch_on_setter;
	arduino_homekit_setup(&config);

  bool status = getStatus();
  lastStatus = status;
	cha_switch_on.value.bool_value = status;
	homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void homekit_loop() {

  bool status = getStatus();
  if(status != lastStatus) {
    lastStatus = status;
    Serial.println("Status changed. Updating...");
    cha_switch_on.value.bool_value = status;
	  homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
  }

	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_heap_millis) {
    cha_switch_on.value.bool_value = getStatus();
	  homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
		// show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}