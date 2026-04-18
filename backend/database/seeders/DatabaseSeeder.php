<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use App\Models\User;
use App\Models\Crops;
use Illuminate\Support\Facades\Hash;

class DatabaseSeeder extends Seeder
{
    /**
     * Seed the application's database.
     */
    public function run(): void
    {
        // 1. Création de l'utilisateur de test
        // On utilise updateOrCreate pour éviter les erreurs si on lance le seeder 2 fois
        $user = User::updateOrCreate(
            ['email' => 'modou@test.com'],
            [
                'username' => 'modou',
                'password' => Hash::make('password123'),
                'phone'    => '771234567',
            ]
        );

        // 2. Appel des seeders de données
        $this->call([
            PredefinedCropsSeeder::class,
            SensorDataSeeder::class,
        ]);
    }
}