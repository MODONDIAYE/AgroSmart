#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// ==================== Configuration DHT11 ====================
#define DHTPIN 5        // Broche GPIO du DHT11 (à adapter selon votre setup)
#define DHTTYPE DHT11   // Spécifier DHT11
DHT dht(DHTPIN, DHTTYPE);

// ==================== Configuration WiFi ====================
const char* ssid = "YOUR_WIFI_SSID";           // Votre SSID WiFi
const char* password = "YOUR_WIFI_PASSWORD";   // Votre mot de passe WiFi

// ==================== Configuration Serveur ====================
const char* serverUrl = "http://192.168.1.100:8000/api/sensor-data";  // URL de votre backend
// Alternative si vous avez un domaine local :
// const char* serverUrl = "http://your-server.local/api/sensor-data";

// ==================== Configuration d'envoi ====================
unsigned long lastSend = 0;
const unsigned long intervalMs = 30000;  // Envoyer toutes les 30 secondes (à adapter)

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialiser le DHT11
  dht.begin();
  
  // Connexion WiFi
  Serial.println("\n\nDémarrage du capteur DHT11...");
  Serial.print("Connexion à WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connecté");
    Serial.print("Adresse IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ Impossible de se connecter au WiFi");
  }
}

void sendSensorData() {
  // Vérifier la connexion WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERREUR] WiFi non connecté");
    return;
  }

  // Lire les données du DHT11
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Vérifier si la lecture est valide
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("[ERREUR] Impossible de lire les données du DHT11");
    return;
  }

  // Afficher les données lues
  Serial.print("\nDonnées lues - Temp: ");
  Serial.print(temperature);
  Serial.print("°C, Humidité: ");
  Serial.print(humidity);
  Serial.println("%");

  // Préparer le payload JSON
  String payload = "{\"temp\":";
  payload += String(temperature, 2);
  payload += ",\"hum\":";
  payload += String(humidity, 2);
  payload += "}";

  Serial.print("Payload: ");
  Serial.println(payload);

  // Envoyer les données via HTTP POST
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  
  Serial.print("Envoi vers: ");
  Serial.println(serverUrl);

  int httpResponseCode = http.POST(payload);

  // Afficher la réponse
  Serial.print("Code de réponse HTTP: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Réponse: ");
    Serial.println(response);
  } else {
    Serial.print("[ERREUR] Erreur de requête HTTP: ");
    Serial.println(http.errorToString(httpResponseCode));
  }

  http.end();
}

void loop() {
  // Envoyer les données à intervalle régulier
  if (millis() - lastSend >= intervalMs) {
    lastSend = millis();
    sendSensorData();
  }
  
  delay(1000);  // Petit délai pour éviter un bouclage trop rapide
}
