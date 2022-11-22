#include <SSD1311.h>
#include <WiFiNINA.h>
#include <vrpc.h>
#include "Wire.h"

const char ssid[] = "<WLAN-SSID>";
const char pass[] = "<WLAN-PASSWORD>";
const char domain[] = "vrpc-arduino";
const char token[] = "";
const char host[] = "vrpc.io";

SSD1311 Screen;
WiFiClient wifi;
VrpcAgent agent;

void setText(String text, int row = 0) {
  clearRow(row);
  Screen.sendString(text.c_str(), row * 32, 0);
}

String getText() {
  String result;
  char buffer[] = "                    0";
  Screen.readString(buffer, 0, 0, 20);
  result += String(buffer) + "\n";
  Screen.readString(buffer, 32, 0, 20);
  result += String(buffer) + "\n";
  Screen.readString(buffer, 64, 0, 20);
  result += String(buffer) + "\n";
  Screen.readString(buffer, 96, 0, 20);
  result += String(buffer) + "\n";
  return result;
}

void clearRow(int row) {
  Screen.sendString("                    ", row * 32, 0);
}

void connect() {
  while (WiFi.status() != WL_CONNECTED) {
    Screen.sendString("Connect WiFi...     ", 0, 0);
    WiFi.begin(ssid, pass);
    delay(1000);
  }
  Screen.sendString("Connect WiFi... OK", 0, 0);
  agent.connect();
}

void setup() {
  Wire.begin();
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  Screen.powerMode(SSD1311_LCD_ON);
  Screen.clear();

  WiFi.begin(ssid, pass);
  agent.begin(wifi, domain, token, host);

  connect();
}

void loop() {
  agent.loop();
  if (!agent.connected()) {
    connect();
  }
}

VRPC_GLOBAL_FUNCTION(void, setText, String, int);
VRPC_GLOBAL_FUNCTION(String, getText);
