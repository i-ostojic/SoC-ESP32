#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_AHTX0.h>
#include <Wire.h>
#include "secrets.h"  // WiFi lozinka, adresa servera i API kljuc

// WiFi postavke

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Server postavke

const char* serverUrl = SERVER_URL;
const char* api_key = API_KEY;

// Pinovi

const int TOUCH_PIN = 4;
#define ANALOG_PIN 34
#define R1 33000.0
#define R2 10000.0

// Senzor

Adafruit_AHTX0 aht;

// Postavke mjerenja

const int BROJ_MJERENJA = 8;
const int RAZMAK_MJERENJA = 2000;

void setup() {
  Serial.begin(115200);
  
  Wire.begin(21, 22);
  
  if (!aht.begin()) {
    Serial.println("AHT20 nije pronađen!");
  } else {
    Serial.println("AHT20 OK");
  }

  analogSetAttenuation(ADC_11db);
  pinMode(TOUCH_PIN, INPUT);
  
  Serial.print("Spajam se na WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi spojen!");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());
  Serial.println("Pritisni touch senzor za početak mjerenja...");
}

void saljiPodatke(float temperatura, float vlaga, float napon) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    doc["api_key"] = api_key;
    doc["temperature"] = temperatura;
    doc["voltage"] = napon;
    doc["current"] = 0;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial.println("Šaljem: " + jsonString);
    
    int httpCode = http.POST(jsonString);
    
    if (httpCode == 201 || httpCode == 200) {
      Serial.println("Podaci uspješno poslani!");
    } else {
      Serial.println("Greška pri slanju: " + String(httpCode));
    }
    
Serial.println("HTTP kod: " + String(httpCode));
Serial.println("Poruka: " + http.errorToString(httpCode));
    http.end();
  }
}

void loop() {
  if (digitalRead(TOUCH_PIN) == HIGH) {
    Serial.println("Touch senzor pritisnut! Počinjem mjerenje...");
    delay(500);
    
    for (int i = 0; i < BROJ_MJERENJA; i++) {
      Serial.println("Mjerenje " + String(i + 1) + "/" + String(BROJ_MJERENJA));
      
      // Očitaj temperaturu i vlagu
      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp);
      float temperatura = temp.temperature;
      float vlaga = humidity.relative_humidity;
      
      // Očitaj napon
      int mv = analogReadMilliVolts(ANALOG_PIN);
      float measuredVoltage = mv / 1000.0;
      float napon = measuredVoltage * ((R1 + R2) / R2);
      
      Serial.println("Temperatura: " + String(temperatura) + " °C");
      Serial.println("Vlaga: " + String(vlaga) + " %");
      Serial.println("Napon: " + String(napon) + " V");
      
      saljiPodatke(temperatura, vlaga, napon);
      
      if (i < BROJ_MJERENJA - 1) {
        delay(RAZMAK_MJERENJA);
      }
    }
    
    Serial.println("Mjerenje završeno! Pritisni touch senzor za novo mjerenje.");
    delay(1000);
  }
}