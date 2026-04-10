<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;

class Device extends Model
{
    use HasFactory;

    protected $fillable = [
        'device_name',
        'device_key',
        'user_id',
        'status',
        'location',
        'crop_id'
    ];

    /**
     * Un appareil appartient à un utilisateur
     */
    public function user()
    {
        return $this->belongsTo(User::class);
    }

    /**
     * Un appareil peut être lié à une culture (optionnel)
     */
    public function crop()
    {
        return $this->belongsTo(Crop::class);
    }
}