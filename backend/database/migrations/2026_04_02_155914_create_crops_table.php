<?php
use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration {
    public function up() {
        Schema::create('crops', function (Blueprint $table) {
            $table->id();
            $table->foreignId('user_id')->constrained()->onDelete('cascade');
            $table->string('name');
            $table->integer('water_need'); // Ex: 25
            $table->text('description')->nullable(); // Ex: Besoins hydriques modérés
            $table->integer('humidity_threshold'); // Ex: 60
            $table->integer('min_water_level'); // Ex: 30
            $table->integer('irrigations_per_day'); // Ex: 2
            $table->boolean('is_predefined')->default(false); // 1 pour les cultures par défaut
            $table->timestamps();
        });
    }

    public function down() {
        Schema::dropIfExists('crops');
    }
};