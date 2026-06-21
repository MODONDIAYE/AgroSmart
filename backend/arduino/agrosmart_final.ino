// ============================================================
//  AgroSmart — ESP32 Code Final
//  Logique de priorité :
//    1. Switch physique (Pin 32) → priorité absolue
//    2. Boutons Dashboard (Irriguer / Arrêter) → commande Web
//    3. Mode automatique → seuils culture définis dans Laravel
// ============================================================

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- CONFIGURATION WI-FI & API ---
const char* SSID     = "Etudiants_ISM";
const char* PASSWORD = "Etudiant123&";
const char* API_URL  = "http://192.30.0.54:8000/api/devices/3/sensors";
const char* TOKEN    = "19|u8UlaD1fse3H0ZpnNaPNy6A0oD61OLCowOzFdfD5f16182a0";

// --- PINS ---
#define SOIL_PIN   35   // Capteur humidité sol (analogique)
#define FLOW_PIN   18   // Débitmètre (interruption)
#define TEMP_PIN   26   // DS18B20 température sol
#define DHTPIN     21   // DHT11 température + humidité air
#define RELAY_PIN  23   // Relais → pompe
#define SWITCH_PIN 32   // Interrupteur physique (priorité absolue)
#define TRIG_PIN   14   // Ultrasons TRIG
#define ECHO_PIN   25   // Ultrasons ECHO

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- ÉTALONNAGE ---
const int   SOIL_DRY   = 2670;  // valeur analogique sol sec
const int   SOIL_WET   = 2540;  // valeur analogique sol humide
const float CAL_FACTOR = 7.5;   // facteur calibration débitmètre
const int   TANK_DEPTH = 10;    // profondeur cuve en cm (distance max capteur→eau)

// --- VARIABLES GLOBALES ---
volatile int  pulseCount = 0;
float         totalLitres = 0.0;
float         flowRate    = 0.0;

unsigned long lastSensorMillis = 0;  // cadence lecture capteurs (1s)
unsigned long lastAPIMillis    = 0;  // cadence envoi API (5s)

// État pompe — source unique de vérité
bool pumpStatus = false;

// Mémorisation état précédent du switch pour détecter les changements
bool switchPrecedent = false;

// Données capteurs
int   humSol    = 0;
int   rawSoil   = 0;
bool  soilError = false;
int   waterLevel = 0;
float tempSol   = -127.0;
float humAir    = 0.0;
float tempAir   = 0.0;

OneWire           oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// ── Interruption débitmètre ──────────────────────────────────
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// ── Contrôle pompe centralisé ────────────────────────────────
void setPump(bool on) {
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  pumpStatus = on;
  Serial.printf("🔌 Pompe → %s\n", on ? "ON" : "OFF");
}

// ── Connexion WiFi ───────────────────────────────────────────
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
    Serial.printf("\n✅ WiFi connecté — IP : %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n⚠️ WiFi non disponible — mode local actif.");
  }
}

// ── Niveau eau via ultrasons ─────────────────────────────────
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

// ────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  // Relais
  pinMode(RELAY_PIN, OUTPUT);
  setPump(false);

  // Switch physique — lecture initiale
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  switchPrecedent = !digitalRead(SWITCH_PIN);

  // Débitmètre
  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);

  // Ultrasons + sol
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SOIL_PIN, INPUT);

  // Capteurs température
  sensors.begin();
  sensors.setWaitForConversion(false);
  dht.begin();

  connecterWiFi();

  lastSensorMillis = millis();
  lastAPIMillis    = millis();
}

// ────────────────────────────────────────────────────────────
void loop() {

  // ══════════════════════════════════════════════════════════
  //  PRIORITÉ 1 — SWITCH PHYSIQUE (PIN 32)
  //  Priorité absolue, indépendant du WiFi
  //  Agit dès le changement de position de l'interrupteur
  // ══════════════════════════════════════════════════════════
  bool switchActif = !digitalRead(SWITCH_PIN);

  if (switchActif != switchPrecedent) {
    switchPrecedent = switchActif;
    if (switchActif) {
      setPump(true);
      Serial.println("🚨 [MODE PHYSIQUE] Switch ON → Pompe ALLUMÉE (priorité absolue)");
    } else {
      setPump(false);
      Serial.println("🛑 [MODE PHYSIQUE] Switch OFF → Pompe ÉTEINTE — contrôle web/auto actif");
    }
  }

  unsigned long now = millis();

  // ══════════════════════════════════════════════════════════
  //  LECTURE CAPTEURS — toutes les 1 seconde
  // ══════════════════════════════════════════════════════════
  if (now - lastSensorMillis >= 1000) {
    unsigned long dt = now - lastSensorMillis;
    lastSensorMillis = now;

    // Débitmètre
    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();
    flowRate    = (pulses / CAL_FACTOR) * (60000.0f / (float)dt);
    totalLitres += ((float)pulses / CAL_FACTOR) / 60.0f;

    // Humidité sol
    rawSoil = analogRead(SOIL_PIN);
    if (rawSoil < 300) {
      soilError = true;
    } else {
      soilError = false;
      humSol = constrain(map(rawSoil, SOIL_DRY, SOIL_WET, 0, 100), 0, 100);
    }

    // Niveau eau
    waterLevel = getWaterLevelPercent();

    // Température sol
    tempSol = sensors.getTempCByIndex(0);
    sensors.requestTemperatures();

    // Air (DHT11)
    humAir  = dht.readHumidity();
    tempAir = dht.readTemperature();

    // Moniteur série
    Serial.print("🌱 Sol: ");
    soilError ? Serial.print("ERR") : (Serial.print(humSol), Serial.print("%"));
    Serial.print(" | 🌡️ TempSol: ");
    (tempSol != -127.0) ? (Serial.print(tempSol,1), Serial.print("°C")) : Serial.print("ERR");
    Serial.print(" | 💨 Air: ");
    isnan(tempAir) ? Serial.print("?") : (Serial.print(tempAir,1), Serial.print("°C "));
    isnan(humAir)  ? Serial.print("?%") : (Serial.print(humAir,0), Serial.print("%"));
    Serial.print(" | 🛢️ Cuve: ");  Serial.print(waterLevel);   Serial.print("%");
    Serial.print(" | 💧 Débit: "); Serial.print(flowRate,1);   Serial.print("L/m");
    Serial.print(" | Total: ");    Serial.print(totalLitres,2); Serial.print("L");
    Serial.print(" | SWITCH: ");   Serial.print(switchActif ? "ON" : "OFF");
    Serial.print(" | POMPE: ");    Serial.println(pumpStatus ? "ON" : "OFF");
  }

  // ══════════════════════════════════════════════════════════
  //  SYNCHRONISATION API — toutes les 5 secondes
  //  Envoie les données + reçoit pump_command de Laravel
  //  Appliqué UNIQUEMENT si switch physique est sur OFF
  // ══════════════════════════════════════════════════════════
  if (now - lastAPIMillis >= 5000) {
    lastAPIMillis = now;

    // Reconnexion automatique si WiFi perdu
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ WiFi perdu — reconnexion...");
      connecterWiFi();
      return;
    }

    // --- Construction du payload JSON ---
    StaticJsonDocument<512> jsonDoc;
    if (!soilError)         jsonDoc["soil_humidity"]    = humSol;
    if (tempSol != -127.0)  jsonDoc["soil_temperature"] = tempSol;
    if (!isnan(tempAir))    jsonDoc["air_temperature"]  = tempAir;
    if (!isnan(humAir))     jsonDoc["air_humidity"]     = humAir;
    jsonDoc["water_level"]  = waterLevel;
    jsonDoc["pump_status"]  = pumpStatus ? 1 : 0;
    jsonDoc["flow_rate"]    = round(flowRate    * 10.0f) / 10.0f;
    jsonDoc["total_volume"] = round(totalLitres * 100.0f) / 100.0f;
    jsonDoc["manual_mode"]  = switchActif ? 1 : 0;  // Laravel saura que le switch est actif

    String payload;
    serializeJson(jsonDoc, payload);

    // --- Envoi HTTP POST ---
    HTTPClient http;
    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + TOKEN);
    http.setTimeout(4000);  // 4s — suffisant même sur réseau WiFi partagé

    int httpCode = http.POST(payload);
    Serial.printf("📡 API HTTP %d\n", httpCode);

    if (httpCode == 200 || httpCode == 201) {
      String reponse = http.getString();
      StaticJsonDocument<256> reponseDoc;

      if (deserializeJson(reponseDoc, reponse) == DeserializationError::Ok) {

        // ══════════════════════════════════════════════════
        //  PRIORITÉ 2 & 3 — COMMANDE WEB / AUTO (Laravel)
        //  Appliquée SEULEMENT si switch physique est OFF
        //  Laravel décide en fonction de :
        //    - Bouton "Irriguer" → pump_command = 1
        //    - Bouton "Arrêter" → pump_command = 0
        //    - Mode auto (seuils culture) → pump_command = 0 ou 1
        //    - Cuve vide (sécurité) → pump_command = 0
        // ══════════════════════════════════════════════════
        if (!switchActif && reponseDoc.containsKey("pump_command")) {
          int cmd = reponseDoc["pump_command"];
          Serial.printf("🌐 [API] pump_command = %d | pompe actuelle = %s\n",
                        cmd, pumpStatus ? "ON" : "OFF");

          if (cmd == 1 && !pumpStatus) {
            setPump(true);
            Serial.println("✅ [WEB/AUTO] Ordre ALLUMER → Pompe activée");
          } else if (cmd == 0 && pumpStatus) {
            setPump(false);
            Serial.println("✅ [WEB/AUTO] Ordre ÉTEINDRE → Pompe coupée");
          } else {
            Serial.println("ℹ️ [WEB/AUTO] Pompe déjà dans l'état demandé");
          }
        } else if (switchActif) {
          Serial.println("⚙️ [PHYSIQUE] Switch actif — commande web ignorée");
        }

      } else {
        Serial.println("⚠️ Erreur parsing JSON réponse");
      }

    } else {
      Serial.printf("❌ Erreur HTTP %d — vérifier le serveur Laravel\n", httpCode);
    }

    http.end();
  }
}
