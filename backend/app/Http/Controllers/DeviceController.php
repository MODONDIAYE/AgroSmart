<?php

namespace App\Http\Controllers;

use App\Models\Device;
use Illuminate\Http\Request;
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
        // Ici, on cherche par ID sans forcément être connecté (pour l'ESP32)
        $device = Device::findOrFail($id);
        
        // Logique de stockage des données (ex: humidité, température)
        return response()->json([
            'success' => true,
            'message' => 'Données capteurs reçues avec succès'
        ]);
    }
    public function destroy(Request $request, $id)
    {
        $device = $request->user()->devices()->findOrFail($id);
        $device->delete();
        return response()->json(['success' => true, 'message' => 'Appareil supprimé']);
    }
}