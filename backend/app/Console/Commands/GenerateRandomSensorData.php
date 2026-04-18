<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Models\Device;
use App\Models\SensorData;

class GenerateRandomSensorData extends Command
{
    protected $signature = 'sensor:generate-random {device_id?} {--rows=20}';
    protected $description = 'Generate random sensor data rows for device(s) in sensor_data table.';

    public function handle(): int
    {
        $deviceId = $this->argument('device_id');
        $rows = intval($this->option('rows')) ?: 20;

        $devices = $deviceId
            ? Device::where('id', $deviceId)->get()
            : Device::all();

        if ($devices->isEmpty()) {
            $this->error('Aucun appareil trouvé.');
            return 1;
        }

        $sensorTypes = [
            ['name' => 'Capacitive_Soil_Humidity', 'unit' => '%', 'min' => 30, 'max' => 80],
            ['name' => 'BME280_Temperature', 'unit' => '°C', 'min' => 16, 'max' => 32],
            ['name' => 'BME280_Humidity', 'unit' => '%', 'min' => 35, 'max' => 85],
            ['name' => 'Ultrasonic_Water_Level', 'unit' => '%', 'min' => 10, 'max' => 100],
            ['name' => 'Flowmeter_Liters', 'unit' => 'L', 'min' => 0, 'max' => 5],
        ];

        $generated = 0;

        foreach ($devices as $device) {
            for ($i = 0; $i < $rows; $i++) {
                foreach ($sensorTypes as $sensor) {
                    SensorData::create([
                        'device_id' => $device->id,
                        'sensor_name' => $sensor['name'],
                        'value' => round(mt_rand($sensor['min'] * 10, $sensor['max'] * 10) / 10, 1),
                        'unit' => $sensor['unit'],
                        'created_at' => now()->subMinutes(mt_rand(0, 120)),
                        'updated_at' => now(),
                    ]);
                    $generated++;
                }
            }
        }

        $this->info("Generated {$generated} random sensor data rows.");

        return 0;
    }
}
