<?php
$db = new PDO('mysql:host=127.0.0.1;port=3306;dbname=smart_irrigationV2R;charset=utf8mb4','root','',[PDO::ATTR_ERRMODE=>PDO::ERRMODE_EXCEPTION]);
$cols = $db->query('SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = "smart_irrigationV2R" AND TABLE_NAME = "notifications" ORDER BY ORDINAL_POSITION')->fetchAll(PDO::FETCH_COLUMN);
echo 'COLUMNS=' . implode(',', $cols) . "\n";
$usr = $db->query('SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = "smart_irrigationV2R" AND TABLE_NAME = "notifications" AND COLUMN_NAME LIKE "%user%"')->fetchAll(PDO::FETCH_COLUMN);
echo 'USER COLUMNS=' . implode(',', $usr) . "\n";
$create = $db->query('SHOW CREATE TABLE notifications')->fetch(PDO::FETCH_ASSOC);
echo $create['Create Table'] . "\n";
