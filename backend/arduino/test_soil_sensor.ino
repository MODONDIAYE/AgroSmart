/*
 * ============================================================
 *  AgroSmart — ESP32 + Capteur Sol Capacitif V1.2
 *                    + Débitmètre YF-TM02
 *                    + Sonde température sol DS18B20
 *
 *  Endpoint : POST /api/devices/{DEVICE_ID}/sensors
 *  Table    : sensor_data
 *
 *  Bibliothèques requises (Arduino Library Manager) :
 *    - ArduinoJson          (Benoit Blanchon, v6.x)
 *    - OneWire              (Paul Stoffregen)
 *    - DallasTemperature    (Miles Burton)
 * ============================================================
 *
 *  BRANCHEMENT :
 *
 *  Capteur Sol Capacitif V1.2 :
 *    VCC  → 3.3V
 *    GND  → GND
 *    AOUT → GPIO34
 *
 *  Débitmètre YF-TM02 :
 *    VCC    → 5V
 *    GND    → GND
 *    Signal → GPIO19  (interrupt)
 *
 *  Sonde DS18B20 (température sol) :
 *    VCC (fil rouge)  → 3.3V
 *    GND (fil noir)   → GND
 *    DATA (fil jaune) → GPIO23
 *    + Résistance pull-up 4.7kΩ entre DATA et VCC
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ============================================================
//  CONFIGURATION WiFi + API
// ============================================================
const char* WIFI_SSID     = "Etudiants_ISM";
const char* WIFI_PASSWORD = "Etudiant123&";
const char* API_BASE_URL  = "http://172.16.6.224:8000/api";
const int   DEVICE_ID     = 3;   // AgroDevice (id=3, user mariama)
const char* API_TOKEN     = "3|esbLbVL4XfgmpJ7Grx8DEdpWeU6RTKAd8yDpoGTZ6f094cab";

// Intervalle d'envoi
const unsigned long SEND_INTERVAL_MS = 10000; // 10 secondes

// ============================================================
//  CAPTEUR SOL CAPACITIF — GPIO34
// ============================================================
#define SOIL_PIN  34
#define SOIL_DRY  304    // ADC dans l'air  → 0%   ✅ mesuré
#define SOIL_WET   77    // ADC dans l'eau  → 100% ✅ mesuré

// ============================================================
//  DÉBITMÈTRE YF-TM02 — GPIO19
// ============================================================
#define FLOW_PIN          19
#define FLOW_CALIBRATION  450.0  // impulsions par litre (YF-TM02 typique)

volatile unsigned long flowPulseCount = 0;
float totalLiters = 0.0;

void IRAM_ATTR flowPulseISR() {
    flowPulseCount++;
}

// ============================================================
//  SONDE TEMPÉRATURE SOL DS18B20 — GPIO23
// ============================================================
#define DS18B20_PIN  23

OneWire           oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

// ============================================================
//  VARIABLES GLOBALES
// ============================================================
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
        Serial.println("\n[WiFi] ✓ Connecté — IP : " + WiFi.localIP().toString());
    } else {
        Serial.println("\n[WiFi] ✗ Échec connexion");
    }
}

// ============================================================
//  LECTURE HUMIDITÉ DU SOL (Capacitif V1.2)
// ============================================================
float readSoilHumidity() {
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(SOIL_PIN);
        delay(10);
    }
    int raw = sum / 10;
    float voltage = raw * (3.3 / 4095.0);
    float percent = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
    percent = constrain(percent, 0.0, 100.0);

    String etat;
    if (percent < 20)      etat = "Sol tres sec";
    else if (percent < 40) etat = "Sol sec";
    else if (percent < 60) etat = "Humidite moderee";
    else if (percent < 80) etat = "Sol humide";
    else                   etat = "Sol sature";

    Serial.println("[SOL]     ADC=" + String(raw) +
                   "  Tension=" + String(voltage, 3) + "V" +
                   "  Humidite=" + String(percent, 1) + "%" +
                   "  -> " + etat);
    return percent;
}

// ============================================================
//  LECTURE DÉBIT (YF-TM02)
//  Retourne les litres écoulés depuis le dernier appel
// ============================================================
float readFlowLiters() {
    detachInterrupt(digitalPinToInterrupt(FLOW_PIN));
    unsigned long pulses = flowPulseCount;
    flowPulseCount = 0;
    attachInterrupt(digitalPinToInterrupt(FLOW_PIN), flowPulseISR, RISING);

    float liters = (float)pulses / FLOW_CALIBRATION;
    totalLiters += liters;

    Serial.println("[DEBIT]   Impulsions=" + String(pulses) +
                   "  Ce cycle=" + String(liters, 4) + "L" +
                   "  Total=" + String(totalLiters, 3) + "L");
    return liters;
}

// ============================================================
//  LECTURE TEMPÉRATURE SOL (DS18B20)
// ============================================================
float readSoilTemperature() {
    ds18b20.requestTemperatures();
    float tempC = ds18b20.getTempCByIndex(0);

    if (tempC == DEVICE_DISCONNECTED_C) {
        Serial.println("[DS18B20] Capteur non detecte — verifiez cablage + resistance 4.7kOhm");
        return -999.0;
    }

    Serial.println("[DS18B20] Temperature sol = " + String(tempC, 2) + " C");
    return tempC;
}

// ============================================================
//  ENVOI VERS L'API
// ============================================================
void sendToAPI(float soilHumidity, float flowLiters, float soilTemp) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[API] WiFi perdu — reconnexion...");
        connectWiFi();
        return;
    }

    String url = String(API_BASE_URL) + "/devices/" + String(DEVICE_ID) + "/sensors";

    StaticJsonDocument<256> doc;

    // Humidite sol capacitif → sensor_data : Capacitive_Soil_Humidity
    if (soilHumidity >= 0)
        doc["soil_humidity"] = round(soilHumidity * 10) / 10.0;

    // Debit → sensor_data : Flowmeter_Liters
    if (flowLiters >= 0)
        doc["liters"] = round(flowLiters * 10000) / 10000.0;

    // Temperature sol DS18B20 → sensor_data : BME280_Temperature
    // (affiché sur la carte "Température" du dashboard)
    if (soilTemp > -100)
        doc["air_temperature"] = round(soilTemp * 10) / 10.0;

    String payload;
    serializeJson(doc, payload);

    Serial.println("[API] Envoi -> " + url);
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
        Serial.println("[API] Reponse : " + response);
        if (httpCode == 200 || httpCode == 201) {
            Serial.println("[API] Donnees enregistrees dans sensor_data");
        } else {
            Serial.println("[API] Erreur serveur — code " + String(httpCode));
        }
    } else {
        Serial.println("[API] Erreur HTTP : " + http.errorToString(httpCode));
    }

    http.end();
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    analogReadResolution(12);

    Serial.println("========================================");
    Serial.println("  AgroSmart ESP32");
    Serial.println("  Sol Capacitif + Debit + DS18B20");
    Serial.println("========================================");

    // Capteur sol
    Serial.println("[SOL]     GPIO34 | DRY=" + String(SOIL_DRY) + " WET=" + String(SOIL_WET));

    // Debitmetre
    pinMode(FLOW_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FLOW_PIN), flowPulseISR, RISING);
    Serial.println("[DEBIT]   GPIO19 | Calibration=" + String(FLOW_CALIBRATION) + " imp/L");

    // DS18B20
    ds18b20.begin();
    int nbSensors = ds18b20.getDeviceCount();
    Serial.println("[DS18B20] GPIO23 | " + String(nbSensors) + " sonde(s) detectee(s)");
    if (nbSensors == 0) {
        Serial.println("[DS18B20] Aucune sonde — verifiez cablage + resistance 4.7kOhm");
    }

    connectWiFi();
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
        lastSendTime = millis();

        Serial.println("\n--- Lecture des capteurs ---");

        float soilHum  = readSoilHumidity();
        float flow     = readFlowLiters();
        float soilTemp = readSoilTemperature();

        sendToAPI(soilHum, flow, soilTemp);
    }

    delay(100);
}
