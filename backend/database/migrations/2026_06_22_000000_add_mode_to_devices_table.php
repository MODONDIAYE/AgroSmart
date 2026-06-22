<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::table('devices', function (Blueprint $table) {
            if (!Schema::hasColumn('devices', 'mode')) {
                // 'manual' = boutons app | 'auto' = seuils culture (à venir)
                $table->string('mode')->default('manual')->after('status');
            }
        });
    }

    public function down(): void
    {
        Schema::table('devices', function (Blueprint $table) {
            if (Schema::hasColumn('devices', 'mode')) {
                $table->dropColumn('mode');
            }
        });
    }
};
