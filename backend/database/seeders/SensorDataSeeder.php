<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use Illuminate\Support\Facades\DB;
use App\Models\Device;

class SensorDataSeeder extends Seeder
{
    public function run()
    {
        $sensors = [
            ['name' => 'Capacitive_Soil_Humidity', 'unit' => '%', 'min' => 30, 'max' => 80],
            ['name' => 'BME280_Temperature', 'unit' => '°C', 'min' => 16, 'max' => 32],
            ['name' => 'BME280_Humidity', 'unit' => '%', 'min' => 35, 'max' => 85],
            ['name' => 'Ultrasonic_Water_Level', 'unit' => '%', 'min' => 10, 'max' => 100],
            ['name' => 'Flowmeter_Liters', 'unit' => 'L', 'min' => 0, 'max' => 5],
        ];

        $deviceIds = Device::pluck('id')->toArray();

        foreach ($sensors as $sensor) {
            for ($i = 0; $i < 10; $i++) {
                DB::table('sensor_data')->insert([
                    'device_id'   => $deviceIds ? $deviceIds[array_rand($deviceIds)] : null,
                    'sensor_name' => $sensor['name'],
                    'value'       => rand($sensor['min'] * 10, $sensor['max'] * 10) / 10,
                    'unit'        => $sensor['unit'],
                    'created_at'  => now()->subMinutes(rand(1, 1000)),
                    'updated_at'  => now()->subMinutes(rand(1, 1000)),
                ]);
            }
        }
    }
}