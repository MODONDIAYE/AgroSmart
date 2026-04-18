<?php
$db = new PDO('mysql:host=127.0.0.1;port=3306;dbname=smart_irrigationV2R;charset=utf8mb4','root','',[PDO::ATTR_ERRMODE=>PDO::ERRMODE_EXCEPTION]);
$cols = $db->query('SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = "smart_irrigationV2R" AND TABLE_NAME = "notifications"')->fetchAll(PDO::FETCH_COLUMN);
echo 'COLUMNS=' . implode(',', $cols) . "\n";
$user = $db->query('SELECT id FROM users LIMIT 1')->fetch(PDO::FETCH_ASSOC);
$device = $db->query('SELECT id FROM devices LIMIT 1')->fetch(PDO::FETCH_ASSOC);
if (! $user || ! $device) {
    echo 'NO_USER_OR_DEVICE\n';
    exit(1);
}
$stmt = $db->prepare('INSERT INTO notifications (user_id,device_id,title,message,body,type,is_read,created_at,updated_at) VALUES (?,?,?,?,?,?,?,?,?)');
$stmt->execute([$user['id'], $device['id'], 'Verification', 'Test message', 'Test body', 'info', 0, date('Y-m-d H:i:s'), date('Y-m-d H:i:s')]);
echo 'INSERT_OK\n';
$row = $db->query('SELECT id,body FROM notifications ORDER BY id DESC LIMIT 1')->fetch(PDO::FETCH_ASSOC);
echo 'LAST=' . $row['id'] . ',' . $row['body'] . "\n";
