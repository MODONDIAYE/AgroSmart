<?php

namespace App\Http\Controllers;

use App\Models\Device;
use App\Models\IrrigationEven as IrrigationEvent;
use App\Models\Notification;
use App\Models\SensorData;
use Illuminate\Http\Request;
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
            'user_id'   => $request->user()->id,
            'device_id' => $device->id,
            'title'     => 'Nouveau dispositif ajouté',
            'message'   => "L'appareil \"{$device->device_name}\" a bien été ajouté.",
            'body'      => "L'appareil \"{$device->device_name}\" est prêt à être connecté.",
            'type'      => 'info',
            'is_read'   => false,
        ]);

        return response()->json(['success' => true, 'data' => $device], 201);
    }

    /**
     * Détails d'un appareil spécifique
     */
    public function show(Request $request, $id)
    {
        $device = $request->user()->devices()->with('crop')->findOrFail($id);
        return response()->json(['success' => true, 'data' => $device]);
    }

    /**
     * Mettre à jour un appareil
     */
    public function update(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);

        if ($request->has('deleted') && $request->deleted === true) {
            $device->delete();
            return response()->json(['success' => true, 'message' => 'Appareil supprimé']);
        }

        $request->validate([
            'device_name' => 'sometimes|string|max:255',
            'location'    => 'sometimes|nullable|string|max:255',
            'status'      => 'sometimes|string',
            'crop_id'     => 'sometimes|nullable|exists:crops,id',
            'mode'        => 'sometimes|in:manual,auto',
        ]);

        $oldCropId = $device->crop_id;

        $device->update($request->only(['device_name', 'location', 'status', 'crop_id', 'mode']));
        $device->load('crop');

        if ($request->has('crop_id') && $oldCropId !== $device->crop_id) {
            $message = $device->crop
                ? "Culture \"{$device->crop->name}\" assignée à l'appareil."
                : "Aucune culture n'est désormais associée à cet appareil.";

            Notification::create([
                'user_id'   => $device->user_id,
                'device_id' => $device->id,
                'title'     => $device->crop ? 'Culture mise à jour' : 'Culture désassignée',
                'message'   => $message,
                'body'      => $message,
                'type'      => 'info',
                'is_read'   => false,
            ]);
        }

        return response()->json(['success' => true, 'data' => $device]);
    }

    /**
     * Supprimer un appareil
     */
    public function destroy(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);
        $device->delete();
        return response()->json(['success' => true, 'message' => 'Appareil supprimé avec succès']);
    }

    /**
     * Basculer l'irrigation manuellement (toggle)
     */
    public function toggleIrrigation(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);

        $lastEvent = IrrigationEvent::where('device_id', $device->id)
            ->orderBy('timestamp', 'desc')
            ->first();

        // Inverse l'état actuel
        $newAction = ($lastEvent && $lastEvent->action == 1) ? 0 : 1;

        IrrigationEvent::create([
            'device_id' => $device->id,
            'action'    => $newAction,
            'mode'      => 'manual',
            'timestamp' => now(),
        ]);

        $label = $newAction ? 'démarrée' : 'arrêtée';
        Notification::create([
            'user_id'   => $device->user_id,
            'device_id' => $device->id,
            'title'     => 'Irrigation ' . $label,
            'message'   => "L'irrigation a été {$label} manuellement.",
            'body'      => "L'irrigation a été {$label} manuellement.",
            'type'      => 'info',
            'is_read'   => false,
        ]);

        return response()->json([
            'success'    => true,
            'action'     => $newAction,
            'message'    => "Irrigation {$label}",
        ]);
    }

    /**
     * Déclencher une irrigation manuelle
     */
    public function triggerManualIrrigation(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);

        IrrigationEvent::create([
            'device_id'        => $device->id,
            'action'           => 1,
            'mode'             => 'manual',
            'duration_seconds' => $request->input('duration_seconds', null),
            'timestamp'        => now(),
        ]);

        Notification::create([
            'user_id'   => $device->user_id,
            'device_id' => $device->id,
            'title'     => 'Irrigation démarrée',
            'message'   => "L'irrigation manuelle a été déclenchée pour \"{$device->device_name}\".",
            'body'      => "L'irrigation manuelle a été déclenchée pour \"{$device->device_name}\".",
            'type'      => 'info',
            'is_read'   => false,
        ]);

        return response()->json(['success' => true, 'message' => 'Irrigation démarrée']);
    }

    /**
     * Arrêter l'irrigation
     */
    public function stopIrrigation(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);

        IrrigationEvent::create([
            'device_id' => $device->id,
            'action'    => 0,
            'mode'      => 'manual',
            'timestamp' => now(),
        ]);

        return response()->json(['success' => true, 'message' => 'Irrigation arrêtée']);
    }

    /**
     * Historique des événements d'irrigation
     */
    public function getEvents(Request $request, $id)
    {
        $request->user()->devices()->findOrFail($id);

        $events = IrrigationEvent::where('device_id', $id)
            ->orderBy('timestamp', 'desc')
            ->limit(50)
            ->get();

        return response()->json(['success' => true, 'data' => $events]);
    }

    /**
     * Réception des données capteurs (ESP32 → Backend)
     */
    public function updateSensors(Request $request, $id)
    {
        $device = Device::with('crop')->findOrFail($id);

        $data = $request->validate([
            'soil_humidity'    => 'nullable|numeric',
            'soil_temperature' => 'nullable|numeric',
            'air_temperature'  => 'nullable|numeric',
            'air_humidity'     => 'nullable|numeric',
            'water_level'      => 'nullable|numeric',
            'liters'           => 'nullable|numeric',
            'flow_rate'        => 'nullable|numeric',
            'total_volume'     => 'nullable|numeric',
            'bme_temperature'  => 'nullable|numeric',
            'bme_pressure'     => 'nullable|numeric',
            'timestamp'        => 'nullable|date',
            'manual_mode'      => 'nullable|integer',
            'pump_status'      => 'nullable|integer',
        ]);

        $timestamp = $data['timestamp'] ?? now();

        $sensorMapping = [
            'soil_humidity'    => ['name' => 'Capacitive_Soil_Humidity',    'unit' => '%'],
            'soil_temperature' => ['name' => 'DS18B20_Soil_Temperature',    'unit' => '°C'],
            'air_temperature'  => ['name' => 'BME280_Temperature',          'unit' => '°C'],
            'air_humidity'     => ['name' => 'BME280_Humidity',             'unit' => '%'],
            'water_level'      => ['name' => 'Ultrasonic_Water_Level',      'unit' => '%'],
            'flow_rate'        => ['name' => 'Flowmeter_Liters',            'unit' => 'L/min'],
            'total_volume'     => ['name' => 'Flowmeter_Total_Volume',      'unit' => 'L'],
        ];

        $sensorRows = [];
        foreach ($sensorMapping as $field => $sensor) {
            if (isset($data[$field])) {
                $sensorRows[] = [
                    'device_id'   => $device->id,
                    'sensor_name' => $sensor['name'],
                    'value'       => $data[$field],
                    'unit'        => $sensor['unit'],
                    'created_at'  => $timestamp,
                    'updated_at'  => $timestamp,
                ];
            }
        }

        if (!empty($sensorRows)) {
            SensorData::insert($sensorRows);
        }

        $device->update(['status' => 'online']);

        // ════ DÉCISION POMPE ════════════════════════════════════════

        // PRIORITÉ 1 — Switch physique : l'ESP32 gère localement, on confirme
        if (($data['manual_mode'] ?? 0) == 1) {
            return response()->json([
                'success'      => true,
                'mode'         => 'physical',
                'pump_command' => 1,
            ]);
        }

        // PRIORITÉ 2 — Mode AUTO (sera configuré plus tard)
        if ($device->mode === 'auto') {
            return response()->json([
                'success'      => true,
                'mode'         => 'auto',
                'pump_command' => $this->computeAutoCommand($device, $data),
            ]);
        }

        // PRIORITÉ 3 — Mode MANUEL : dernier ordre des boutons Dashboard
        $lastEvent = IrrigationEvent::where('device_id', $device->id)
            ->where('mode', 'manual')
            ->orderByDesc('id')   // id évite les collisions de timestamp à la même seconde
            ->first();

        $pumpCommand = ($lastEvent && (int) $lastEvent->action === 1) ? 1 : 0;

        return response()->json([
            'success'      => true,
            'mode'         => 'manual',
            'pump_command' => $pumpCommand,
        ]);
    }

    /**
     * Mode automatique avec hystérésis (anti-battement autour du seuil)
     * Sera activé quand device.mode = 'auto'
     */
    private function computeAutoCommand(Device $device, array $data): int
    {
        $crop = $device->crop;
        if (!$crop) return 0;

        $soil  = $data['soil_humidity'] ?? null;
        $water = $data['water_level']   ?? null;

        if ($soil === null) return 0;

        // Sécurité : réservoir vide → ne jamais pomper
        if ($water !== null && $water <= $crop->min_water_level) return 0;

        // Hystérésis : marge +5% pour éviter le battement au seuil
        $threshold   = $crop->humidity_threshold;
        $currentlyOn = ($data['pump_status'] ?? 0) == 1;

        if ($currentlyOn) {
            return ($soil >= $threshold + 5) ? 0 : 1;
        }
        return ($soil < $threshold) ? 1 : 0;
    }

    /**
     * Dernières valeurs de chaque capteur pour un appareil
     */
    public function latestSensorData(Request $request, $id)
    {
        $request->user()->devices()->findOrFail($id);

        $sensorNames = [
            'Capacitive_Soil_Humidity',
            'DS18B20_Soil_Temperature',
            'BME280_Temperature',
            'BME280_Humidity',
            'Ultrasonic_Water_Level',
            'Flowmeter_Liters',
            'Flowmeter_Total_Volume',
        ];

        $sensorMap = [
            'Capacitive_Soil_Humidity'  => 'soil_humidity',
            'DS18B20_Soil_Temperature'  => 'soil_temperature',
            'BME280_Temperature'        => 'air_temperature',
            'BME280_Humidity'           => 'air_humidity',
            'Ultrasonic_Water_Level'    => 'water_level',
            'Flowmeter_Liters'          => 'liters',
            'Flowmeter_Total_Volume'    => 'esp32_total_volume',
        ];

        $latest = [
            'soil_humidity'      => null,
            'soil_temperature'   => null,
            'air_temperature'    => null,
            'air_humidity'       => null,
            'water_level'        => null,
            'liters'             => null,
            'esp32_total_volume' => null,
            'total_volume'       => null,
            'timestamp'          => null,
        ];

        // Une requête par capteur sur les 100 dernières lignes — rapide avec l'index
        foreach ($sensorNames as $sensorName) {
            $row = SensorData::where('device_id', $id)
                ->where('sensor_name', $sensorName)
                ->orderByDesc('created_at')
                ->select(['sensor_name', 'value', 'created_at'])
                ->first();

            if ($row) {
                $field = $sensorMap[$sensorName] ?? null;
                if ($field) {
                    $latest[$field] = $row->value;
                    if (!$latest['timestamp'] || $row->created_at > $latest['timestamp']) {
                        $latest['timestamp'] = $row->created_at;
                    }
                }
            }
        }

        $latest['total_volume'] = round(
            (float) SensorData::where('device_id', $id)
                ->where('sensor_name', 'Flowmeter_Liters')
                ->sum('value'),
            3
        );

        return response()->json(['success' => true, 'data' => $latest]);
    }

    /**
     * Historique des capteurs (groupé par minute)
     */
    public function sensorHistory(Request $request, $id)
    {
        $request->user()->devices()->findOrFail($id);
        $limit = min((int) $request->query('limit', 24), 100);

        $historicalRows = SensorData::where('device_id', $id)
            ->whereIn('sensor_name', [
                'Capacitive_Soil_Humidity',
                'DS18B20_Soil_Temperature',
                'BME280_Temperature',
                'BME280_Humidity',
                'Ultrasonic_Water_Level',
                'Flowmeter_Liters',
            ])
            ->orderBy('created_at', 'asc')
            ->limit($limit * 6)
            ->get();

        $grouped = $historicalRows->groupBy(function ($row) {
            return $row->created_at->format('Y-m-d H:i');
        });

        $history = $grouped->map(function ($group) {
            $entry = [
                'timestamp'        => $group->first()->created_at,
                'temperature'      => null,
                'air_humidity'     => null,
                'soil_humidity'    => null,
                'soil_temperature' => null,
                'water_level'      => null,
                'liters'           => null,
            ];

            foreach ($group as $row) {
                switch ($row->sensor_name) {
                    case 'BME280_Temperature':
                        $entry['temperature'] = $row->value;
                        break;
                    case 'BME280_Humidity':
                        $entry['air_humidity'] = $row->value;
                        break;
                    case 'Capacitive_Soil_Humidity':
                        $entry['soil_humidity'] = $row->value;
                        break;
                    case 'DS18B20_Soil_Temperature':
                        $entry['soil_temperature'] = $row->value;
                        break;
                    case 'Ultrasonic_Water_Level':
                        $entry['water_level'] = $row->value;
                        break;
                    case 'Flowmeter_Liters':
                        $entry['liters'] = $row->value;
                        break;
                }
            }

            return $entry;
        })->values();

        return response()->json(['success' => true, 'data' => $history]);
    }

    /**
     * Toutes les données brutes des capteurs (50 dernières par capteur)
     */
    public function allSensorData(Request $request, $id)
    {
        $request->user()->devices()->findOrFail($id);

        // Limite aux 200 dernières lignes pour éviter les timeouts avec beaucoup de données
        $data = SensorData::where('device_id', $id)
            ->orderByDesc('created_at')
            ->limit(200)
            ->get();

        return response()->json(['success' => true, 'data' => $data]);
    }

    /**
     * Générer des données aléatoires pour les tests
     */
    public function generateRandomSensorData(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);
        $count  = min((int) $request->input('count', 10), 100);

        $sensorMapping = [
            'Capacitive_Soil_Humidity'  => ['min' => 20,  'max' => 95,  'unit' => '%'],
            'DS18B20_Soil_Temperature'  => ['min' => 15,  'max' => 40,  'unit' => '°C'],
            'BME280_Temperature'        => ['min' => 18,  'max' => 45,  'unit' => '°C'],
            'BME280_Humidity'           => ['min' => 30,  'max' => 90,  'unit' => '%'],
            'Ultrasonic_Water_Level'    => ['min' => 10,  'max' => 100, 'unit' => '%'],
            'Flowmeter_Liters'          => ['min' => 0,   'max' => 5,   'unit' => 'L/min'],
        ];

        $rows = [];
        $now  = now();

        for ($i = $count; $i >= 1; $i--) {
            $timestamp = $now->copy()->subMinutes($i * 5);
            foreach ($sensorMapping as $sensorName => $range) {
                $rows[] = [
                    'device_id'   => $device->id,
                    'sensor_name' => $sensorName,
                    'value'       => round($range['min'] + mt_rand() / mt_getrandmax() * ($range['max'] - $range['min']), 2),
                    'unit'        => $range['unit'],
                    'created_at'  => $timestamp,
                    'updated_at'  => $timestamp,
                ];
            }
        }

        SensorData::insert($rows);

        return response()->json([
            'success' => true,
            'message' => "{$count} séries de données générées avec succès",
            'count'   => count($rows),
        ]);
    }
}
