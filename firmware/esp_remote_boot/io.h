#ifndef IO_H_
#define IO_H_

#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_START_ADDRESS 1408      // HomeKit library uses [0, 1408)

#define DEVICE_SETUP        0
#define DEVICE_CONNECTING   1
#define DEVICE_OPERATIONAL  2

#define LED_PIN 14
#define IO_BUTTON_PIN 12

const uint16_t debounceDelay = 100; // ms
const uint32_t BUTTON_HOLD_TIME = 5000; // 5 seconds

uint8_t flashDelay = 1000; // 1 second
uint8_t lastLedFlash = 0;
bool led_override = false;
bool ledState = false;

bool lastButtonReading = HIGH;
uint32_t lastDebounceTime = 0;
bool buttonHeldReported = false;
uint32_t buttonPressStart = 0;

void io_setup();
void io_loop();
bool io_button_held();
void handleLED(uint8_t state);

// ----------------------------

void io_setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(IO_BUTTON_PIN, INPUT_PULLUP); // Active LOW
  digitalWrite(LED_PIN, LOW);
}

void io_loop() {
  uint32_t now = millis();

  // --- LED Flashing ---
  if (now - lastLedFlash >= flashDelay && led_override == false) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastLedFlash = now;
  }

  // --- Button Debouncing and Hold Detection ---
  bool reading = digitalRead(IO_BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) > debounceDelay) {
    if (reading == LOW) {
      if (buttonPressStart == 0) {
        buttonPressStart = now; // Button was just pressed
        buttonHeldReported = false;
      } else if (!buttonHeldReported && (now - buttonPressStart >= BUTTON_HOLD_TIME)) {
        // Button has been held for 5 seconds
        EEPROM.write(EEPROM_START_ADDRESS, 0x00); // Reset or clear config
        EEPROM.commit();
        Serial.println("Button held for 5s: EEPROM cleared.");
        buttonHeldReported = true;
        ESP.restart();
      }
    } else {
      buttonPressStart = 0;
      buttonHeldReported = false;
    }
  }

  lastButtonReading = reading;
}

void io_set_led(bool on) {
  led_override = true;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void io_led_speed(uint8_t delay) {
  led_override = false;
  flashDelay = delay;
}

#endif