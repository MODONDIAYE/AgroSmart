<?php

use Illuminate\Http\Request;
use Illuminate\Support\Facades\Route;
use App\Http\Controllers\AuthController;
use App\Http\Controllers\CropsController;
use App\Http\Controllers\IrrigationTimeController;
use App\Http\Controllers\NotificationController;
use App\Http\Controllers\DeviceController; // <--- Importation directe

/* --- Routes Publiques --- */
Route::post('/login', [AuthController::class, 'login']);
Route::post('/register', [AuthController::class, 'register']);

/* --- Routes Protégées --- */
Route::middleware('auth:sanctum')->group(function () {
    
    Route::get('/user', function (Request $request) {
        return $request->user();
    });

    // Appareils (Devices)
    Route::get('/devices', [DeviceController::class, 'index']);
    Route::post('/devices', [DeviceController::class, 'store']);
    Route::get('/devices/{id}', [DeviceController::class, 'show']);
    Route::put('/devices/{id}', [DeviceController::class, 'update']);
    Route::post('/devices/{id}/toggle', [DeviceController::class, 'toggleIrrigation']);
    Route::post('/devices/{id}/sensors', [DeviceController::class, 'updateSensors']);
    Route::get('/devices/{id}/sensors/latest', [DeviceController::class, 'latestSensorData']);
    Route::get('/devices/{id}/sensors', [DeviceController::class, 'sensorHistory']);
    Route::get('/devices/{id}/events', [DeviceController::class, 'getEvents']);
    Route::post('/devices/{id}/irrigate', [DeviceController::class, 'triggerManualIrrigation']);
    Route::post('/devices/{id}/stop', [DeviceController::class, 'stopIrrigation']);
    Route::delete('/devices/{id}', [DeviceController::class, 'destroy']);

    // Autres (Crops, Irrigation, Notifications)
    Route::get('/crops', [CropsController::class, 'index']);
    Route::post('/crops', [CropsController::class, 'store']);
    Route::delete('/crops/{id}', [CropsController::class, 'destroy']);
    Route::post('/irrigation-times', [IrrigationTimeController::class, 'store']);
    Route::delete('/irrigation-times/{id}', [IrrigationTimeController::class, 'destroy']);
    Route::get('/notifications', [NotificationController::class, 'index']);
    Route::put('/notifications/{id}/read', [NotificationController::class, 'markAsRead']);
});