#include <Adafruit_MLX90614.h>
#include <MKRGSM.h>
#include <SPI.h>
#include <vrpc.h>

// APN and GSM data for the SIM used
const char PINNUMBER[] = "<PIN NUMBER>";  // Leave "" when there is no pin
const char GPRS_APN[] = "<APN>";
const char GPRS_LOGIN[] = "<LOGIN>";
const char GPRS_PASSWORD[] = "<PASSWORD>";

// instantiation of utility libraries
GSMClient client;
GPRS gprs;
GSM gsm;
GSMModem modem;
GSMScanner scan;
GSMLocation location;
VrpcAgent agent;
Adafruit_MLX90614 mlx;

// last values cached for location information
float lat = 0;
float lng = 0;
long alt = 0;
long acc = 0;
unsigned long timeout = 0;

void connectGsm() {
  digitalWrite(LED_BUILTIN, HIGH);
  gprs.setTimeout(180000);
  gsm.setTimeout(180000);
  Serial.println("Connecting GSM...");
  while (gsm.begin(PINNUMBER) != GSM_READY ||
         gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) != GPRS_READY) {
    Serial.println("GSM Connection [FAILED]");
    delay(1000);
  }
  Serial.println("GSM Connection [OK]");
  Serial.println(scan.getCurrentCarrier());
  digitalWrite(LED_BUILTIN, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  connectGsm();
  location.begin();
  modem.begin();
  String token = modem.getIMEI();
  while (token == NULL) {
    delay(1000);
    token = modem.getIMEI();
  }
  agent.begin(client);
  mlx.begin();
}

void measureLocation() {
  if (location.available()) {
    lat = location.latitude();
    lng = location.longitude();
    alt = location.altitude();
    acc = location.accuracy();
  }
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
    connectGsm();
    return;
  }
  if (millis() - timeout > 2000) {
    measureLocation();
    timeout = millis();
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

String getSignalStrength() {
  return scan.getSignalStrength();
}

String getLocation() {
  return String(lat, 7) + "," + String(lng, 7);
}

long getAltitude() {
  return alt;
}

long getAccuracy() {
  return acc;
}

VRPC_GLOBAL_FUNCTION(float, getObjectTemperature);
VRPC_GLOBAL_FUNCTION(float, getAmbientTemperature);
VRPC_GLOBAL_FUNCTION(String, getGSMCarrier);
VRPC_GLOBAL_FUNCTION(String, getSignalStrength);
VRPC_GLOBAL_FUNCTION(String, getLocation);
VRPC_GLOBAL_FUNCTION(long, getAltitude);
VRPC_GLOBAL_FUNCTION(long, getAccuracy);
