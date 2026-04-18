<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;
use App\Models\Device;

class Notification extends Model
{
    use HasFactory;

    protected $fillable = [
        'user_id',
        'device_id',
        'title',
        'message',
        'body',
        'type',
        'is_read',
    ];

    public function device()
    {
        return $this->belongsTo(Device::class);
    }
}