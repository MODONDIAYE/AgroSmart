<?php

namespace App\Http\Controllers;

use App\Models\Device;
use App\Models\IrrigationEven;
use App\Models\Notification;
use App\Models\SensorData;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Schema;
use Illuminate\Support\Facades\Validator;

class DeviceController extends Controller
{
    /**
     * Liste des appareils de l'utilisateur connecté
     */
    public function index(Request $request)
    {
        $devices = $request->user()->devices()->with('crop')->orderBy('created_at', 'desc')->get();
        return response()->json(['success' => true, 'data' => $devices]);
    }

    /**
     * Enregistrer un nouvel appareil
     */
    public function store(Request $request)
    {
        $validator = Validator::make($request->all(), [
            'device_name' => 'required|string|max:255',
            'device_key'  => 'required|string|unique:devices,device_key',
            'location'    => 'nullable|string|max:255',
        ]);

        if ($validator->fails()) {
            return response()->json(['success' => false, 'message' => $validator->errors()->first()], 422);
        }

        $device = $request->user()->devices()->create([
            'device_name' => $request->device_name,
            'device_key'  => $request->device_key,
            'location'    => $request->location,
            'status'      => 'offline',
        ]);

        Notification::create([
            'user_id' => $request->user()->id,
            'device_id' => $device->id,
            'title' => 'Nouveau dispositif ajouté',
            'message' => "L'appareil \"{$device->device_name}\" a bien été ajouté.",
            'body' => "L'appareil \"{$device->device_name}\" est prêt à être connecté.",
            'type' => 'info',
            'is_read' => false,
        ]);

        return response()->json(['success' => true, 'data' => $device], 201);
    }

    /**
     * Détails d'un appareil spécifique
     */
    public function show(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);
        return response()->json(['success' => true, 'data' => $device]);
    }

    /**
     * Mettre à jour ou supprimer un appareil
     */
    public function update(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);

        if ($request->has('deleted') && $request->deleted === true) {
            $device->delete();
            return response()->json(['success' => true, 'message' => 'Appareil supprimé']);
        }

        $request->validate([
            'crop_id' => 'nullable|exists:crops,id',
        ]);

        $oldCropId = $device->crop_id;

        $device->update($request->only(['device_name', 'location', 'status', 'crop_id']));
        $device->load('crop');

        if ($request->has('crop_id') && $oldCropId !== $device->crop_id) {
            $message = $device->crop
                ? "Culture \"{$device->crop->name}\" assignée à l'appareil."
                : 'Aucune culture n’est désormais associée à cet appareil.';

            Notification::create([
                'user_id' => $device->user_id,
                'device_id' => $device->id,
                'title' => $device->crop ? 'Culture mise à jour' : 'Culture désassignée',
                'message' => $message,
                'body' => $message,
                'type' => 'info',
                'is_read' => false,
            ]);
        }

        return response()->json(['success' => true, 'data' => $device]);
    }

    /**
     * Contrôle manuel de l'arrosage (Toggle)
     */
    public function toggleIrrigation(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);
        
        // Simule l'envoi d'une commande (on change le statut pour le test)
        $newStatus = $request->status ? 'online' : 'offline';
        $device->update(['status' => $newStatus]); 

        return response()->json([
            'success' => true, 
            'message' => 'Commande transmise à l\'appareil',
            'status' => $device->status
        ]);
    }

    /**
     * Réception des données capteurs (ESP32)
     */
    public function updateSensors(Request $request, $id)
    {
        $device = Device::findOrFail($id);

        $data = $request->validate([
            'soil_humidity' => 'nullable|numeric',
            'air_temperature' => 'nullable|numeric',
            'air_humidity' => 'nullable|numeric',
            'water_level' => 'nullable|numeric',
            'liters' => 'nullable|numeric',
            'timestamp'   => 'nullable|date',
        ]);

        $sensorRows = [];
        $timestamp = $data['timestamp'] ?? now();

        if (isset($data['soil_humidity'])) {
            $sensorRows[] = [
                'device_id'   => $device->id,
                'sensor_name' => 'Capacitive_Soil_Humidity',
                'value'       => $data['soil_humidity'],
                'unit'        => '%',
                'created_at'  => $timestamp,
                'updated_at'  => $timestamp,
            ];
        }
        if (isset($data['air_temperature'])) {
            $sensorRows[] = [
                'device_id'   => $device->id,
                'sensor_name' => 'BME280_Temperature',
                'value'       => $data['air_temperature'],
                'unit'        => '°C',
                'created_at'  => $timestamp,
                'updated_at'  => $timestamp,
            ];
        }
        if (isset($data['air_humidity'])) {
            $sensorRows[] = [
                'device_id'   => $device->id,
                'sensor_name' => 'BME280_Humidity',
                'value'       => $data['air_humidity'],
                'unit'        => '%',
                'created_at'  => $timestamp,
                'updated_at'  => $timestamp,
            ];
        }
        if (isset($data['water_level'])) {
            $sensorRows[] = [
                'device_id'   => $device->id,
                'sensor_name' => 'Ultrasonic_Water_Level',
                'value'       => $data['water_level'],
                'unit'        => '%',
                'created_at'  => $timestamp,
                'updated_at'  => $timestamp,
            ];
        }
        if (isset($data['liters'])) {
            $sensorRows[] = [
                'device_id'   => $device->id,
                'sensor_name' => 'Flowmeter_Liters',
                'value'       => $data['liters'],
                'unit'        => 'L',
                'created_at'  => $timestamp,
                'updated_at'  => $timestamp,
            ];
        }

        if (!empty($sensorRows)) {
            SensorData::insert($sensorRows);

            $device->load('crop');
            $crop = $device->crop;

            if ($crop) {
                $notifications = [];

                if (isset($data['soil_humidity']) && $data['soil_humidity'] < $crop->humidity_threshold) {
                    $notifications[] = [
                        'title' => 'Humidité du sol trop basse',
                        'message' => "L'humidité du sol est de {$data['soil_humidity']}% et est inférieure au seuil de {$crop->humidity_threshold}% pour {$crop->name}.",
                        'type' => 'warning',
                    ];
                }

                if (isset($data['water_level']) && $data['water_level'] < $crop->min_water_level) {
                    $notifications[] = [
                        'title' => 'Niveau d’eau faible',
                        'message' => "Le niveau d'eau est de {$data['water_level']}% et est inférieur au seuil minimum de {$crop->min_water_level}%.",
                        'type' => 'warning',
                    ];
                }

                foreach ($notifications as $notification) {
                    Notification::create([
                        'user_id' => $device->user_id,
                        'device_id' => $device->id,
                        'title' => $notification['title'],
                        'message' => $notification['message'],
                        'body' => $notification['message'],
                        'type' => $notification['type'],
                        'is_read' => false,
                    ]);
                }
            }
        }

        $device->update(['status' => 'online']);

        return response()->json([
            'success' => true,
            'message' => 'Données capteurs reçues avec succès'
        ]);
    }

    /**
     * Événements d'irrigation du capteur
     */
    public function getEvents(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);
        $events = IrrigationEven::where('device_id', $device->id)
            ->orderByDesc('timestamp')
            ->limit(10)
            ->get();

        return response()->json(['success' => true, 'data' => $events]);
    }

    /**
     * Démarrer une irrigation manuelle
     */
    public function triggerManualIrrigation(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);

        $data = $request->validate([
            'duration_seconds' => 'nullable|integer',
            'mode'             => 'nullable|string',
        ]);

        $event = IrrigationEven::create([
            'device_id'       => $device->id,
            'action'          => 1,
            'mode'            => $data['mode'] ?? 'Manual',
            'duration_seconds'=> $data['duration_seconds'] ?? null,
            'timestamp'       => now(),
        ]);

        $device->update(['status' => 'online']);

        return response()->json(['success' => true, 'data' => $event]);
    }

    /**
     * Arrêter une irrigation manuelle
     */
    public function stopIrrigation(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);

        $event = IrrigationEven::create([
            'device_id' => $device->id,
            'action'    => 0,
            'mode'      => 'Manual',
            'timestamp' => now(),
        ]);

        $device->update(['status' => 'offline']);

        return response()->json(['success' => true, 'data' => $event]);
    }

    /**
     * Dernière lecture du capteur pour un appareil
     */
    public function latestSensorData(Request $request, $id)
    {
        $request->user()->devices()->findOrFail($id);

        $rows = SensorData::query()
            ->when(Schema::hasColumn((new SensorData())->getTable(), 'device_id'), fn ($q) => $q->where('device_id', $id))
            ->whereIn('sensor_name', [
                'Capacitive_Soil_Humidity',
                'BME280_Temperature',
                'BME280_Humidity',
                'Ultrasonic_Water_Level',
                'Flowmeter_Liters',
            ])
            ->orderByDesc('created_at')
            ->get();

        $latest = [
            'soil_humidity' => null,
            'air_temperature' => null,
            'air_humidity' => null,
            'water_level' => null,
            'liters' => null,
            'timestamp' => null,
        ];

        foreach ($rows as $row) {
            if ($row->sensor_name === 'Capacitive_Soil_Humidity') {
                $latest['soil_humidity'] = $row->value;
            }
            if ($row->sensor_name === 'BME280_Temperature') {
                $latest['air_temperature'] = $row->value;
            }
            if ($row->sensor_name === 'BME280_Humidity') {
                $latest['air_humidity'] = $row->value;
            }
            if ($row->sensor_name === 'Ultrasonic_Water_Level') {
                $latest['water_level'] = $row->value;
            }
            if ($row->sensor_name === 'Flowmeter_Liters') {
                $latest['liters'] = $row->value;
            }
            if (!$latest['timestamp'] || $row->created_at > $latest['timestamp']) {
                $latest['timestamp'] = $row->created_at;
            }
        }

        return response()->json(['success' => true, 'data' => $latest]);
    }

    /**
     * Historique des lectures de capteurs pour un appareil
     */
    public function sensorHistory(Request $request, $id)
    {
        $request->user()->devices()->findOrFail($id);
        $limit = min(intval($request->query('limit', 24)), 100);

        $query = SensorData::query();
        if (Schema::hasColumn((new SensorData())->getTable(), 'device_id')) {
            $query->where('device_id', $id);
        }

        $historicalRows = $query->whereIn('sensor_name', [
                'Capacitive_Soil_Humidity',
                'BME280_Temperature',
                'BME280_Humidity',
                'Ultrasonic_Water_Level',
                'Flowmeter_Liters',
            ])
            ->orderByDesc('created_at')
            ->limit($limit * 5)
            ->get()
            ->sortBy('created_at');

        $grouped = $historicalRows->groupBy(function ($row) {
            return $row->created_at->format('Y-m-d H:i');
        });

        $history = $grouped->map(function ($group) {
            $entry = [
                'timestamp' => $group->first()->created_at,
                'temperature' => null,
                'air_humidity' => null,
                'soil_humidity' => null,
                'water_level' => null,
                'liters' => null,
            ];

            foreach ($group as $row) {
                if ($row->sensor_name === 'BME280_Temperature') {
                    $entry['temperature'] = $row->value;
                }
                if ($row->sensor_name === 'BME280_Humidity') {
                    $entry['air_humidity'] = $row->value;
                }
                if ($row->sensor_name === 'Capacitive_Soil_Humidity') {
                    $entry['soil_humidity'] = $row->value;
                }
                if ($row->sensor_name === 'Ultrasonic_Water_Level') {
                    $entry['water_level'] = $row->value;
                }
                if ($row->sensor_name === 'Flowmeter_Liters') {
                    $entry['liters'] = $row->value;
                }
            }

            return $entry;
        })->values();

        return response()->json(['success' => true, 'data' => $history]);
    }

    /**
     * Retourne toutes les lectures de capteurs pour un appareil.
     */
    public function allSensorData(Request $request, $id)
    {
        $request->user()->devices()->findOrFail($id);

        $rows = SensorData::query()
            ->when(Schema::hasColumn((new SensorData())->getTable(), 'device_id'), fn ($q) => $q->where('device_id', $id))
            ->orderByDesc('created_at')
            ->limit(100)
            ->get();

        return response()->json(['success' => true, 'data' => $rows]);
    }

    public function generateRandomSensorData(Request $request, $id)
    {
        $request->user()->devices()->findOrFail($id);

        $data = $request->validate([
            'rows' => 'nullable|integer|min:1|max:50',
        ]);

        $rows = $data['rows'] ?? 5;

        $sensorTypes = [
            ['name' => 'Capacitive_Soil_Humidity', 'unit' => '%', 'min' => 30, 'max' => 80],
            ['name' => 'BME280_Temperature', 'unit' => '°C', 'min' => 16, 'max' => 32],
            ['name' => 'BME280_Humidity', 'unit' => '%', 'min' => 35, 'max' => 85],
            ['name' => 'Ultrasonic_Water_Level', 'unit' => '%', 'min' => 10, 'max' => 100],
            ['name' => 'Flowmeter_Liters', 'unit' => 'L', 'min' => 0, 'max' => 5],
        ];

        $entries = [];
        for ($i = 0; $i < $rows; $i++) {
            $timestamp = now()->subMinutes(rand(0, 120));
            foreach ($sensorTypes as $sensor) {
                $entries[] = [
                    'device_id' => $id,
                    'sensor_name' => $sensor['name'],
                    'value' => round(rand($sensor['min'] * 10, $sensor['max'] * 10) / 10, 1),
                    'unit' => $sensor['unit'],
                    'created_at' => $timestamp,
                    'updated_at' => $timestamp,
                ];
            }
        }

        SensorData::insert($entries);

        return response()->json(['success' => true, 'generated' => count($entries)]);
    }

    public function destroy(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);
        $device->delete();
        return response()->json(['success' => true, 'message' => 'Appareil supprimé']);
    }
}