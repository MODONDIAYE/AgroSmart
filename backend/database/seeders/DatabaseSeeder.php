<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use App\Models\User;
use Illuminate\Support\Facades\Hash;

class DatabaseSeeder extends Seeder
{
    /**
     * Seed the application's database.
     */
    public function run(): void
    {
        // 1. Appel du seeder de données capteurs
        $this->call([
            SensorDataSeeder::class,
        ]);

        // 2. Création de l'utilisateur de test
        // On utilise updateOrCreate pour éviter les erreurs si on lance le seeder 2 fois
        User::updateOrCreate(
            ['email' => 'modou@test.com'],
            [
                'username' => 'modou',
                'password' => Hash::make('password123'),
                'phone'    => '771234567',
            ]
        );
    }
}