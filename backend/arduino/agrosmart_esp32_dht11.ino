/*
 * ============================================================
 *  AgroSmart — ESP32 + DHT11 uniquement
 *
 *  Envoie température et humidité de l'air vers l'API AgroSmart
 *  Endpoint : POST /api/devices/{DEVICE_ID}/sensors
 *  Table    : sensor_data
 *
 *  Bibliothèques requises (Arduino Library Manager) :
 *    - DHT sensor library  (Adafruit)
 *    - Adafruit Unified Sensor
 *    - ArduinoJson  (Benoit Blanchon, v6.x)
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ============================================================
//  CONFIGURATION — À MODIFIER AVANT DE FLASHER
// ============================================================

// --- WiFi ---
const char* WIFI_SSID     = "Etudiants_ISM";        // Nom de votre réseau WiFi
const char* WIFI_PASSWORD = "Etudiant123&";          // Mot de passe WiFi

// --- API Backend ---
// IP de votre PC sur le réseau local → ipconfig → "Adresse IPv4"
const char* API_BASE_URL = "http://172.16.6.224:8000/api";
const int   DEVICE_ID    = 2;                        // Device ESP32 (user mariama, id=2)
const char* API_TOKEN    = "3|esbLbVL4XfgmpJ7Grx8DEdpWeU6RTKAd8yDpoGTZ6f094cab"; // Token Bearer

// --- Intervalle d'envoi ---
const unsigned long SEND_INTERVAL_MS = 30000; // 30 secondes

// ============================================================
//  BROCHE DHT11 — À ADAPTER SELON VOTRE CÂBLAGE
// ============================================================
#define DHT_PIN  4       // GPIO4 → broche Data du DHT11
#define DHT_TYPE DHT11

// ============================================================
//  VARIABLES GLOBALES
// ============================================================
DHT dht(DHT_PIN, DHT_TYPE);
unsigned long lastSendTime = 0;

// ============================================================
//  CONNEXION WIFI
// ============================================================
void connectWiFi() {
    Serial.print("\n[WiFi] Connexion à : ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] ✓ Connecté");
        Serial.print("[WiFi] IP locale : ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n[WiFi] ✗ Échec — vérifiez SSID/mot de passe");
    }
}

// ============================================================
//  ENVOI VERS L'API
// ============================================================
void sendToAPI(float temperature, float humidity) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[API] WiFi perdu — reconnexion...");
        connectWiFi();
        return;
    }

    // URL : POST /api/devices/{id}/sensors
    String url = String(API_BASE_URL) + "/devices/" + String(DEVICE_ID) + "/sensors";

    // Construire le JSON
    StaticJsonDocument<128> doc;
    doc["air_temperature"] = round(temperature * 10) / 10.0;
    doc["air_humidity"]    = round(humidity * 10) / 10.0;

    String payload;
    serializeJson(doc, payload);

    Serial.println("\n[API] URL     : " + url);
    Serial.println("[API] Payload : " + payload);

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type",  "application/json");
    http.addHeader("Accept",        "application/json");
    http.addHeader("Authorization", String("Bearer ") + API_TOKEN);
    http.setTimeout(10000);

    int httpCode = http.POST(payload);

    Serial.print("[API] Code HTTP : ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.println("[API] Réponse   : " + response);
        if (httpCode == 200 || httpCode == 201) {
            Serial.println("[API] ✓ Données enregistrées dans sensor_data");
        } else {
            Serial.println("[API] ✗ Erreur serveur — code " + String(httpCode));
        }
    } else {
        Serial.println("[API] ✗ Erreur HTTP : " + http.errorToString(httpCode));
    }

    http.end();
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n========================================");
    Serial.println("  AgroSmart ESP32 — DHT11");
    Serial.println("========================================");

    dht.begin();
    Serial.println("[DHT11] Initialisé sur GPIO" + String(DHT_PIN));

    connectWiFi();
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
        lastSendTime = millis();

        Serial.println("\n--- Lecture DHT11 ---");

        float temperature = dht.readTemperature();
        float humidity    = dht.readHumidity();

        // Vérifier si la lecture est valide
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("[DHT11] ✗ Lecture invalide — vérifiez le câblage");
            return;
        }

        Serial.print("[DHT11] Température : ");
        Serial.print(temperature, 1);
        Serial.println(" °C");

        Serial.print("[DHT11] Humidité    : ");
        Serial.print(humidity, 1);
        Serial.println(" %");

        sendToAPI(temperature, humidity);
    }

    delay(100);
}
