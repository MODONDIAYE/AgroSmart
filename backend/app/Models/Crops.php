<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;

class Crops extends Model
{
    use HasFactory;

    protected $fillable = [
        'user_id', 'name', 'water_need', 'description', 
        'humidity_threshold', 'min_water_level', 
        'irrigations_per_day', 'is_predefined'
    ];

    public function user() {
        return $this->belongsTo(User::class);
    }

    public function irrigationTimes() {
        return $this->hasMany(IrrigationTime::class);
    }
}