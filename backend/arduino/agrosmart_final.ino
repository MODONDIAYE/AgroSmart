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

// --- ATTRIBUTION DES PINS ---
#define SOIL_PIN    35
#define FLOW_PIN    18
#define TEMP_PIN    26
#define DHTPIN      21
#define RELAY_PIN   23
#define SWITCH_PIN  32
#define TRIG_PIN    14
#define ECHO_PIN    25
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// --- VALEURS D'ETALONNAGE & SEUILS ---
const int   SOIL_DRY  = 2670;
const int   SOIL_WET  = 2540;
const float CAL_FACTOR = 7.5;
int         TANK_DEPTH = 10;

// --- VARIABLES ---
volatile int pulseCount = 0;
float        totalLitres = 0.0;
float        flowRate    = 0.0;

unsigned long lastDisplayMillis = 0;
unsigned long lastAPIMillis     = 0;

bool pumpStatus               = false;
bool dernierEtatBoutonPhysique = false;

int   humSol    = 0;
int   rawSoil   = 0;
bool  soilError = false;
int   waterLevel = 0;
float tempSol   = -127.0;
float humAir    = 0.0;
float tempAir   = 0.0;

OneWire         oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// ─────────────────────────────────────────────────────────────
void connecterWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("\n=========================================");
  Serial.printf("Connexion au WiFi : %s\n", SSID);
  Serial.println("=========================================");
  WiFi.begin(SSID, PASSWORD);
  int tentatives = 0;
  while (WiFi.status() != WL_CONNECTED && tentatives < 30) {
    delay(500);
    Serial.print(".");
    tentatives++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n\n✅ WiFi Connecté avec succès !");
    Serial.print("   -> Adresse IP accordée : ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n\n⚠️ Échec de connexion WiFi (Mode local actif).");
  }
  Serial.println("=========================================\n");
}

// ─────────────────────────────────────────────────────────────
int getWaterLevelPercent() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duree = pulseIn(ECHO_PIN, HIGH, 10000);
  if (duree == 0) return 0;
  int distance    = duree * 0.034 / 2;
  int levelPercent = map(distance, TANK_DEPTH, 3, 0, 100);
  return constrain(levelPercent, 0, 100);
}

// ─────────────────────────────────────────────────────────────
void setPump(bool on) {
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  pumpStatus = on;
}

// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(RELAY_PIN, OUTPUT);
  setPump(false);  // pompe éteinte au démarrage

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  dernierEtatBoutonPhysique = !digitalRead(SWITCH_PIN);

  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SOIL_PIN, INPUT);

  sensors.begin();
  sensors.setWaitForConversion(false);
  dht.begin();

  connecterWiFi();
  lastDisplayMillis = millis();
  lastAPIMillis     = millis();
}

// ─────────────────────────────────────────────────────────────
void loop() {

  // ── 1. COMMUTATEUR PHYSIQUE (priorité absolue) ──────────────
  bool modeManuelPhysique = !digitalRead(SWITCH_PIN);

  if (modeManuelPhysique != dernierEtatBoutonPhysique) {
    dernierEtatBoutonPhysique = modeManuelPhysique;
    if (modeManuelPhysique) {
      setPump(true);
      Serial.println("🚨 [PHYSIQUE] Commutateur ON → Pompe activée");
    } else {
      setPump(false);
      Serial.println("🛑 [PHYSIQUE] Commutateur OFF → Pompe coupée");
    }
  }

  unsigned long currentMillis = millis();

  // ── 2. LECTURE CAPTEURS (toutes les 1s) ─────────────────────
  if (currentMillis - lastDisplayMillis >= 1000) {
    unsigned long intervalleCalcul = currentMillis - lastDisplayMillis;
    lastDisplayMillis = currentMillis;

    noInterrupts();
    int pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    flowRate    = (pulses / CAL_FACTOR) * (60000.0f / (float)intervalleCalcul);
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

    // Moniteur série
    Serial.print("🌱 Hum Sol: ");
    if (!soilError) { Serial.print(humSol); Serial.print("%"); } else { Serial.print("ERREUR"); }
    Serial.print(" | 🌡️ Temp Sol: ");
    if (tempSol != -127.00) { Serial.print(tempSol, 1); Serial.print("°C"); } else { Serial.print("ERREUR"); }
    Serial.print(" | 💨 Air: ");
    if (!isnan(tempAir)) { Serial.print(tempAir, 1); Serial.print("°C "); } else { Serial.print("?°C "); }
    if (!isnan(humAir))  { Serial.print(humAir, 0);  Serial.print("%"); }  else { Serial.print("?%"); }
    Serial.print(" | 🛢️ Cuve: ");    Serial.print(waterLevel);  Serial.print("%");
    Serial.print(" | 💧 Débit: ");    Serial.print(flowRate, 1);  Serial.print(" L/min");
    Serial.print(" | Total: ");       Serial.print(totalLitres, 3); Serial.print(" L");
    Serial.print(" | SWITCH: ");      Serial.print(modeManuelPhysique ? "ON" : "OFF");
    Serial.print(" | 🚀 POMPE: ");    Serial.println(pumpStatus ? "ON" : "OFF");
  }

  // ── 3. SYNCHRONISATION API (toutes les 5s) ──────────────────
  if (currentMillis - lastAPIMillis >= 5000) {
    lastAPIMillis = currentMillis;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ WiFi déconnecté — tentative de reconnexion...");
      connecterWiFi();
      return;
    }

    HTTPClient http;
    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + TOKEN);
    http.setTimeout(3000);  // 3s de timeout (était 1.5s — trop court !)

    StaticJsonDocument<400> jsonDoc;
    if (!soilError) jsonDoc["soil_humidity"] = humSol;
    jsonDoc["water_level"]  = waterLevel;
    jsonDoc["pump_status"]  = pumpStatus ? 1 : 0;
    jsonDoc["flow_rate"]    = round(flowRate    * 10.0f) / 10.0f;
    jsonDoc["total_volume"] = round(totalLitres * 100.0f) / 100.0f;
    jsonDoc["manual_mode"]  = modeManuelPhysique ? 1 : 0;
    if (tempSol != -127.00)  jsonDoc["soil_temperature"] = tempSol;
    if (!isnan(tempAir))     jsonDoc["air_temperature"]  = tempAir;
    if (!isnan(humAir))      jsonDoc["air_humidity"]     = humAir;

    String corpsJSON;
    serializeJson(jsonDoc, corpsJSON);

    int codeHTTP = http.POST(corpsJSON);
    Serial.printf("📡 API → HTTP %d\n", codeHTTP);

    if (codeHTTP == 200 || codeHTTP == 201) {
      String reponse = http.getString();
      StaticJsonDocument<300> reponseDoc;

      if (!deserializeJson(reponseDoc, reponse)) {

        // ── Commande web : acceptée UNIQUEMENT si switch physique est sur OFF
        if (!modeManuelPhysique && reponseDoc.containsKey("pump_command")) {
          int commandePompe = reponseDoc["pump_command"];

          // -1 = backend signale que le switch physique est actif → on ignore
          if (commandePompe == -1) {
            Serial.println("🌐 [API] Switch physique actif côté backend — commande web ignorée");
          } else {
            Serial.printf("🌐 [API] pump_command reçu = %d (pompe actuelle = %s)\n",
                          commandePompe, pumpStatus ? "ON" : "OFF");

            if (commandePompe == 1) {
              if (!pumpStatus) {
                setPump(true);
                Serial.println("🌐 [API WEB] Ordre ALLUMER → Pompe activée ✅");
              } else {
                Serial.println("🌐 [API WEB] Pompe déjà ON — rien à faire");
              }
            } else {  // commandePompe == 0
              if (pumpStatus) {
                setPump(false);
                Serial.println("🌐 [API WEB] Ordre ÉTEINDRE → Pompe coupée ✅");
              } else {
                Serial.println("🌐 [API WEB] Pompe déjà OFF — rien à faire");
              }
            }
          }
        }

      } else {
        Serial.println("⚠️ Erreur parsing JSON réponse API");
      }
    } else {
      Serial.printf("❌ Erreur HTTP : %d\n", codeHTTP);
    }

    http.end();
  }
}
