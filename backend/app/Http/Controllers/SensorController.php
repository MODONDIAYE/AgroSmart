<?php

namespace App\Http\Controllers;

use Illuminate\Http\Request;
use App\Models\Sensor; // On utilise 'Sensor' comme vu dans votre dossier Models

class SensorController extends Controller
{
    public function index()
    {
        // On récupère les données via le modèle Sensor
        $readings = Sensor::orderBy('created_at', 'desc')->paginate(15);

        return view('sensors.index', compact('readings'));
    }
}