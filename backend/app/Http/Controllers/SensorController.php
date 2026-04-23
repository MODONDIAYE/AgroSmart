<?php

namespace App\Http\Controllers;

use Illuminate\Http\Request;
use App\Models\Sensor;
use Illuminate\Support\Facades\Log;

class SensorController extends Controller
{
    // Affiche les données sur ton interface web
    public function index()
    {
        $readings = Sensor::orderBy('created_at', 'desc')->paginate(15);
        return view('sensors.index', compact('readings'));
    }

    // Méthode appelée par l'ESP32
    public function store(Request $request)
    {
        // 1. Validation des données entrantes
        $validated = $request->validate([
            'temp' => 'required|numeric',
            'hum'  => 'required|numeric',
        ]);

        // 2. Enregistrement en base de données
        try {
            Log::info('Tentative d\'enregistrement des données du capteur', $validated);
            
            $reading = new Sensor();
            $reading->temperature = $request->input('temp');
            $reading->humidite = $request->input('hum');
            $reading->save();

            Log::info('Données enregistrées avec succès', ['id' => $reading->id]);

            return response()->json([
                'status' => 'Success',
                'message' => 'Mesure enregistrée avec succès',
                'data' => $reading
            ], 201);

        } catch (\Exception $e) {
            Log::error('Erreur lors de l\'enregistrement du capteur', [
                'error' => $e->getMessage(),
                'file' => $e->getFile(),
                'line' => $e->getLine(),
                'trace' => $e->getTraceAsString()
            ]);
            
            return response()->json([
                'status' => 'Error',
                'message' => 'Erreur lors de l\'enregistrement',
                'debug' => env('APP_DEBUG') ? $e->getMessage() : null
            ], 500);
        }
    }
}