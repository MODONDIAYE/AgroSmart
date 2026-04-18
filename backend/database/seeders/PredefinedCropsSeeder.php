<?php

namespace Database\Seeders;

use App\Models\Crops;
use App\Models\User;
use Illuminate\Database\Seeder;

class PredefinedCropsSeeder extends Seeder
{
    public function run(): void
    {
        $user = User::first();
        if (!$user) {
            return;
        }

        $predefined = [
            [
                'name' => 'Tomate',
                'water_need' => 800,
                'humidity_threshold' => 40,
                'min_water_level' => 20,
                'irrigations_per_day' => 2,
                'description' => 'Besoin hydrique élevé, adapté aux serres et potagers.',
            ],
            [
                'name' => 'Laitue',
                'water_need' => 400,
                'humidity_threshold' => 50,
                'min_water_level' => 25,
                'irrigations_per_day' => 3,
                'description' => 'Culture à faible enracinement et besoin d’humidité régulier.',
            ],
            [
                'name' => 'Maïs',
                'water_need' => 600,
                'humidity_threshold' => 35,
                'min_water_level' => 15,
                'irrigations_per_day' => 1,
                'description' => 'Culture résistante, mais qui profite d’un apport régulier en eau.',
            ],
            [
                'name' => 'Oignon',
                'water_need' => 350,
                'humidity_threshold' => 45,
                'min_water_level' => 20,
                'irrigations_per_day' => 2,
                'description' => 'Cultivé pour des besoins hydriques modérés et un bon drainage.',
            ],
            [
                'name' => 'Piment',
                'water_need' => 500,
                'humidity_threshold' => 42,
                'min_water_level' => 18,
                'irrigations_per_day' => 2,
                'description' => 'Culture sensible aux variations d’humidité, préfère des apports réguliers.',
            ],
            [
                'name' => 'Aubergine',
                'water_need' => 700,
                'humidity_threshold' => 45,
                'min_water_level' => 22,
                'irrigations_per_day' => 2,
                'description' => 'Exige une humidité stable et des apports d’eau fréquents.',
            ],
            [
                'name' => 'Carotte',
                'water_need' => 450,
                'humidity_threshold' => 40,
                'min_water_level' => 20,
                'irrigations_per_day' => 2,
                'description' => 'Préférée pour des sols bien drainés et des apports réguliers.',
            ],
            [
                'name' => 'Gombo',
                'water_need' => 550,
                'humidity_threshold' => 38,
                'min_water_level' => 15,
                'irrigations_per_day' => 2,
                'description' => 'Culture tropicale avec un besoin d’eau élevé et une tolérance à la chaleur.',
            ],
        ];

        foreach ($predefined as $cropData) {
            Crops::updateOrCreate(
                [
                    'user_id' => $user->id,
                    'name' => $cropData['name'],
                    'is_predefined' => true,
                ],
                array_merge($cropData, ['user_id' => $user->id, 'is_predefined' => true])
            );
        }
    }
}
