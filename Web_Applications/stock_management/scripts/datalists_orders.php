<?php
$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

$conn = new mysqli($servername, $db_username, $db_password, $dbname);
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}
$type = $_GET['type'] ?? '';

if ($type === 'orders') {
    $sql = "SELECT orderID, stat, client_nif FROM orders_client WHERE stat IN ('NEW', 'PREPARED')";
    $result = $conn->query($sql);

    if ($result) {
        while ($row = $result->fetch_assoc()) {
            $label = "{$row['stat']} - {$row['orderID']} - {$row['client_nif']}";
            echo "<option value=\"{$row['orderID']}\" label=\"$label\"></option>";
        }
    }
    $conn->close();
    exit;
}

if ($type === 'products') {
    $sql = "SELECT id, name FROM products";
    $result = $conn->query($sql);

    if ($result && $result->num_rows > 0) {
        while ($row = $result->fetch_assoc()) {
            echo '<option value="' . htmlspecialchars($row['id']) . '">' . htmlspecialchars($row['id']) . ' - ' . htmlspecialchars($row['name']) . '</option>';
        }
    }
} elseif ($type === 'clients') {
    $sql = "SELECT nif, name FROM clients";
    $result = $conn->query($sql);

    if ($result && $result->num_rows > 0) {
        while ($row = $result->fetch_assoc()) {
            echo '<option value="' . htmlspecialchars($row['nif']) . '">' . htmlspecialchars($row['nif']) . ' - ' . htmlspecialchars($row['name']) . '</option>';
        }
    }
}

$conn->close();
?>