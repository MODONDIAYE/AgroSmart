<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::table('sensor_data', function (Blueprint $table) {
            // Index composite pour les requêtes (device_id + sensor_name + created_at)
            // Couvre latestSensorData et sensorHistory en une seule passe
            $table->index(['device_id', 'sensor_name', 'created_at'], 'idx_sensor_data_device_name_created');
        });
    }

    public function down(): void
    {
        Schema::table('sensor_data', function (Blueprint $table) {
            $table->dropIndex('idx_sensor_data_device_name_created');
        });
    }
};
