<?php

use Illuminate\Support\Facades\Route;
use App\Http\Controllers\SensorController;

Route::get('/', function () {
    return view('welcome');
});

// Une seule route pour le monitoring
Route::get('/monitoring', [SensorController::class, 'index'])->name('sensors.index');