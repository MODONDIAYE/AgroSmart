<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * Ajoute le support du capteur DS18B20 pour la température du sol.
     * Note: La table sensor_data utilise un design EAV (Entity-Attribute-Value),
     * donc aucune modification de schéma n'est nécessaire - on ajoute simplement
     * un nouveau type de capteur: DS18B20_Soil_Temperature.
     */
    public function up(): void
    {
        // La table sensor_data utilise le modèle EAV:
        // - sensor_name: nom du capteur (ex: DS18B20_Soil_Temperature)
        // - value: valeur mesurée
        // - unit: unité de mesure (ex: °C)
        // Aucune modification de schéma requise pour ajouter un nouveau type de capteur.
        
        // Documentation du nouveau capteur:
        // sensor_name: DS18B20_Soil_Temperature
        // unit: °C
        // description: Température du sol mesurée par la sonde DS18B20
        // range typique: -10°C à 50°C
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        // Aucun rollback nécessaire car aucune modification de schéma
    }
};
