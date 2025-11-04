#ifndef WIFI_H_
#define WIFI_H_

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

#include "config_webpage.h"
#include "io.h"

#define EEPROM_START_ADDRESS 1408      // HomeKit library uses [0, 1408)
#define EEPROM_CONFIG_SIGNATURE 0x42   // Arbitrary byte to indicate stored config
#define EEPROM_TOTAL_SIZE (EEPROM_START_ADDRESS + sizeof(Configuration) + sizeof(byte))

#define DEVICE_SETUP        0
#define DEVICE_CONNECTING   1
#define DEVICE_OPERATIONAL  2

enum OperationMode : uint8_t {
  OP_MODE_HTTP = 0,
  OP_MODE_HOMEKIT = 1
};

struct Configuration {
  char ssid[30];
  char password[30];
  OperationMode opMode;
};

Configuration deviceConfig;

ESP8266WebServer server(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;

bool tryLoadConfig() {
  Serial.println("\nAttempting to load config.");
  EEPROM.begin(EEPROM_TOTAL_SIZE);

  byte signatureByte; 
  EEPROM.get(EEPROM_START_ADDRESS, signatureByte);

  if (signatureByte == EEPROM_CONFIG_SIGNATURE) {
    Serial.println("Found stored config.");

    // Read config from 1 byte after start address
    EEPROM.get(EEPROM_START_ADDRESS + sizeof(byte), deviceConfig);
    Serial.println("SSID: " + String(deviceConfig.ssid));
    Serial.println("Password: " + String(deviceConfig.password));
    Serial.println("Op-Mode: " + String(deviceConfig.opMode));
    return true;
  } else {
    Serial.println("No config found.");
    strcpy(deviceConfig.ssid, "");
    strcpy(deviceConfig.password, "");
    deviceConfig.opMode = OP_MODE_HTTP;
    return false;
  }
}

bool tryConnectWifi() {
  WiFi.persistent(false);
	WiFi.mode(WIFI_STA);
	WiFi.setAutoReconnect(true);
	WiFi.begin(deviceConfig.ssid, deviceConfig.password);

	Serial.println("WiFi connecting...");
	for (uint8_t i = 0; i < 30; i++) {
		if (WiFi.isConnected()) {
      Serial.print("\n");
      Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
      return true;
    }
		Serial.print(".");
    delay(150);
    io_set_led(true);
    delay(150);
    io_set_led(false);
	}
  Serial.println("\nWiFi connection timeout.");
  return false;
}

void handlePortal() {
  server.send(200, "text/html", index_html);
}

// Reference: https://github.com/CDFER/Captive-Portal-ESP32/blob/main/src/main.cpp
void handleConfigSetup() {
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);

  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("PC Switch Setup");

  dnsServer.setTTL(3600);
  dnsServer.start(53, "*", apIP);

  // Handle common captive portal triggers
  server.on("/generate_204", []() { server.sendHeader("Location", "http://192.168.4.1", true); server.send(302, "text/plain", ""); }); // Android
  server.on("/hotspot-detect.html", []() { server.sendHeader("Location", "http://192.168.4.1", true); server.send(302, "text/plain", ""); }); // Apple
  server.on("/redirect", []() { server.sendHeader("Location", "http://192.168.4.1", true); server.send(302, "text/plain", ""); });
  server.on("/connecttest.txt", []() { server.sendHeader("Location", "http://logout.net", true); server.send(302, "text/plain", ""); }); // Windows
  server.on("/wpad.dat", []() { server.send(404); }); // Some browsers probe this

  // Main UI
  server.on("/", handlePortal);
  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1", true);
    server.send(302, "text/plain", "");
  });

  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("opmode")) {
      String ssid = server.arg("ssid");
      String password = server.arg("password");
      int opmode = server.arg("opmode").toInt();

      Serial.println("Saving config:");
      Serial.println("SSID: " + ssid);
      Serial.println("Password: " + password);
      Serial.println("Mode: " + String(opmode));

      // Copy to struct
      strncpy(deviceConfig.ssid, ssid.c_str(), sizeof(deviceConfig.ssid));
      strncpy(deviceConfig.password, password.c_str(), sizeof(deviceConfig.password));
      deviceConfig.opMode = (OperationMode)opmode;

      // Save to EEPROM
      EEPROM.write(EEPROM_START_ADDRESS, EEPROM_CONFIG_SIGNATURE);
      EEPROM.put(EEPROM_START_ADDRESS + sizeof(byte), deviceConfig);
      EEPROM.commit();

      server.send(200, "text/html", saved_html);
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/plain", "Missing fields");
    }
  });

  server.begin();
  Serial.println("Captive portal started at http://192.168.4.1");

  // Main portal loop
  uint32_t lastBlink = millis();
  bool ledState = false;
  while (true) {
    dnsServer.processNextRequest();
    server.handleClient();

    uint32_t now = millis();
    if (now - lastBlink >= 1000) {
      ledState = !ledState;
      io_set_led(ledState);
      lastBlink = now;
    }

    delay(1);
  }
}

void wifi_initialize() {
  Serial.print("ESP8266 MAC Address: ");
  Serial.println(WiFi.macAddress()); // Get and print the MAC address
  if (tryLoadConfig()) {
    if(!tryConnectWifi()) {
      handleConfigSetup();
    }
  } else {
    handleConfigSetup();
  }
  io_set_led(true);
}

#endif