/*
 * ============================================================
 *  AgroSmart — ESP32 Code Final Global (Version DHT11)
 *  Carte : LILYGO ESP32 (Profil : ESP32 Dev Module)
 *  Capteurs : Capacitif Sol, DS18B20, YF-TM02, HC-SR04, DHT11
 * ============================================================
 *
 *  CÂBLAGE DHT11 :
 *    VCC  → 3.3V
 *    GND  → GND
 *    DATA → GPIO19
 *    + Résistance pull-up 4.7kΩ entre DATA et VCC (OBLIGATOIRE)
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <DHT.h>

// --- CONFIGURATION ---
const char* SSID    = "Etudiants_ISM";
const char* PASS    = "Etudiant123&";
const char* API_URL = "http://172.16.7.228:8000/api/devices/3/sensors";
const char* TOKEN   = "10|f6dRzVrOa38v3oWTzUsuahT3VteuAa0HETnl4CF7d1d37669";

// --- PINS ---
#define RELAY_PIN   25
#define BUTTON_PIN  32
#define SOIL_PIN    36   // GPIO36 (VP) — input only, ADC1
#define FLOW_PIN    33
#define TEMP_PIN    26   // DS18B20 température sol
#define TRIG_PIN    13
#define ECHO_PIN    14
#define DHT_PIN     4    // DHT11 DATA — GPIO4 (GPIO19 = SPI MISO réservé sur LILYGO)

// --- TYPE DHT ---
#define DHTTYPE DHT11

// --- CALIBRATION ---
const int   SOIL_DRY   = 3930;
const int   SOIL_WET   = 1800;
const float CAL_FACTOR = 7.5;
const int   TANK_DEPTH = 10;  // Distance cm cuve vide
const int   TANK_FULL  = 5;   // Distance cm cuve pleine

// --- INTERVALLES ---
const unsigned long SEND_INTERVAL  = 10000; // Envoi toutes les 10s (laisse du temps au DHT)
const unsigned long DHT_INTERVAL   = 5000;  // Lecture DHT toutes les 5s

// --- VARIABLES GLOBALES ---
volatile int  pulseCount  = 0;
float         totalLitres = 0.0;
unsigned long lastMillis  = 0;
unsigned long lastDHTRead = 0;  // Timer dédié au DHT11
bool          appStatus   = false;

// Dernières valeurs DHT valides (cache)
float lastTempAir = NAN;
float lastHumAir  = NAN;

// --- OBJETS CAPTEURS ---
OneWire           oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
DHT               dht(DHT_PIN, DHTTYPE);

// --- ISR Débitmètre ---
void IRAM_ATTR pulseCounter() {
    pulseCount++;
}

// ============================================================
//  Niveau d'eau HC-SR04
// ============================================================
int getWaterLevel() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return 0;

    int distance = duration * 0.034 / 2;
    int level    = map(distance, TANK_DEPTH, TANK_FULL, 0, 100);
    return constrain(level, 0, 100);
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000); // Laisser le temps aux capteurs de démarrer

    pinMode(RELAY_PIN,  OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(FLOW_PIN,   INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // DS18B20
    sensors.begin();
    sensors.setWaitForConversion(true); // Attente bloquante = lectures fiables

    // DHT11 — délai de 2s après begin() avant première lecture
    dht.begin();
    Serial.println("[DHT11] Initialisation... attente 2s");
    delay(2000); // CRITIQUE : le DHT11 a besoin de ce délai au démarrage

    // Tentatives répétées pour initialiser le cache (jusqu'à 5 essais)
    for (int i = 0; i < 5; i++) {
        delay(2500);
        lastTempAir = dht.readTemperature();
        lastHumAir  = dht.readHumidity();
        lastDHTRead = millis();
        if (!isnan(lastTempAir) && !isnan(lastHumAir)) {
            Serial.printf("[DHT11] ✓ Temp=%.1f°C Hum=%.1f%%\n", lastTempAir, lastHumAir);
            break;
        }
        Serial.printf("[DHT11] Tentative %d/5 invalide...\n", i + 1);
    }

    if (isnan(lastTempAir) || isnan(lastHumAir)) {
        Serial.println("[DHT11] ⚠️  Toujours NaN — vérifiez GPIO4 + 3V3");
    }

    WiFi.begin(SSID, PASS);
    Serial.print("[WiFi] Connexion");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ AGROSMART PRÊT — MODE DHT11 ACTIF");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    unsigned long now = millis();
    bool physicalSwitch = (digitalRead(BUTTON_PIN) == LOW);

    // --- Lecture DHT11 sur son propre timer (indépendant du timer d'envoi) ---
    // Le DHT11 NaN vient souvent d'une lecture trop fréquente
    if (now - lastDHTRead >= DHT_INTERVAL) {
        lastDHTRead = now;

        float t = dht.readTemperature();
        float h = dht.readHumidity();

        if (!isnan(t) && !isnan(h)) {
            // Valeur valide → on met à jour le cache
            lastTempAir = t;
            lastHumAir  = h;
            Serial.printf("[DHT11] ✓ Temp=%.1f°C | Hum=%.1f%%\n", t, h);
        } else {
            // Valeur invalide → on garde le cache précédent
            Serial.println("[DHT11] ⚠️  Lecture invalide (NaN) — utilisation du cache précédent");
        }
    }

    // --- Bloc d'envoi toutes les 3s ---
    if (now - lastMillis >= SEND_INTERVAL) {
        lastMillis = now;

        // DS18B20
        sensors.requestTemperatures();
        float tempSol = sensors.getTempCByIndex(0);

        // Humidité sol
        int humSol = constrain(map(analogRead(SOIL_PIN), SOIL_DRY, SOIL_WET, 0, 100), 0, 100);

        // Ultrasons
        int waterLevel = getWaterLevel();

        // Débitmètre
        noInterrupts();
        int pulses = pulseCount;
        pulseCount = 0;
        interrupts();

        float flowRate = (float)pulses / CAL_FACTOR;              // L/min
        totalLitres   += (flowRate / 60.0) * (SEND_INTERVAL / 1000.0); // L sur ce cycle

        // --- Affichage série ---
        Serial.println("\n==================================================");
        Serial.printf("🌤️  Air:  %.1f°C | 💧 Hum Air:  %.1f%%\n",
                      isnan(lastTempAir) ? 0.0 : lastTempAir,
                      isnan(lastHumAir)  ? 0.0 : lastHumAir);
        if (tempSol <= -100.0) {
            Serial.println("🌡️  Sol:  CAPTEUR DS18B20 DÉBRANCHÉ");
        } else {
            Serial.printf("🌡️  Sol:  %.1f°C | 🌱 Hum Sol:  %d%%\n", tempSol, humSol);
        }
        Serial.printf("🛢️  Cuve: %d%%   | 💧 Débit: %.2f L/min | 📊 Total: %.3f L\n",
                      waterLevel, flowRate, totalLitres);
        Serial.printf("📱 Pompe App: %s | 🔘 Interrupteur: %s\n",
                      appStatus ? "ON" : "OFF", physicalSwitch ? "ON" : "OFF");
        Serial.println("==================================================");

        // --- Envoi API ---
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(API_URL);
            http.addHeader("Content-Type",  "application/json");
            http.addHeader("Accept",        "application/json");
            http.addHeader("Authorization", String("Bearer ") + TOKEN);
            http.setTimeout(5000); // réduit pour ne pas bloquer le DHT

            StaticJsonDocument<400> doc;
            doc["soil_humidity"] = humSol;
            doc["flow_rate"]     = round(flowRate * 100) / 100.0;
            doc["total_volume"]  = round(totalLitres * 1000) / 1000.0;
            doc["water_level"]   = waterLevel;

            // DS18B20 — seulement si connecté
            if (tempSol > -100.0) {
                doc["soil_temperature"] = round(tempSol * 10) / 10.0;
            }

            // DHT11 — on utilise le cache (dernière valeur valide)
            if (!isnan(lastTempAir)) {
                doc["air_temperature"] = round(lastTempAir * 10) / 10.0;
            }
            if (!isnan(lastHumAir)) {
                doc["air_humidity"] = round(lastHumAir * 10) / 10.0;
            }

            String jsonPayload;
            serializeJson(doc, jsonPayload);
            Serial.println("[API] → " + jsonPayload);

            int httpCode = http.POST(jsonPayload);
            Serial.printf("[API] Code HTTP: %d\n", httpCode);

            if (httpCode == 200 || httpCode == 201) {
                String response = http.getString();
                StaticJsonDocument<256> respDoc;
                if (deserializeJson(respDoc, response) == DeserializationError::Ok) {
                    appStatus = respDoc["irrigation"] | false;
                }
                Serial.printf("[API] ✓ irrigation=%s\n", appStatus ? "true" : "false");
            } else {
                Serial.printf("[API] ✗ Erreur: %s\n", http.errorToString(httpCode).c_str());
            }

            http.end();
        } else {
            Serial.println("[WiFi] ✗ Déconnecté — tentative reconnexion...");
            WiFi.reconnect();
        }
    }

    // --- Commande relais ---
    digitalWrite(RELAY_PIN, (appStatus || physicalSwitch) ? HIGH : LOW);

    delay(50);
}
