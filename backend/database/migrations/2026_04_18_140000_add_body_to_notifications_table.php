<?php
use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration {
    public function up() {
        Schema::table('notifications', function (Blueprint $table) {
            if (!Schema::hasColumn('notifications', 'body')) {
                $table->text('body')->nullable()->after('message');
            }
        });
    }

    public function down() {
        Schema::table('notifications', function (Blueprint $table) {
            if (Schema::hasColumn('notifications', 'body')) {
                $table->dropColumn('body');
            }
        });
    }
};
