<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;

class Sensor extends Model
{
    use HasFactory;

    // Indique à Laravel que le modèle utilise la table 'mesures'
    protected $table = 'mesures';

    // Désactiver les timestamps automatiques de Laravel
    // car la table utilise 'date_enregistrement' au lieu de 'created_at' et 'updated_at'
    public $timestamps = false;

    // Autorise l'insertion massive pour ces colonnes
    protected $fillable = [
        'temperature',
        'humidite'
    ];
}