// ============================================================
//  AgroSmart — ESP32 Code Final
//
//  DEUX MODES DE CONTRÔLE (mode auto supprimé) :
//
//  MODE 1 — PHYSIQUE (Switch PIN 32, priorité absolue)
//    → Switch ON  : relais activé immédiatement
//    → Switch OFF : relais coupé, passe en mode App
//
//  MODE 2 — APPLICATION (boutons Dashboard)
//    → Bouton "Irriguer" → pump_command = 1 → pompe ON
//    → Bouton "Arrêter"  → pump_command = 0 → pompe OFF
//    → L'ESP32 lit la réponse JSON toutes les 5s et applique
// ============================================================

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ─── CONFIGURATION ───────────────────────────────────────────
const char* SSID     = "Etudiants_ISM";
const char* PASSWORD = "Etudiant123&";
const char* API_URL  = "http://192.30.0.54:8000/api/devices/3/sensors";
const char* TOKEN    = "19|u8UlaD1fse3H0ZpnNaPNy6A0oD61OLCowOzFdfD5f16182a0";

// ─── PINS ────────────────────────────────────────────────────
#define SOIL_PIN   35
#define FLOW_PIN   18
#define TEMP_PIN   26
#define DHTPIN     21
#define RELAY_PIN  23
#define SWITCH_PIN 32
#define TRIG_PIN   14
#define ECHO_PIN   25

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ─── ÉTALONNAGE ──────────────────────────────────────────────
const int   SOIL_DRY   = 2670;
const int   SOIL_WET   = 2540;
const float CAL_FACTOR = 7.5;
const int   TANK_DEPTH = 10;

// ─── VARIABLES ───────────────────────────────────────────────
volatile int  pulseCount  = 0;
float         totalLitres = 0.0;
float         flowRate    = 0.0;

unsigned long lastSensorMillis = 0;
unsigned long lastAPIMillis    = 0;

bool pumpStatus      = false;
bool switchPrecedent = false;

int   humSol     = 0;
int   rawSoil    = 0;
bool  soilError  = false;
int   waterLevel = 0;
float tempSol    = -127.0;
float humAir     = 0.0;
float tempAir    = 0.0;

OneWire           oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void IRAM_ATTR pulseCounter() { pulseCount++; }

// ─── CONTRÔLE POMPE ──────────────────────────────────────────
void setPump(bool on) {
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  pumpStatus = on;
  Serial.printf("🔌 RELAIS → %s\n", on ? "ON" : "OFF");
}

// ─── CONNEXION WIFI ──────────────────────────────────────────
void connecterWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.printf("\n📶 Connexion à %s", SSID);
  WiFi.begin(SSID, PASSWORD);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 30) {
    delay(500);
    Serial.print(".");
    t++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n✅ WiFi OK — IP : %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n⚠️ WiFi indisponible — mode physique uniquement.");
  }
}

// ─── NIVEAU EAU ──────────────────────────────────────────────
int getWaterLevelPercent() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duree = pulseIn(ECHO_PIN, HIGH, 10000);
  if (duree == 0) return 0;
  int distance = duree * 0.034 / 2;
  return constrain(map(distance, TANK_DEPTH, 3, 0, 100), 0, 100);
}

// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== AgroSmart ESP32 démarré ===");

  pinMode(RELAY_PIN, OUTPUT);
  setPump(false);

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  switchPrecedent = !digitalRead(SWITCH_PIN);

  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SOIL_PIN, INPUT);

  sensors.begin();
  sensors.setWaitForConversion(false);
  dht.begin();

  connecterWiFi();

  lastSensorMillis = millis();
  lastAPIMillis    = millis();
}

// ─────────────────────────────────────────────────────────────
void loop() {

  // ════════════════════════════════════════════════════════════
  //  MODE 1 — SWITCH PHYSIQUE (priorité absolue)
  //  Détection immédiate du changement de position
  // ════════════════════════════════════════════════════════════
  bool switchActif = !digitalRead(SWITCH_PIN);

  if (switchActif != switchPrecedent) {
    switchPrecedent = switchActif;
    if (switchActif) {
      setPump(true);
      Serial.println("🚨 [PHYSIQUE] Switch ON → Pompe ALLUMÉE");
    } else {
      setPump(false);
      Serial.println("🛑 [PHYSIQUE] Switch OFF → Pompe ÉTEINTE");
    }
  }

  unsigned long now = millis();

  // ════════════════════════════════════════════════════════════
  //  LECTURE CAPTEURS — toutes les 1 seconde
  // ════════════════════════════════════════════════════════════
  if (now - lastSensorMillis >= 1000) {
    unsigned long dt = now - lastSensorMillis;
    lastSensorMillis = now;

    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();
    flowRate    = (pulses / CAL_FACTOR) * (60000.0f / (float)dt);
    totalLitres += ((float)pulses / CAL_FACTOR) / 60.0f;

    rawSoil = analogRead(SOIL_PIN);
    if (rawSoil < 300) {
      soilError = true;
    } else {
      soilError = false;
      humSol = constrain(map(rawSoil, SOIL_DRY, SOIL_WET, 0, 100), 0, 100);
    }

    waterLevel = getWaterLevelPercent();
    tempSol    = sensors.getTempCByIndex(0);
    sensors.requestTemperatures();
    humAir  = dht.readHumidity();
    tempAir = dht.readTemperature();

    Serial.print("🌱 Sol: ");
    soilError ? Serial.print("ERR") : (Serial.print(humSol), Serial.print("%"));
    Serial.print(" | 🌡️ Tsol: ");
    (tempSol != -127.0) ? (Serial.print(tempSol,1), Serial.print("°C")) : Serial.print("ERR");
    Serial.print(" | 💨 Air: ");
    isnan(tempAir) ? Serial.print("?") : (Serial.print(tempAir,1), Serial.print("°C "));
    isnan(humAir)  ? Serial.print("?%") : (Serial.print(humAir,0), Serial.print("%"));
    Serial.print(" | 🛢️ Cuve: ");   Serial.print(waterLevel);    Serial.print("%");
    Serial.print(" | 💧 Débit: ");  Serial.print(flowRate, 1);   Serial.print("L/m");
    Serial.print(" | Total: ");     Serial.print(totalLitres, 2); Serial.print("L");
    Serial.print(" | SW: ");        Serial.print(switchActif ? "ON" : "OFF");
    Serial.print(" | POMPE: ");     Serial.println(pumpStatus ? "ON ✅" : "OFF ⛔");
  }

  // ════════════════════════════════════════════════════════════
  //  MODE 2 — APPLICATION (toutes les 5 secondes)
  //
  //  L'ESP32 envoie ses données à Laravel.
  //  Laravel répond pump_command selon le DERNIER bouton cliqué :
  //    • "Irriguer" → pump_command = 1 → setPump(true)
  //    • "Arrêter"  → pump_command = 0 → setPump(false)
  //
  //  Actif UNIQUEMENT si switch physique est sur OFF
  // ════════════════════════════════════════════════════════════
  if (now - lastAPIMillis >= 5000) {
    lastAPIMillis = now;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ WiFi perdu — reconnexion...");
      connecterWiFi();
      return;
    }

    // Payload envoyé à Laravel
    StaticJsonDocument<512> jsonDoc;
    if (!soilError)        jsonDoc["soil_humidity"]    = humSol;
    if (tempSol != -127.0) jsonDoc["soil_temperature"] = tempSol;
    if (!isnan(tempAir))   jsonDoc["air_temperature"]  = tempAir;
    if (!isnan(humAir))    jsonDoc["air_humidity"]     = humAir;
    jsonDoc["water_level"]  = waterLevel;
    jsonDoc["pump_status"]  = pumpStatus ? 1 : 0;
    jsonDoc["flow_rate"]    = round(flowRate    * 10.0f) / 10.0f;
    jsonDoc["total_volume"] = round(totalLitres * 100.0f) / 100.0f;
    jsonDoc["manual_mode"]  = switchActif ? 1 : 0;

    String payload;
    serializeJson(jsonDoc, payload);

    HTTPClient http;
    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + TOKEN);
    http.setTimeout(4000);

    int httpCode = http.POST(payload);
    Serial.printf("📡 HTTP %d", httpCode);

    if (httpCode == 200 || httpCode == 201) {
      String reponse = http.getString();
      StaticJsonDocument<256> reponseDoc;

      if (deserializeJson(reponseDoc, reponse) == DeserializationError::Ok) {

        if (!switchActif && reponseDoc.containsKey("pump_command")) {
          // Switch OFF → on applique l'ordre du Dashboard
          int cmd = reponseDoc["pump_command"];
          Serial.printf(" | App→ cmd=%d pompe=%s", cmd, pumpStatus ? "ON" : "OFF");

          if (cmd == 1 && !pumpStatus) {
            setPump(true);
            Serial.println("\n   ✅ [APP] Irriguer → Pompe démarrée");
          } else if (cmd == 0 && pumpStatus) {
            setPump(false);
            Serial.println("\n   ✅ [APP] Arrêter → Pompe coupée");
          } else {
            Serial.println(" | ℹ️ Déjà dans l'état demandé");
          }

        } else if (switchActif) {
          Serial.println(" | ⚙️ Switch ON — commande App ignorée");
        }

      } else {
        Serial.println(" | ⚠️ JSON invalide");
      }
    } else {
      Serial.printf("\n❌ Erreur HTTP %d\n", httpCode);
    }

    http.end();
  }
}
