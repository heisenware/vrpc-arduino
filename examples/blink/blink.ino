#include <SPI.h>
#include <WiFiNINA.h>
#include <vrpc.h>

const char ssid[] = "<WLAN-SSID>";
const char pass[] = "<WLAN-PASSWORD>";

WiFiClient wifi;
VrpcAgent agent;

void ledOn() {
  digitalWrite(LED_BUILTIN, HIGH);
}

void ledOff() {
  digitalWrite(LED_BUILTIN, LOW);
}

void connect() {
  Serial.print("Connecting WiFi...");
  while (WiFi.status() != WL_CONNECTED) delay(1000);
  Serial.println("[OK]");
  agent.connect();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(ssid, pass);
  agent.begin(wifi);

  connect();

}

void loop() {
  agent.loop();
  if (!agent.connected()) connect();
}

VRPC_GLOBAL_FUNCTION(void, ledOn);
VRPC_GLOBAL_FUNCTION(void, ledOff);
VRPC_GLOBAL_FUNCTION(int, analogRead, uint8_t);
