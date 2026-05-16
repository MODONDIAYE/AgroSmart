/*
 * ============================================================
 *  AgroSmart — ESP32 Final
 *  Capteurs : Capacitif Sol, DS18B20, YF-TM02, HC-SR04
 *
 *  Endpoint : POST /api/devices/{DEVICE_ID}/sensors
 *  Table    : sensor_data
 *
 *  Champs envoyés à l'API :
 *    soil_humidity    → Capacitive_Soil_Humidity   (%)
 *    soil_temperature → DS18B20_Soil_Temperature   (°C)
 *    water_level      → Ultrasonic_Water_Level      (%)
 *    flow_rate        → Flowmeter_Liters            (L/min)
 *    total_volume     → Flowmeter_Total_Volume      (L)
 *
 *  Bibliothèques requises :
 *    - ArduinoJson       (Benoit Blanchon, v6.x)
 *    - OneWire           (Paul Stoffregen)
 *    - DallasTemperature (Miles Burton)
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

// ============================================================
//  CONFIGURATION
// ============================================================
const char* SSID    = "Etudiants_ISM";
const char* PASS    = "Etudiant123&";
const char* API_URL = "http://172.16.8.178:8000/api/devices/3/sensors";
const char* TOKEN   = "6|80sQI3KbWiyd3s1TV1RWE87zb1WVjELTRvLEYwu0c2807a35";

// Intervalle d'envoi (ms)
const unsigned long SEND_INTERVAL_MS = 3000;

// ============================================================
//  PINS
// ============================================================
#define RELAY_PIN   25
#define BUTTON_PIN  32   // Interrupteur physique (INPUT_PULLUP → LOW = actif)
#define SOIL_PIN    35
#define FLOW_PIN    33
#define TEMP_PIN    26
#define TRIG_PIN    13
#define ECHO_PIN    14

// ============================================================
//  CALIBRATION
// ============================================================
const int   SOIL_DRY   = 3930;  // ADC sol sec  → 0%
const int   SOIL_WET   = 1800;  // ADC sol mouillé → 100%
const float CAL_FACTOR = 7.5;   // Impulsions/seconde par L/min (YF-TM02)
const int   TANK_DEPTH = 30;    // Distance cm quand cuve vide (capteur → fond)
const int   TANK_FULL  = 4;     // Distance cm quand cuve pleine

// ============================================================
//  VARIABLES GLOBALES
// ============================================================
volatile int pulseCount = 0;
float        totalLitres = 0.0;
unsigned long lastMillis = 0;
bool         appStatus   = false;

OneWire           oneWire(TEMP_PIN);
DallasTemperature tempSensors(&oneWire);

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
    // Plus la distance est grande → niveau bas → 0%
    // Plus la distance est petite → niveau haut → 100%
    int level = map(distance, TANK_DEPTH, TANK_FULL, 0, 100);
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

    tempSensors.begin();

    WiFi.begin(SSID, PASS);
    Serial.print("[WiFi] Connexion");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ AGROSMART PRÊT — IP: " + WiFi.localIP().toString());
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    // Lecture interrupteur physique (sans délai)
    bool physicalSwitch = (digitalRead(BUTTON_PIN) == LOW);

    if (millis() - lastMillis >= SEND_INTERVAL_MS) {
        lastMillis = millis();

        // --- Lecture capteurs ---
        tempSensors.requestTemperatures();
        float tempSol = tempSensors.getTempCByIndex(0);

        int soilHum   = constrain(map(analogRead(SOIL_PIN), SOIL_DRY, SOIL_WET, 0, 100), 0, 100);
        int waterLevel = getWaterLevel();

        // Débitmètre : capture atomique des impulsions
        noInterrupts();
        int pulses = pulseCount;
        pulseCount = 0;
        interrupts();

        // Débit en L/min et volume du cycle (3s)
        float flowRate   = (float)pulses / CAL_FACTOR;           // L/min
        float cycleVolume = (flowRate / 60.0) * (SEND_INTERVAL_MS / 1000.0); // L sur ce cycle
        totalLitres += cycleVolume;

        // --- Affichage série ---
        Serial.println("\n==================================================");
        if (tempSol <= -100.0) {
            Serial.println("⚠️  Temp Sol: CAPTEUR DÉBRANCHÉ");
        } else {
            Serial.printf("🌡️  Temp Sol:   %.1f °C\n", tempSol);
        }
        Serial.printf("🌱 Hum Sol:    %d %%\n",       soilHum);
        Serial.printf("🛢️  Niveau Cuve: %d %%\n",     waterLevel);
        Serial.printf("💧 Débit:       %.2f L/min\n", flowRate);
        Serial.printf("📊 Vol Total:   %.3f L\n",     totalLitres);
        Serial.printf("🚀 Pompe App:   %s | 🔘 Interrupteur: %s\n",
                      appStatus ? "ON" : "OFF",
                      physicalSwitch ? "ON" : "OFF");
        Serial.println("==================================================");

        // --- Envoi API ---
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(API_URL);
            http.addHeader("Content-Type",  "application/json");
            http.addHeader("Accept",        "application/json");
            http.addHeader("Authorization", String("Bearer ") + TOKEN);
            http.setTimeout(8000);

            // Construction du JSON
            StaticJsonDocument<256> doc;
            doc["soil_humidity"] = soilHum;
            doc["water_level"]   = waterLevel;
            doc["flow_rate"]     = round(flowRate * 100) / 100.0;
            doc["total_volume"]  = round(totalLitres * 1000) / 1000.0;

            // DS18B20 : n'envoyer que si la sonde est connectée
            // -127°C = DEVICE_DISCONNECTED, -85°C = erreur de lecture
            if (tempSol > -100.0) {
                doc["soil_temperature"] = round(tempSol * 10) / 10.0;
            }

            String payload;
            serializeJson(doc, payload);

            Serial.println("[API] → " + payload);

            int httpCode = http.POST(payload);
            Serial.printf("[API] Code: %d\n", httpCode);

            if (httpCode == 200 || httpCode == 201) {
                String response = http.getString();
                StaticJsonDocument<256> resp;
                if (deserializeJson(resp, response) == DeserializationError::Ok) {
                    appStatus = resp["irrigation"] | false;
                    Serial.printf("[API] ✓ irrigation=%s\n", appStatus ? "true" : "false");
                }
            } else if (httpCode > 0) {
                Serial.println("[API] ✗ Erreur serveur: " + String(httpCode));
            } else {
                Serial.println("[API] ✗ Erreur réseau: " + http.errorToString(httpCode));
            }

            http.end();
        } else {
            Serial.println("[WiFi] Déconnecté — tentative de reconnexion...");
            WiFi.reconnect();
        }
    }

    // Commande relais (App OU interrupteur physique)
    digitalWrite(RELAY_PIN, (appStatus || physicalSwitch) ? HIGH : LOW);

    delay(50);
}
