/*
 * ============================================================
 *  AgroSmart — ESP32 Multi-Capteurs
 *  Capteurs : DHT11, BME280, Capacitive Soil Moisture V1.2,
 *             HC-SR04 (ultrason), YF-TM02 (débitmètre)
 *
 *  Endpoint ciblé : POST /api/devices/{DEVICE_ID}/sensors
 *  Table de destination : sensor_data
 *
 *  Bibliothèques requises (Arduino Library Manager) :
 *    - DHT sensor library  (Adafruit)
 *    - Adafruit Unified Sensor
 *    - Adafruit BME280 Library
 *    - ArduinoJson  (Benoit Blanchon, v6.x)
 *    - WiFi  (intégrée ESP32)
 *    - HTTPClient  (intégrée ESP32)
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ============================================================
//  CONFIGURATION — À MODIFIER SELON VOTRE INSTALLATION
// ============================================================

// --- WiFi ---
const char* WIFI_SSID     = "VOTRE_SSID";        // Nom de votre réseau WiFi
const char* WIFI_PASSWORD = "VOTRE_MOT_DE_PASSE"; // Mot de passe WiFi

// --- API Backend ---
// Remplacez par l'IP de votre PC sur le réseau local (ex: 192.168.1.10)
// Vous pouvez la trouver avec : ipconfig (Windows) → "Adresse IPv4"
const char* API_BASE_URL  = "http://192.168.1.100:8000/api";
const int   DEVICE_ID     = 1;          // ID de votre appareil dans la BDD (table devices)
const char* API_TOKEN     = "VOTRE_TOKEN_SANCTUM"; // Token Bearer (obtenu après login)

// --- Intervalle d'envoi ---
const unsigned long SEND_INTERVAL_MS = 30000; // 30 secondes entre chaque envoi

// ============================================================
//  BROCHES GPIO — À ADAPTER SELON VOTRE CÂBLAGE
// ============================================================

// DHT11
#define DHT_PIN    4        // GPIO4 → Data du DHT11
#define DHT_TYPE   DHT11

// BME280 (I2C)
// SDA → GPIO21 (par défaut ESP32)
// SCL → GPIO22 (par défaut ESP32)
#define BME280_I2C_ADDR 0x76  // Adresse I2C : 0x76 ou 0x77 selon le module

// Capteur d'humidité du sol capacitif V1.2 (analogique)
#define SOIL_SENSOR_PIN  34   // GPIO34 (entrée analogique uniquement)
// Valeurs de calibration à mesurer sur votre capteur :
//   SOIL_DRY  = valeur ADC dans l'air (sol sec)
//   SOIL_WET  = valeur ADC dans l'eau (sol saturé)
#define SOIL_DRY  3200        // Valeur ADC typique à l'air libre (~3.3V)
#define SOIL_WET   800        // Valeur ADC typique dans l'eau (~0.8V)

// Capteur ultrasonique HC-SR04 (niveau d'eau)
#define TRIG_PIN   5          // GPIO5 → TRIG
#define ECHO_PIN   18         // GPIO18 → ECHO
// Distance en cm quand le réservoir est VIDE (capteur au-dessus du réservoir)
#define RESERVOIR_EMPTY_CM  30.0
// Distance en cm quand le réservoir est PLEIN
#define RESERVOIR_FULL_CM    2.0

// Débitmètre YF-TM02 (signal impulsions)
#define FLOW_SENSOR_PIN  19   // GPIO19 → Signal du débitmètre (interrupt capable)
// Facteur de calibration YF-TM02 : impulsions par litre
// Valeur typique YF-TM02 : ~450 impulsions/litre (à ajuster selon votre modèle)
#define FLOW_CALIBRATION_FACTOR  450.0

// ============================================================
//  VARIABLES GLOBALES
// ============================================================

DHT          dht(DHT_PIN, DHT_TYPE);
Adafruit_BME280 bme;

// Débitmètre — compteur d'impulsions (volatile car modifié en ISR)
volatile unsigned long flowPulseCount = 0;
unsigned long lastFlowCheck    = 0;
float         totalLiters      = 0.0;

unsigned long lastSendTime     = 0;
bool          bmeAvailable     = false;

// ============================================================
//  INTERRUPTION — Débitmètre YF-TM02
// ============================================================
void IRAM_ATTR flowPulseISR() {
    flowPulseCount++;
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n========================================");
    Serial.println("  AgroSmart ESP32 — Démarrage");
    Serial.println("========================================");

    // --- DHT11 ---
    dht.begin();
    Serial.println("[DHT11] Initialisé sur GPIO" + String(DHT_PIN));

    // --- BME280 ---
    Wire.begin();
    if (bme.begin(BME280_I2C_ADDR)) {
        bmeAvailable = true;
        Serial.println("[BME280] Initialisé (I2C 0x" + String(BME280_I2C_ADDR, HEX) + ")");
    } else {
        Serial.println("[BME280] NON DÉTECTÉ — vérifiez le câblage I2C et l'adresse");
    }

    // --- Capteur sol ---
    pinMode(SOIL_SENSOR_PIN, INPUT);
    Serial.println("[SOL] Capteur capacitif sur GPIO" + String(SOIL_SENSOR_PIN));

    // --- Ultrason ---
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    Serial.println("[ULTRASON] TRIG=GPIO" + String(TRIG_PIN) + " ECHO=GPIO" + String(ECHO_PIN));

    // --- Débitmètre ---
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseISR, RISING);
    Serial.println("[DEBIT] YF-TM02 sur GPIO" + String(FLOW_SENSOR_PIN));

    // --- WiFi ---
    connectWiFi();
}

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
        Serial.println("\n[WiFi] ✗ Échec de connexion — vérifiez SSID/mot de passe");
    }
}

// ============================================================
//  LECTURE HUMIDITÉ DU SOL (Capacitive V1.2)
//  Retourne un pourcentage 0–100 %
// ============================================================
float readSoilHumidity() {
    // Moyenne sur 10 lectures pour stabiliser
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(SOIL_SENSOR_PIN);
        delay(10);
    }
    int raw = sum / 10;

    // Convertir en pourcentage (sol sec = 0%, sol humide = 100%)
    float percent = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
    percent = constrain(percent, 0.0, 100.0);

    Serial.print("[SOL] ADC brut=" + String(raw) + " → ");
    Serial.println(String(percent, 1) + "%");
    return percent;
}

// ============================================================
//  LECTURE NIVEAU D'EAU (HC-SR04)
//  Retourne un pourcentage 0–100 %
// ============================================================
float readWaterLevel() {
    // Envoyer une impulsion TRIG de 10µs
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Mesurer la durée de l'écho (timeout 30ms)
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) {
        Serial.println("[ULTRASON] Timeout — aucun écho reçu");
        return -1.0;
    }

    // Distance en cm (vitesse du son = 343 m/s)
    float distanceCm = (duration * 0.0343) / 2.0;

    // Convertir en niveau d'eau (%)
    // Plus la distance est petite → réservoir plein → 100%
    float level = map(distanceCm * 10, RESERVOIR_FULL_CM * 10, RESERVOIR_EMPTY_CM * 10, 100, 0);
    level = constrain(level, 0.0, 100.0);

    Serial.print("[ULTRASON] Distance=" + String(distanceCm, 1) + "cm → Niveau=");
    Serial.println(String(level, 1) + "%");
    return level;
}

// ============================================================
//  CALCUL DÉBIT (YF-TM02)
//  Retourne les litres écoulés depuis le dernier appel
// ============================================================
float readFlowLiters() {
    // Désactiver l'interruption le temps de lire le compteur
    detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));
    unsigned long pulses = flowPulseCount;
    flowPulseCount = 0;
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseISR, RISING);

    // Convertir impulsions → litres
    float liters = (float)pulses / FLOW_CALIBRATION_FACTOR;
    totalLiters += liters;

    Serial.print("[DEBIT] Impulsions=" + String(pulses) + " → ");
    Serial.print(String(liters, 3) + "L ce cycle | Total=");
    Serial.println(String(totalLiters, 3) + "L");

    return liters;
}

// ============================================================
//  ENVOI DES DONNÉES VERS L'API
// ============================================================
void sendToAPI(float soilHumidity, float airTemperature, float airHumidity,
               float bmeTemperature, float bmePressure,
               float waterLevel, float liters) {

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[API] WiFi non connecté — tentative de reconnexion...");
        connectWiFi();
        return;
    }

    // Construire l'URL : POST /api/devices/{id}/sensors
    String url = String(API_BASE_URL) + "/devices/" + String(DEVICE_ID) + "/sensors";

    // Construire le JSON avec ArduinoJson
    StaticJsonDocument<512> doc;

    if (soilHumidity >= 0)   doc["soil_humidity"]   = round(soilHumidity * 10) / 10.0;
    if (airTemperature > -100) doc["air_temperature"] = round(airTemperature * 10) / 10.0;
    if (airHumidity >= 0)    doc["air_humidity"]    = round(airHumidity * 10) / 10.0;
    if (waterLevel >= 0)     doc["water_level"]     = round(waterLevel * 10) / 10.0;
    if (liters >= 0)         doc["liters"]          = round(liters * 1000) / 1000.0;

    // Champs supplémentaires BME280
    if (bmeAvailable && bmeTemperature > -100) {
        doc["bme_temperature"] = round(bmeTemperature * 10) / 10.0;
        doc["bme_pressure"]    = round(bmePressure * 10) / 10.0;
    }

    String payload;
    serializeJson(doc, payload);

    Serial.println("\n[API] Envoi vers : " + url);
    Serial.println("[API] Payload    : " + payload);

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept",       "application/json");
    http.addHeader("Authorization", String("Bearer ") + API_TOKEN);
    http.setTimeout(10000); // timeout 10s

    int httpCode = http.POST(payload);

    Serial.print("[API] Code HTTP  : ");
    Serial.println(httpCode);

    if (httpCode > 0) {
        String response = http.getString();
        Serial.println("[API] Réponse    : " + response);
        if (httpCode == 200 || httpCode == 201) {
            Serial.println("[API] ✓ Données envoyées avec succès");
        } else {
            Serial.println("[API] ✗ Erreur serveur — code " + String(httpCode));
        }
    } else {
        Serial.println("[API] ✗ Erreur HTTP : " + http.errorToString(httpCode));
    }

    http.end();
}

// ============================================================
//  LOOP PRINCIPAL
// ============================================================
void loop() {
    unsigned long now = millis();

    if (now - lastSendTime >= SEND_INTERVAL_MS) {
        lastSendTime = now;

        Serial.println("\n--- Lecture des capteurs ---");

        // 1. DHT11 — Température et humidité de l'air
        float dhtTemp = dht.readTemperature();
        float dhtHum  = dht.readHumidity();
        if (isnan(dhtTemp) || isnan(dhtHum)) {
            Serial.println("[DHT11] ✗ Lecture invalide");
            dhtTemp = -999.0;
            dhtHum  = -1.0;
        } else {
            Serial.print("[DHT11] Temp=" + String(dhtTemp, 1) + "°C");
            Serial.println("  Hum=" + String(dhtHum, 1) + "%");
        }

        // 2. BME280 — Température, humidité, pression
        float bmeTemp     = -999.0;
        float bmeHum      = -1.0;
        float bmePressure = -1.0;
        if (bmeAvailable) {
            bmeTemp     = bme.readTemperature();
            bmeHum      = bme.readHumidity();
            bmePressure = bme.readPressure() / 100.0F; // hPa
            Serial.print("[BME280] Temp=" + String(bmeTemp, 1) + "°C");
            Serial.print("  Hum=" + String(bmeHum, 1) + "%");
            Serial.println("  Pression=" + String(bmePressure, 1) + "hPa");
        }

        // 3. Capteur sol capacitif
        float soilHum = readSoilHumidity();

        // 4. Ultrason — Niveau d'eau
        float waterLvl = readWaterLevel();

        // 5. Débitmètre YF-TM02
        float liters = readFlowLiters();

        // Choisir la meilleure source pour air_temperature et air_humidity :
        // BME280 est plus précis que DHT11 si disponible
        float airTemp = bmeAvailable ? bmeTemp : dhtTemp;
        float airHum  = bmeAvailable ? bmeHum  : dhtHum;

        // Envoi vers l'API
        sendToAPI(soilHum, airTemp, airHum, bmeTemp, bmePressure, waterLvl, liters);
    }

    delay(100); // Petit délai pour ne pas saturer le CPU
}
