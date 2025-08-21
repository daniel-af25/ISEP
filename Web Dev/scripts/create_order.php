<?php
$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

$conn = new mysqli($servername, $db_username, $db_password, $dbname);
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $clientNIF = $_POST['clientNIF'] ?? '';
    $codes = $_POST['code'] ?? [];
    $quantities = $_POST['quantity'] ?? [];

    // 1. Get client name from clients table
    $stmt = $conn->prepare("SELECT name FROM clients WHERE nif = ?");
    $stmt->bind_param("s", $clientNIF);
    $stmt->execute();
    $stmt->bind_result($clientName);
    if (!$stmt->fetch()) {
        die("Client NIF not found.");
    }
    $stmt->close();

    // 2. Generate a new orderID (e.g., random string)
    $orderID = strtoupper(substr(md5(uniqid(rand(), true)), 0, 6));

    // 3. Insert into orders_client
    $stmt = $conn->prepare("INSERT INTO orders_client (orderID, client_nif, client_name, stat) VALUES (?, ?, ?, 'NEW')");
    $stmt->bind_param("sss", $orderID, $clientNIF, $clientName);
    if (!$stmt->execute()) {
        die("Failed to create order.");
    }
    $stmt->close();

    // 4. Insert each product into orders_products
    for ($i = 0; $i < count($codes); $i++) {
        $prodID = $codes[$i];
        $qty = (int)$quantities[$i];

        if (empty($prodID)) {
            continue;
        }

        // Get product name and category
        $stmt = $conn->prepare("SELECT name, category FROM products WHERE id = ?");
        $stmt->bind_param("s", $prodID);
        $stmt->execute();
        $stmt->bind_result($prodName, $prodCategory);
        if (!$stmt->fetch()) {
            $prodName = '';
            $prodCategory = '';
        }
        $stmt->close();

        // Insert into orders_products (using prod_desc as category)
        $stmt = $conn->prepare("INSERT INTO orders_products (orderID, prodID, prod_name, prod_desc, qty) VALUES (?, ?, ?, ?, ?)");
        $stmt->bind_param("ssssi", $orderID, $prodID, $prodName, $prodCategory, $qty);
        if (!$stmt->execute()) {
            die("Failed to insert product: " . $stmt->error);
        }
        $stmt->close();
    }

header("Location: /DEAPC/admin4.php?order_success=1&order_id=" . urlencode($orderID));
}
?>