#include <LiquidCrystal.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <vrpc.h>

const char ssid[] = "<WLAN-SSID>";
const char pass[] = "<WLAN-PASSWORD>";

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

WiFiClient wifi;
VrpcAgent agent;

// last-values-cached
String lvc_text[2];
float lvc_number[2];

void setText(String text, int row = 0) {
  clearRow(row);
  lcd.print(text);
  lvc_text[row > 0 ? 1 : 0] = text;
}

String getText(int row = 0) {
  return lvc_text[row > 0 ? 1 : 0];
}

void setNumber(float number, int row = 0) {
  clearRow(row);
  lcd.print(String(number, 2));
  lvc_number[row > 0 ? 1 : 0] = number;
}

float getNumber(int row = 0) {
  return lvc_number[row > 0 ? 1 : 0];
}

void clearRow(int row) {
  if (row == 0) {
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
  }
}

void connect() {
  lcd.setCursor(0, 1);
  lcd.print("Connect WiFi... ");
  while (WiFi.status() != WL_CONNECTED)
    delay(1000);
  lcd.print("OK");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Connect VRPC... ");
  agent.connect();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Heisenware GmbH");

  WiFi.begin(ssid, pass);
  agent.begin(wifi);

  connect();
}

void loop() {
  agent.loop();
  if (!agent.connected()) {
    connect();
    // restore text of second row
    setText(lvc_text[1], 1);
  }
}

VRPC_GLOBAL_FUNCTION(void, setText, String, int);
VRPC_GLOBAL_FUNCTION(void, setNumber, float, int);
VRPC_GLOBAL_FUNCTION(String, getText, int);
VRPC_GLOBAL_FUNCTION(float, getNumber, int);
