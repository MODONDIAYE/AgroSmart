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
        ]);

        $oldCropId = $device->crop_id;

        $device->update($request->only(['device_name', 'location', 'status', 'crop_id']));
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
        $device = Device::findOrFail($id);

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
        ]);

        $timestamp = $data['timestamp'] ?? now();

        // Mapping champ → sensor_name + unité
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

        // Mise à jour du statut de l'appareil
        $device->update(['status' => 'online']);
        $device->load('crop');
        $crop = $device->crop;

        // Alertes basées sur les seuils de la culture
        if ($crop && !empty($sensorRows)) {
            $notifications = [];

            if (isset($data['soil_humidity']) && $data['soil_humidity'] < $crop->humidity_threshold) {
                $notifications[] = [
                    'title'   => 'Humidité du sol trop basse',
                    'message' => "L'humidité du sol est de {$data['soil_humidity']}% — inférieure au seuil de {$crop->humidity_threshold}% pour {$crop->name}.",
                    'type'    => 'warning',
                ];
            }

            if (isset($data['water_level']) && $data['water_level'] < $crop->min_water_level) {
                $notifications[] = [
                    'title'   => "Niveau d'eau faible",
                    'message' => "Le niveau d'eau est de {$data['water_level']}% — inférieur au seuil minimum de {$crop->min_water_level}%.",
                    'type'    => 'warning',
                ];
            }

            foreach ($notifications as $notification) {
                Notification::create([
                    'user_id'   => $device->user_id,
                    'device_id' => $device->id,
                    'title'     => $notification['title'],
                    'message'   => $notification['message'],
                    'body'      => $notification['message'],
                    'type'      => $notification['type'],
                    'is_read'   => false,
                ]);
            }
        }

        // --- Décision de la commande pompe envoyée à l'ESP32 ---

        // 1. Dernier ordre Web enregistré en BDD (boutons Irriguer / Arrêter du dashboard)
        $lastEvent = IrrigationEvent::where('device_id', $device->id)
            ->orderBy('timestamp', 'desc')
            ->first();
        $webCommandActive = ($lastEvent && $lastEvent->action == 1);

        // 2. Condition automatique basée sur les capteurs et les seuils de la culture
        $autoConditionActive = false;
        if ($crop && isset($data['soil_humidity']) && isset($data['water_level'])) {
            if (
                $data['soil_humidity'] < $crop->humidity_threshold &&
                $data['water_level'] >= $crop->min_water_level
            ) {
                $autoConditionActive = true;
            }
        }

        // 3. Décision finale : ON si commande Web OU condition auto
        $pumpCommand = ($webCommandActive || $autoConditionActive) ? 1 : 0;

        // 4. Sécurité absolue : cuve vide → pompe coupée quoi qu'il arrive
        if (isset($data['water_level']) && $data['water_level'] <= 2) {
            $pumpCommand = 0;
        }

        // 5. Si l'ESP32 signale que le switch physique est sur ON,
        //    on ne lui envoie pas d'ordre contradictoire (il gère lui-même)
        if (isset($data['manual_mode']) && $data['manual_mode'] == 1) {
            $pumpCommand = -1; // valeur sentinelle → ignorée côté Arduino
        }

        return response()->json([
            'success'      => true,
            'message'      => 'Données capteurs reçues avec succès',
            'pump_command' => $pumpCommand,
        ]);
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

        // Une seule requête pour tous les capteurs, on garde la plus récente par capteur
        $rows = SensorData::where('device_id', $id)
            ->whereIn('sensor_name', $sensorNames)
            ->orderByDesc('created_at')
            ->get()
            ->unique('sensor_name');

        foreach ($rows as $row) {
            $field = $sensorMap[$row->sensor_name] ?? null;
            if ($field) {
                $latest[$field] = $row->value;
                if (!$latest['timestamp'] || $row->created_at > $latest['timestamp']) {
                    $latest['timestamp'] = $row->created_at;
                }
            }
        }

        // Volume total cumulé (somme de tous les débits enregistrés)
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
     * Toutes les données brutes des capteurs (avec pagination)
     */
    public function allSensorData(Request $request, $id)
    {
        $request->user()->devices()->findOrFail($id);

        $data = SensorData::where('device_id', $id)
            ->orderByDesc('created_at')
            ->paginate(50);

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
