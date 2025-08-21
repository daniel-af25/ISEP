<?php

// Produtos
$product_name = $_POST['product_name'] ?? null;
$supplier = $_POST['supplier'] ?? null;
$price = $_POST['price'] ?? null;
$category = $_POST['category'] ?? null;
$notes = $_POST['notes'] ?? null;
$product_date = $_POST['product_date'] ?? ($_POST['date'] ?? null); // Use a unique name if possible

// Clientes
$client_name = $_POST['client_name'] ?? null;
$email = $_POST['email'] ?? null;
$address = $_POST['address'] ?? null;
$nif = $_POST['nif'] ?? null;
$payment_method = $_POST['payment_method'] ?? null;
$phone = $_POST['phone'] ?? null;
$client_date = $_POST['client_date'] ?? ($_POST['date'] ?? null); // Use a unique name if possible


$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

$conn = new mysqli($servername, $db_username, $db_password, $dbname);

if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

if (!empty($product_name)) {
    $stmt = $conn->prepare("INSERT INTO products (name, supplier, price, category, notes, date) VALUES (?, ?, ?, ?, ?, ?)");
    $stmt->bind_param("ssdsss", $product_name, $supplier, $price, $category, $notes, $product_date);
    $stmt->execute();
    echo $conn->error;
    $stmt->close();
    $conn->close();
    header("Location: /DEAPC/admin3.php?success=product");
    exit();
}

if (!empty($client_name)) {
    $stmt = $conn->prepare("INSERT INTO clients (nif, name, email, address, payment, phone, date) VALUES (?, ?, ?, ?, ?, ?, ?)");
    $stmt->bind_param("sssssss", $nif, $client_name, $email, $address, $payment_method, $phone, $client_date);
    $stmt->execute();
    echo $conn->error;
    $stmt->close();
    $conn->close();
    header("Location: /DEAPC/admin3.php?success=client");
    exit();
}
$conn->close();
header("Location: /DEAPC/admin3.php?success=none");
exit();
?>