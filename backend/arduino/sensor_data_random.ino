#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://your-server.local/api/devices/DEVICE_ID/sensors";
const char* bearerToken = "YOUR_API_TOKEN"; // si vous utilisez Sanctum/API token

unsigned long lastSend = 0;
const unsigned long intervalMs = 30000; // toutes les 30 secondes

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connecté");
}

float randomFloat(float minValue, float maxValue) {
  return minValue + ((float)random(0, 1001) / 1000.0) * (maxValue - minValue);
}

void sendSensorData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi non connecté");
    return;
  }

  float soilHumidity = randomFloat(30.0, 80.0);
  float airTemperature = randomFloat(18.0, 32.0);
  float airHumidity = randomFloat(35.0, 85.0);
  float waterLevel = randomFloat(20.0, 100.0);
  float liters = randomFloat(0.0, 3.5);

  String payload = "{";
  payload += "\"soil_humidity\":" + String(soilHumidity, 1) + ",";
  payload += "\"air_temperature\":" + String(airTemperature, 1) + ",";
  payload += "\"air_humidity\":" + String(airHumidity, 1) + ",";
  payload += "\"water_level\":" + String(waterLevel, 1) + ",";
  payload += "\"liters\":" + String(liters, 1);
  payload += "}";

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  if (strlen(bearerToken) > 0) {
    http.addHeader("Authorization", String("Bearer ") + bearerToken);
  }

  int httpResponseCode = http.POST(payload);
  Serial.print("HTTP response: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
    Serial.println(http.getString());
  }
  http.end();
}

void loop() {
  if (millis() - lastSend >= intervalMs) {
    lastSend = millis();
    sendSensorData();
  }
}
