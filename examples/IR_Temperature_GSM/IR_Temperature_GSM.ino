#include <Adafruit_MLX90614.h>
#include <MKRGSM.h>
#include <SPI.h>
#include <vrpc.h>

// APN and GSM data for the SIM used
const char PINNUMBER[] = "6908";
const char GPRS_APN[] = "internet.T-mobile";
const char GPRS_LOGIN[] = "congstar";
const char GPRS_PASSWORD[] = "cs";

// instantiation of utility libraries
GSMClient client;
GPRS gprs;
GSM gsm;
GSMScanner scan;
GSMLocation location;
VrpcAgent agent;
Adafruit_MLX90614 mlx;

void connect_gsm() {
  digitalWrite(LED_BUILTIN, HIGH);
  gprs.setTimeout(60000);
  gsm.setTimeout(60000);
  Serial.println("Connecting GSM...");
  while (gsm.begin(PINNUMBER) != GSM_READY ||
         gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) != GPRS_READY) {
    Serial.println("GSM Connection [FAILED]");
    Serial.println(scan.getCurrentCarrier());
    delay(1000);
  }
  Serial.println("GSM Connection [OK]");
  digitalWrite(LED_BUILTIN, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  connect_gsm();
  location.begin();
  agent.begin(client);
  mlx.begin();
}

void loop() {
  if (gsm.isAccessAlive()) {
    if (gprs.status() != GPRS_READY) {
      if (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) != GPRS_READY) {
        Serial.println("GRPS not ready!");
        delay(1000);
        return;
      }
    }
  } else {
    Serial.println("Reconnect to GSM...");
    connect_gsm();
    return;
  }
  agent.loop();
}

float getObjectTemperature() {
  return mlx.readObjectTempC();
}

float getAmbientTemperature() {
  return mlx.readAmbientTempC();
}

String getGSMCarrier() {
  return scan.getCurrentCarrier();
}

float latitude() {
  return location.latitude();
}

float longitude() {
  return location.longitude();
}

long altitude() {
  return location.altitude();
}

long accuracy() {
  return location.accuracy();
}

VRPC_GLOBAL_FUNCTION(float, getObjectTemperature);
VRPC_GLOBAL_FUNCTION(float, getAmbientTemperature);
VRPC_GLOBAL_FUNCTION(String, getGSMCarrier);
VRPC_GLOBAL_FUNCTION(float, latitude);
VRPC_GLOBAL_FUNCTION(float, longitude);
VRPC_GLOBAL_FUNCTION(long, altitude);
VRPC_GLOBAL_FUNCTION(long, accuracy);
