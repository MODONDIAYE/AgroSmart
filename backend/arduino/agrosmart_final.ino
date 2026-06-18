/*
 * ============================================================
 *  AgroSmart — ESP32 Code Final Global (Version DHT11)
 *  Carte    : LILYGO ESP32 (Profil : ESP32 Dev Module)
 *  Capteurs : Capacitif Sol, DS18B20, YF-TM02, HC-SR04, DHT11
 *
 *  Endpoint : POST /api/devices/{DEVICE_ID}/sensors
 *  Table    : sensor_data
 *
 *  Champs envoyés à l'API :
 *    soil_humidity    → Capacitive_Soil_Humidity   (%)
 *    soil_temperature → DS18B20_Soil_Temperature   (°C)
 *    air_temperature  → BME280_Temperature         (°C)  [DHT11]
 *    air_humidity     → BME280_Humidity            (%)   [DHT11]
 *    water_level      → Ultrasonic_Water_Level     (%)
 *    flow_rate        → Flowmeter_Liters           (L/min)
 *    total_volume     → Flowmeter_Total_Volume     (L)
 *
 *  Bibliothèques requises (Arduino Library Manager) :
 *    - ArduinoJson          (Benoit Blanchon, v6.x)
 *    - OneWire              (Paul Stoffregen)
 *    - DallasTemperature    (Miles Burton)
 *    - DHT sensor library   (Adafruit)
 *    - Adafruit Unified Sensor
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ============================================================
//  CONFIGURATION
// ============================================================
const char* SSID    = "Etudiants_ISM";
const char* PASS    = "Etudiant123&";
const char* API_URL = "http://172.20.61.163:8000/api/devices/3/sensors"; // ← espace retiré
const char* TOKEN   = "7|sLBdWts9JCxrf2hfe15VUT8e0DowQpgQNcRd9CzE61640e6b";

// Intervalle d'envoi (ms)
const unsigned long SEND_INTERVAL_MS = 3000;

// ============================================================
//  PINS
// ============================================================
#define RELAY_PIN   25
#define BUTTON_PIN  32   // Interrupteur physique (INPUT_PULLUP → LOW = actif)
#define SOIL_PIN    36   // GPIO36 (VP) — input only, ADC1   // Capteur sol capacitif (ADC)
#define FLOW_PIN    33   // Débitmètre YF-TM02 (interrupt)
#define TEMP_PIN    26   // DS18B20 température sol
#define TRIG_PIN    13   // HC-SR04 TRIG
#define ECHO_PIN    14   // HC-SR04 ECHO
#define DHT_PIN     4    // DHT11 DATA — GPIO4 (GPIO19/21 = SPI/I2C réservés sur LILYGO)

// ============================================================
//  CALIBRATION
// ============================================================
const int   SOIL_DRY   = 3930;  // ADC sol sec  → 0%
const int   SOIL_WET   = 1800;  // ADC sol mouillé → 100%
const float CAL_FACTOR = 7.5;   // Impulsions/s par L/min (YF-TM02)
const int   TANK_DEPTH = 10;    // Distance cm quand cuve vide
const int   TANK_FULL  = 5;     // Distance cm quand cuve pleine

// ============================================================
//  VARIABLES GLOBALES
// ============================================================
volatile int  pulseCount = 0;
float         totalLitres = 0.0;
unsigned long lastMillis  = 0;
bool          appStatus   = false;

// ============================================================
//  OBJETS CAPTEURS
// ============================================================
OneWire           oneWire(TEMP_PIN);
DallasTemperature tempSensors(&oneWire);
DHT               dht(DHT_PIN, DHT11);

// ============================================================
//  ISR — Débitmètre
// ============================================================
void IRAM_ATTR pulseCounter() {
    pulseCount++;
}

// ============================================================
//  LECTURE NIVEAU D'EAU (HC-SR04)
//  Retourne 0–100 % (100 = plein)
// ============================================================
int getWaterLevel() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return 0; // Timeout → cuve vide ou capteur débranché

    int distance = duration * 0.034 / 2;
    int level    = map(distance, TANK_DEPTH, TANK_FULL, 0, 100);
    return constrain(level, 0, 100);
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);

    pinMode(RELAY_PIN,  OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    pinMode(FLOW_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Démarrage capteurs
    tempSensors.begin();
    tempSensors.setWaitForConversion(false);
    dht.begin();

    // Connexion WiFi
    WiFi.begin(SSID, PASS);
    Serial.print("[WiFi] Connexion");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ AGROSMART PRET - MODE DHT11 ACTIF");
    Serial.println("IP: " + WiFi.localIP().toString());
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    // Lecture interrupteur physique (sans délai)
    bool physicalSwitch = (digitalRead(BUTTON_PIN) == LOW);

    if (millis() - lastMillis >= SEND_INTERVAL_MS) {
        lastMillis = millis();

        // --- Étape 1 : DHT11 (température et humidité air) ---
        Serial.println("\n[1] Lecture DHT11 (Air)...");
        float humAir  = dht.readHumidity();
        float tempAir = dht.readTemperature();

        // --- Étape 2 : DS18B20 (température sol) ---
        Serial.println("[2] Lecture DS18B20 (Sol)...");
        tempSensors.requestTemperatures();
        float tempSol = tempSensors.getTempCByIndex(0);

        // --- Étape 3 : Humidité sol capacitif ---
        Serial.println("[3] Lecture Humidité Sol...");
        int humSol = constrain(map(analogRead(SOIL_PIN), SOIL_DRY, SOIL_WET, 0, 100), 0, 100);

        // --- Étape 4 : Niveau d'eau (ultrason) ---
        Serial.println("[4] Lecture Ultrasons...");
        int waterLevel = getWaterLevel();

        // --- Débitmètre (lecture atomique) ---
        noInterrupts();
        int pulses = pulseCount;
        pulseCount = 0;
        interrupts();

        float flowRate   = (float)pulses / CAL_FACTOR;                          // L/min
        float cycleVol   = (flowRate / 60.0) * (SEND_INTERVAL_MS / 1000.0);    // L ce cycle
        totalLitres     += cycleVol;

        // --- Étape 5 : Envoi API ---
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[5] Envoi au serveur Laravel...");

            HTTPClient http;
            http.begin(API_URL);
            http.addHeader("Content-Type",  "application/json");
            http.addHeader("Accept",        "application/json");
            http.addHeader("Authorization", String("Bearer ") + TOKEN);
            http.setTimeout(8000);

            // Construction JSON
            StaticJsonDocument<400> doc;
            doc["soil_humidity"] = humSol;
            doc["flow_rate"]     = round(flowRate * 100) / 100.0;
            doc["total_volume"]  = round(totalLitres * 1000) / 1000.0;
            doc["water_level"]   = waterLevel;

            // DS18B20 : n'envoyer que si sonde connectée (-127°C = débranchée)
            if (tempSol > -100.0) {
                doc["soil_temperature"] = round(tempSol * 10) / 10.0;
            }

            // DHT11 : n'envoyer que si lecture valide
            if (!isnan(tempAir)) {
                doc["air_temperature"] = round(tempAir * 10) / 10.0;
            }
            if (!isnan(humAir)) {
                doc["air_humidity"] = round(humAir * 10) / 10.0;
            }

            String jsonPayload;
            serializeJson(doc, jsonPayload);
            Serial.println("[API] → " + jsonPayload);

            int httpCode = http.POST(jsonPayload);
            Serial.printf("[6] Réponse HTTP = %d\n", httpCode);

            if (httpCode == 200 || httpCode == 201) {
                String response = http.getString();
                StaticJsonDocument<300> respDoc;
                if (deserializeJson(respDoc, response) == DeserializationError::Ok) {
                    appStatus = respDoc["irrigation"] | false;
                }

                // Affichage récapitulatif
                Serial.println("--- SYNCHRO REUSSIE ---");
                if (isnan(tempAir)) {
                    Serial.println("⚠️  DHT11 : lecture invalide");
                } else {
                    Serial.printf("🌤️  Air:  %.1f°C | 💧 Hum Air: %.1f%%\n", tempAir, humAir);
                }
                if (tempSol <= -100.0) {
                    Serial.println("⚠️  DS18B20 : capteur débranché");
                } else {
                    Serial.printf("🌡️  Sol:  %.1f°C | 🌱 Hum Sol: %d%%\n", tempSol, humSol);
                }
                Serial.printf("🛢️  Cuve: %d%%\n", waterLevel);
                Serial.printf("💧 Débit: %.2f L/min | 📊 Vol Total: %.3f L\n", flowRate, totalLitres);
                Serial.printf("🚀 Pompe App: %s | 🔘 Interrupteur: %s\n",
                              appStatus ? "ON" : "OFF",
                              physicalSwitch ? "ON" : "OFF");

            } else {
                Serial.printf("❌ Erreur HTTP : %s\n", http.errorToString(httpCode).c_str());
            }

            http.end();

        } else {
            Serial.println("❌ WiFi Déconnecté — tentative reconnexion...");
            WiFi.reconnect();
        }
    }

    // Commande relais (App OU interrupteur physique)
    digitalWrite(RELAY_PIN, (appStatus || physicalSwitch) ? HIGH : LOW);

    delay(50);
}
