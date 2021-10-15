#include <DallasTemperature.h>
#include <MKRGSM.h>
#include <OneWire.h>
#include <SPI.h>
#include <vrpc.h>

// digital input pin for temperature signal
#define ONE_WIRE_BUS 1

// APN and GSM data for the SIM used
const char PINNUMBER[] = "6908";
const char GPRS_APN[] = "internet.T-mobile";
const char GPRS_LOGIN[] = "congstar";
const char GPRS_PASSWORD[] = "cs";

// instantiation of utility libraries
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
GSMClient client;
GPRS gprs;
GSM gsmAccess;
VrpcAgent agent;

unsigned long lastMillis = 0;

// last-value-cached of temperature in degree celsius
float temperature;

void ledOn() {
  digitalWrite(LED_BUILTIN, HIGH);
}

void ledOff() {
  digitalWrite(LED_BUILTIN, LOW);
}

float getTemperature() {
  return temperature;
}

void connect() {
  Serial.print("Connecting GSM...");
  while (gsmAccess.begin(PINNUMBER) != GSM_READY ||
         gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) != GPRS_READY) {
    delay(1000);
  }
  Serial.println("[OK]");
  agent.connect();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  agent.begin(client);
  connect();
  sensors.begin();
}

void loop() {
  agent.loop();
  if (!agent.connected()) {
    connect();
  }
  // read temperature every second
  if (millis() - lastMillis > 1000) {
    lastMillis = millis();
    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);
    Serial.println(temperature);
  }
}

VRPC_GLOBAL_FUNCTION(void, ledOn);
VRPC_GLOBAL_FUNCTION(void, ledOff);
VRPC_GLOBAL_FUNCTION(float, getTemperature);
VRPC_GLOBAL_FUNCTION(int, analogRead, uint8_t);
