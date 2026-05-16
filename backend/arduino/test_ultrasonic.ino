/*
 * ============================================================
 *  TEST — Capteur Ultrasonique HC-SR04
 *  ESP32 — Mesure du niveau d'eau dans un réservoir
 *
 *  Branchement :
 *    VCC  → 5V  (ou 3.3V selon votre module)
 *    GND  → GND
 *    TRIG → GPIO5
 *    ECHO → GPIO18
 *
 *  Placement :
 *    - Fixer le capteur EN HAUT du réservoir, face vers le bas
 *    - Distance minimale de mesure : 2 cm
 *    - Distance maximale : 400 cm
 *
 *  Aucune bibliothèque externe requise.
 * ============================================================
 */

#define TRIG_PIN  5    // GPIO5  → TRIG
#define ECHO_PIN  18   // GPIO18 → ECHO

// ── Calibration réservoir ────────────────────────────────────
// Mesurez ces deux valeurs avec votre bouteille :
//
// DIST_VIDE  = distance (cm) entre le capteur et le FOND
//              quand la bouteille est VIDE
//              → capteur en haut, bouteille vide, mesurez la distance
//
// DIST_PLEIN = distance (cm) entre le capteur et la SURFACE DE L'EAU
//              quand la bouteille est PLEINE
//              → doit être >= 2 cm (limite du HC-SR04)
//
// Exemple pour une bouteille de 20 cm de hauteur :
//   DIST_VIDE  = 22 cm  (capteur 2cm au-dessus du bord + 20cm fond)
//   DIST_PLEIN =  3 cm  (eau presque au bord, 3cm sous le capteur)
#define DIST_VIDE   22.0   // cm — réservoir vide   → niveau 0%
#define DIST_PLEIN   3.0   // cm — réservoir plein  → niveau 100%
// ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    Serial.println("========================================");
    Serial.println("  TEST Capteur Ultrasonique HC-SR04");
    Serial.println("  TRIG=GPIO5   ECHO=GPIO18");
    Serial.println("========================================");
    Serial.println("Calibration actuelle :");
    Serial.println("  Réservoir VIDE  → " + String(DIST_VIDE)  + " cm");
    Serial.println("  Réservoir PLEIN → " + String(DIST_PLEIN) + " cm");
    Serial.println("----------------------------------------");
    Serial.println("Lecture toutes les secondes...\n");
}

// Mesure la distance en cm — retourne -1 si timeout
float mesureDistance() {
    // Impulsion TRIG 10µs
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(4);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Mesure durée écho (timeout 30ms = ~5m max)
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);

    // Afficher la durée brute pour diagnostic
    Serial.println("  [DEBUG] durée écho = " + String(duration) + " µs");

    if (duration == 0)   return -1.0; // timeout — aucun écho
    if (duration < 120)  return -2.0; // < 2cm — zone morte du HC-SR04

    // Vitesse du son : 343 m/s → 0.0343 cm/µs
    return (duration * 0.0343) / 2.0;
}

void loop() {
    // Moyenne sur 5 mesures pour stabiliser
    float total = 0;
    int valid = 0;
    for (int i = 0; i < 5; i++) {
        float d = mesureDistance();
        if (d == -2.0) {
            Serial.println("  ⚠️  Zone morte (<2cm) — éloignez le capteur de la surface");
        } else if (d > 0) {
            total += d;
            valid++;
        }
        delay(60);
    }

    if (valid == 0) {
        Serial.println("⚠️  Pas de mesure valide.");
        Serial.println("  → Si durée=0      : vérifiez câblage TRIG/ECHO");
        Serial.println("  → Si durée < 120µs : capteur trop proche, éloignez-le");
        delay(1000);
        return;
    }

    float distance = total / valid;

    // Calcul niveau d'eau en %
    // Distance grande = peu d'eau = niveau bas
    // Distance petite = beaucoup d'eau = niveau haut
    float niveau = map(distance * 10, DIST_PLEIN * 10, DIST_VIDE * 10, 100, 0);
    niveau = constrain(niveau, 0.0, 100.0);

    // Interprétation
    String etat;
    if (niveau < 10)       etat = "🔴 Réservoir quasi vide";
    else if (niveau < 30)  etat = "🟠 Niveau bas";
    else if (niveau < 60)  etat = "🟡 Niveau moyen";
    else if (niveau < 85)  etat = "🟢 Niveau correct";
    else                   etat = "💧 Réservoir plein";

    // Affichage
    Serial.println("--- Mesure ultrason ---");
    Serial.println("  Distance   : " + String(distance, 1) + " cm  (" + String(valid) + "/5 mesures valides)");
    Serial.println("  Niveau eau : " + String(niveau, 1) + " %");
    Serial.println("  État       : " + etat);
    Serial.println();

    // Guide calibration
    if (distance < 2.0) {
        Serial.println("  ⚠️  Distance < 2cm : trop proche, relevez le capteur");
    }
    if (distance > 400.0) {
        Serial.println("  ⚠️  Distance > 400cm : hors portée du HC-SR04");
    }

    delay(1000);
}
