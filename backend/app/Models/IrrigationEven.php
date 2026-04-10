<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;

class IrrigationEven extends Model
{
    use HasFactory;

    protected $table = 'irrigation_events';
    public $timestamps = false;

    protected $fillable = [
        'device_id',
        'action',
        'mode',
        'duration_seconds',
        'timestamp',
    ];

    public function device()
    {
        return $this->belongsTo(Device::class);
    }
}
