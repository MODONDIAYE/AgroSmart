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
    Schema::create('irrigation_events', function (Blueprint $table) {
        $table->id();
        $table->foreignId('device_id')->constrained()->onDelete('cascade');
        $table->string('action');
        $table->string('mode')->default('Manual');
        $table->integer('duration_seconds')->nullable();
        $table->timestamp('timestamp')->useCurrent();
    });
}
    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('irrigation_events');
    }
};
