<?php

namespace App\Http\Controllers;

use App\Models\Device;
use App\Models\IrrigationEven;
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
        $devices = $request->user()->devices()->orderBy('created_at', 'desc')->get();
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

        $device->update($request->only(['device_name', 'location', 'status', 'crop_id']));
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
            'temperature' => 'nullable|numeric',
            'humidity'    => 'nullable|numeric',
            'water_level' => 'nullable|numeric',
            'timestamp'   => 'nullable|date',
        ]);

        $sensorRows = [];
        if (isset($data['temperature'])) {
            $sensorRows[] = [
                'device_id'   => $device->id,
                'sensor_name' => 'DHT22_Temp',
                'value'       => $data['temperature'],
                'unit'        => '°C',
                'created_at'  => $data['timestamp'] ?? now(),
                'updated_at'  => $data['timestamp'] ?? now(),
            ];
        }
        if (isset($data['humidity'])) {
            $sensorRows[] = [
                'device_id'   => $device->id,
                'sensor_name' => 'DHT22_Hum',
                'value'       => $data['humidity'],
                'unit'        => '%',
                'created_at'  => $data['timestamp'] ?? now(),
                'updated_at'  => $data['timestamp'] ?? now(),
            ];
        }
        if (isset($data['water_level'])) {
            $sensorRows[] = [
                'device_id'   => $device->id,
                'sensor_name' => 'LDR_Light',
                'value'       => $data['water_level'],
                'unit'        => '%',
                'created_at'  => $data['timestamp'] ?? now(),
                'updated_at'  => $data['timestamp'] ?? now(),
            ];
        }

        if (!empty($sensorRows)) {
            SensorData::insert($sensorRows);
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
            'action'          => 'start',
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
            'action'    => 'stop',
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

        $query = SensorData::query();
        if (Schema::hasColumn((new SensorData())->getTable(), 'device_id')) {
            $query->where('device_id', $id);
        }

        $rows = $query->whereIn('sensor_name', ['DHT22_Temp', 'DHT22_Hum', 'LDR_Light'])
            ->orderByDesc('created_at')
            ->get();

        $latest = [
            'temperature' => null,
            'humidity' => null,
            'water_level' => null,
            'timestamp' => null,
        ];

        foreach ($rows as $row) {
            if ($row->sensor_name === 'DHT22_Temp') {
                $latest['temperature'] = $row->value;
            }
            if ($row->sensor_name === 'DHT22_Hum') {
                $latest['humidity'] = $row->value;
            }
            if ($row->sensor_name === 'LDR_Light') {
                $latest['water_level'] = $row->value;
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

        $historicalRows = $query->whereIn('sensor_name', ['DHT22_Temp', 'DHT22_Hum', 'LDR_Light'])
            ->orderByDesc('created_at')
            ->limit($limit * 3)
            ->get()
            ->sortBy('created_at');

        $grouped = $historicalRows->groupBy(function ($row) {
            return $row->created_at->format('Y-m-d H:i');
        });

        $history = $grouped->map(function ($group) {
            $entry = [
                'timestamp' => $group->first()->created_at,
                'temperature' => null,
                'humidity' => null,
                'water_level' => null,
            ];

            foreach ($group as $row) {
                if ($row->sensor_name === 'DHT22_Temp') {
                    $entry['temperature'] = $row->value;
                }
                if ($row->sensor_name === 'DHT22_Hum') {
                    $entry['humidity'] = $row->value;
                }
                if ($row->sensor_name === 'LDR_Light') {
                    $entry['water_level'] = $row->value;
                }
            }

            return $entry;
        })->values();

        return response()->json(['success' => true, 'data' => $history]);
    }

    public function destroy(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);
        $device->delete();
        return response()->json(['success' => true, 'message' => 'Appareil supprimé']);
    }
}