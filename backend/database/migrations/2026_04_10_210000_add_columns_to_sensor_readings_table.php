<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     *
     * @return void
     */
    public function up()
    {
        Schema::table('sensor_readings', function (Blueprint $table) {
            if (!Schema::hasColumn('sensor_readings', 'nbrDeLitre')) {
                $table->integer('nbrDeLitre')->nullable()->after('timestamp');
            }
            if (!Schema::hasColumn('sensor_readings', 'Date')) {
                $table->date('Date')->nullable()->after('nbrDeLitre');
            }
            if (!Schema::hasColumn('sensor_readings', 'Heure')) {
                $table->time('Heure')->nullable()->after('Date');
            }
        });
    }

    /**
     * Reverse the migrations.
     *
     * @return void
     */
    public function down()
    {
        Schema::table('sensor_readings', function (Blueprint $table) {
            if (Schema::hasColumn('sensor_readings', 'Heure')) {
                $table->dropColumn('Heure');
            }
            if (Schema::hasColumn('sensor_readings', 'Date')) {
                $table->dropColumn('Date');
            }
            if (Schema::hasColumn('sensor_readings', 'nbrDeLitre')) {
                $table->dropColumn('nbrDeLitre');
            }
        });
    }
};
