<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     */
public function up()
{
    Schema::create('devices', function (Blueprint $table) {
        $table->id();
        $table->string('device_name');
        $table->string('device_key')->unique();
        $table->foreignId('user_id')->constrained()->onDelete('cascade');
        $table->string('status')->default('offline');
        $table->string('location')->nullable();
        $table->timestamps();
    });
}
    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('devices');
    }
};
