<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class IrrigationTime extends Model
{
    protected $table = 'irrigation_times';
    protected $fillable = ['crop_id', 'time', 'order_index'];

    public function crop() {
        return $this->belongsTo(Crop::class);
    }
}