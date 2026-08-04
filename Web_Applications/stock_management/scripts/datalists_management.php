<?php
// Connect to your DB
$conn = new mysqli("localhost", "datfonso25", "lasanha123", "deapc");
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}
$type = $_GET['type'] ?? ''; // <-- THIS LINE IS NEEDED!
if ($type === 'products') {
    $result = $conn->query("SELECT id, name FROM products");
    while ($row = $result->fetch_assoc()) {
        echo '<option value="' . htmlspecialchars($row['id']) . '">' . htmlspecialchars($row['id'] . ' - ' . $row['name']) . '</option>';
    }
} elseif ($type === 'clients') {
    $result = $conn->query("SELECT nif, name FROM clients");
    while ($row = $result->fetch_assoc()) {
        echo '<option value="' . htmlspecialchars($row['nif']) . '">' . htmlspecialchars($row['nif'] . ' - ' . $row['name']) . '</option>';
    }
}
$conn->close();
?>