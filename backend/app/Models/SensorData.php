<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;

class SensorData extends Model
{
    use HasFactory;

    protected $table = 'sensor_data';

    protected $fillable = [
        'device_id',
        'sensor_name',
        'value',
        'unit',
    ];

    public function device()
    {
        return $this->belongsTo(Device::class);
    }
}
