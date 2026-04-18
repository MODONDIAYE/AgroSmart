<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        if (!Schema::hasTable('sensor_data')) {
            Schema::create('sensor_data', function (Blueprint $table) {
                $table->id();
                $table->foreignId('device_id')->nullable()->constrained()->onDelete('cascade');
                $table->string('sensor_name');
                $table->float('value', 10, 2)->nullable();
                $table->string('unit')->nullable();
                $table->timestamps();
            });
            return;
        }

        Schema::table('sensor_data', function (Blueprint $table) {
            if (!Schema::hasColumn('sensor_data', 'device_id')) {
                $table->foreignId('device_id')->nullable()->constrained()->onDelete('cascade')->after('id');
            }
            if (!Schema::hasColumn('sensor_data', 'sensor_name')) {
                $table->string('sensor_name')->after('device_id');
            }
            if (!Schema::hasColumn('sensor_data', 'value')) {
                $table->float('value', 10, 2)->nullable()->after('sensor_name');
            }
            if (!Schema::hasColumn('sensor_data', 'unit')) {
                $table->string('unit')->nullable()->after('value');
            }
            if (!Schema::hasColumn('sensor_data', 'created_at')) {
                $table->timestamps();
            }
        });
    }

    public function down(): void
    {
        if (!Schema::hasTable('sensor_data')) {
            return;
        }

        Schema::table('sensor_data', function (Blueprint $table) {
            if (Schema::hasColumn('sensor_data', 'unit')) {
                $table->dropColumn('unit');
            }
            if (Schema::hasColumn('sensor_data', 'value')) {
                $table->dropColumn('value');
            }
            if (Schema::hasColumn('sensor_data', 'sensor_name')) {
                $table->dropColumn('sensor_name');
            }
            if (Schema::hasColumn('sensor_data', 'device_id')) {
                $table->dropForeign(['device_id']);
                $table->dropColumn('device_id');
            }
        });
    }
};
