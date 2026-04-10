<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Str;

class SensorDataSeeder extends Seeder
{
    public function run()
    {
        $sensors = [
            ['name' => 'DHT22_Temp', 'unit' => '°C', 'min' => 18, 'max' => 28],
            ['name' => 'DHT22_Hum', 'unit' => '%', 'min' => 40, 'max' => 65],
            ['name' => 'LDR_Light', 'unit' => 'lux', 'min' => 200, 'max' => 800],
        ];

        foreach ($sensors as $sensor) {
            for ($i = 0; $i < 10; $i++) {
                DB::table('sensor_data')->insert([
                    'sensor_name' => $sensor['name'],
                    'value'       => rand($sensor['min'] * 10, $sensor['max'] * 10) / 10,
                    'unit'        => $sensor['unit'],
                    'created_at'  => now()->subMinutes(rand(1, 1000)), // Dates aléatoires passées
                ]);
            }
        }
    }
}