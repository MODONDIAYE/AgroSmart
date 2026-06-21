// ============================================================
//  AgroSmart — ESP32 Code Final
//
//  DEUX MODES DE CONTRÔLE DE LA POMPE :
//
//  MODE 1 — PHYSIQUE (Switch PIN 32, priorité absolue)
//    → Switch ON  : relais activé immédiatement, WiFi ignoré
//    → Switch OFF : relais coupé, passe en mode App/Auto
//
//  MODE 2 — APPLICATION / AUTO (via Dashboard Laravel)
//    → Bouton "Irriguer" sur le Dashboard → pump_command=1
//    → Bouton "Arrêter" sur le Dashboard  → pump_command=0
//    → Mode auto (seuils culture)         → pump_command=0/1
//    → L'ESP32 lit la réponse JSON de l'API toutes les 5s
//    → et applique la commande sur le relais
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

// URL de l'API — remplacer l'IP et l'ID du device si nécessaire
const char* API_URL  = "http://192.30.0.54:8000/api/devices/3/sensors";

// Token Sanctum de l'utilisateur connecté (générer depuis l'app si expiré)
const char* TOKEN    = "19|u8UlaD1fse3H0ZpnNaPNy6A0oD61OLCowOzFdfD5f16182a0";

// ─── PINS ────────────────────────────────────────────────────
#define SOIL_PIN   35   // Capteur humidité sol (ADC)
#define FLOW_PIN   18   // Débitmètre YF-S201 (interruption)
#define TEMP_PIN   26   // DS18B20 température sol
#define DHTPIN     21   // DHT11 température + humidité air
#define RELAY_PIN  23   // Relais → pompe à eau
#define SWITCH_PIN 32   // Interrupteur physique (INPUT_PULLUP)
#define TRIG_PIN   14   // HC-SR04 TRIG
#define ECHO_PIN   25   // HC-SR04 ECHO

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ─── ÉTALONNAGE ──────────────────────────────────────────────
const int   SOIL_DRY   = 2670;  // ADC sol totalement sec
const int   SOIL_WET   = 2540;  // ADC sol saturé d'eau
const float CAL_FACTOR = 7.5;   // pulses/litre débitmètre
const int   TANK_DEPTH = 10;    // profondeur cuve (cm, distance max→eau)

// ─── VARIABLES ───────────────────────────────────────────────
volatile int  pulseCount  = 0;
float         totalLitres = 0.0;
float         flowRate    = 0.0;

unsigned long lastSensorMillis = 0;  // lecture capteurs : 1s
unsigned long lastAPIMillis    = 0;  // sync API : 5s

bool pumpStatus      = false;  // état actuel du relais
bool switchPrecedent = false;  // dernier état lu du switch

// Données capteurs (mise à jour chaque seconde)
int   humSol     = 0;
int   rawSoil    = 0;
bool  soilError  = false;
int   waterLevel = 0;
float tempSol    = -127.0;
float humAir     = 0.0;
float tempAir    = 0.0;

OneWire           oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// ─── INTERRUPTION DÉBITMÈTRE ─────────────────────────────────
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// ─── CONTRÔLE POMPE (fonction centralisée) ───────────────────
// Toujours passer par cette fonction pour changer l'état du relais
void setPump(bool on) {
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  pumpStatus = on;
  Serial.printf("🔌 RELAIS → %s\n", on ? "ON (pompe démarrée)" : "OFF (pompe arrêtée)");
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

// ─── NIVEAU EAU (ultrasons HC-SR04) ──────────────────────────
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

  // Relais — éteint au démarrage
  pinMode(RELAY_PIN, OUTPUT);
  setPump(false);

  // Switch physique
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  switchPrecedent = !digitalRead(SWITCH_PIN);
  Serial.printf("Switch initial : %s\n", switchPrecedent ? "ON" : "OFF");

  // Débitmètre
  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);

  // Ultrasons + sol
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SOIL_PIN, INPUT);

  // Capteurs
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
  //  MODE 1 — SWITCH PHYSIQUE (PIN 32)
  //  Vérification à chaque itération du loop (< 1ms)
  //  Réaction immédiate au changement de position
  // ════════════════════════════════════════════════════════════
  bool switchActif = !digitalRead(SWITCH_PIN);  // LOW quand ON (INPUT_PULLUP)

  if (switchActif != switchPrecedent) {
    switchPrecedent = switchActif;

    if (switchActif) {
      // ── Switch basculé sur ON ──
      // Action immédiate, sans attendre l'API ni le WiFi
      setPump(true);
      Serial.println("🚨 [MODE PHYSIQUE] Switch ON → Pompe ALLUMÉE");
      Serial.println("   ℹ️ Les boutons App sont ignorés tant que le switch est ON");
    } else {
      // ── Switch basculé sur OFF ──
      // On coupe la pompe et on repasse sous contrôle App/Auto
      setPump(false);
      Serial.println("🛑 [MODE PHYSIQUE] Switch OFF → Pompe ÉTEINTE");
      Serial.println("   ℹ️ Contrôle repassé aux boutons Dashboard et mode Auto");
    }
  }

  unsigned long now = millis();

  // ════════════════════════════════════════════════════════════
  //  LECTURE CAPTEURS — toutes les 1 seconde
  // ════════════════════════════════════════════════════════════
  if (now - lastSensorMillis >= 1000) {
    unsigned long dt = now - lastSensorMillis;
    lastSensorMillis = now;

    // Débitmètre
    noInterrupts();
    int pulses = pulseCount;
    pulseCount  = 0;
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

    // Niveau eau, températures
    waterLevel = getWaterLevelPercent();
    tempSol    = sensors.getTempCByIndex(0);
    sensors.requestTemperatures();
    humAir  = dht.readHumidity();
    tempAir = dht.readTemperature();

    // ── Moniteur série ──
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
  //  MODE 2 — APPLICATION / AUTO (toutes les 5 secondes)
  //
  //  L'ESP32 envoie ses données capteurs à Laravel.
  //  Laravel répond avec pump_command = 0 ou 1 selon :
  //    • Bouton "Irriguer" cliqué → pump_command = 1
  //    • Bouton "Arrêter"  cliqué → pump_command = 0
  //    • Mode auto (humSol < seuil culture) → pump_command = 1
  //    • Cuve vide (sécurité)      → pump_command = 0
  //
  //  ⚠️ Appliqué UNIQUEMENT si switch physique est sur OFF
  // ════════════════════════════════════════════════════════════
  if (now - lastAPIMillis >= 5000) {
    lastAPIMillis = now;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ WiFi perdu — tentative reconnexion...");
      connecterWiFi();
      return;
    }

    // ── Construction du payload envoyé à Laravel ──
    StaticJsonDocument<512> jsonDoc;
    if (!soilError)        jsonDoc["soil_humidity"]    = humSol;
    if (tempSol != -127.0) jsonDoc["soil_temperature"] = tempSol;
    if (!isnan(tempAir))   jsonDoc["air_temperature"]  = tempAir;
    if (!isnan(humAir))    jsonDoc["air_humidity"]      = humAir;
    jsonDoc["water_level"]  = waterLevel;
    jsonDoc["pump_status"]  = pumpStatus ? 1 : 0;
    jsonDoc["flow_rate"]    = round(flowRate    * 10.0f) / 10.0f;
    jsonDoc["total_volume"] = round(totalLitres * 100.0f) / 100.0f;
    // Indique à Laravel si le switch physique est actif
    // → Laravel n'enverra pas de commande contradictoire
    jsonDoc["manual_mode"]  = switchActif ? 1 : 0;

    String payload;
    serializeJson(jsonDoc, payload);

    // ── Envoi HTTP POST ──
    HTTPClient http;
    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + TOKEN);
    http.setTimeout(4000);

    int httpCode = http.POST(payload);
    Serial.printf("📡 API HTTP %d", httpCode);

    if (httpCode == 200 || httpCode == 201) {
      String reponse = http.getString();
      StaticJsonDocument<256> reponseDoc;

      if (deserializeJson(reponseDoc, reponse) == DeserializationError::Ok) {

        // ── Lecture de la commande pump_command ──
        if (reponseDoc.containsKey("pump_command")) {
          int cmd = reponseDoc["pump_command"];

          if (switchActif) {
            // Switch physique ON → on ignore complètement la commande app
            Serial.printf(" | ⚙️ Switch ON — pump_command=%d ignoré\n", cmd);

          } else {
            // Switch OFF → on applique la commande de l'application / mode auto
            Serial.printf(" | 🌐 pump_command=%d reçu (pompe=%s)\n",
                          cmd, pumpStatus ? "ON" : "OFF");

            if (cmd == 1 && !pumpStatus) {
              // ── DÉMARRAGE via bouton "Irriguer" ou mode auto ──
              setPump(true);
              Serial.println("   ✅ [APP/AUTO] Ordre IRRIGUER → Pompe démarrée");

            } else if (cmd == 0 && pumpStatus) {
              // ── ARRÊT via bouton "Arrêter" ──
              setPump(false);
              Serial.println("   ✅ [APP/AUTO] Ordre ARRÊTER → Pompe coupée");

            } else {
              // Pompe déjà dans l'état demandé
              Serial.println(" | ℹ️ Pompe déjà dans l'état demandé");
            }
          }
        } else {
          Serial.println(" | ⚠️ pump_command absent de la réponse");
        }

      } else {
        Serial.println(" | ⚠️ Erreur parsing JSON réponse");
      }

    } else {
      Serial.printf("\n❌ Erreur HTTP %d — vérifier que le serveur Laravel est démarré\n", httpCode);
    }

    http.end();
  }
}
