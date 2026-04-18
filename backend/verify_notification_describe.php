<?php
$db = new PDO('mysql:host=127.0.0.1;port=3306;dbname=smart_irrigationV2R;charset=utf8mb4','root','',[PDO::ATTR_ERRMODE=>PDO::ERRMODE_EXCEPTION]);
$stmt = $db->query('DESCRIBE notifications');
while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
    echo $row['Field'] . ' ' . $row['Type'] . ' ' . $row['Null'] . ' ' . $row['Key'] . ' ' . $row['Default'] . ' ' . $row['Extra'] . "\n";
}
