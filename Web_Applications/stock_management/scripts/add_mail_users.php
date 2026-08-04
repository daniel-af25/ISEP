<?php
session_start();

// Verifica se o utilizador está autenticado
if (!isset($_SESSION['username'])) {
    header("Location: index.html"); // Manda para o login 
    exit();
}

$nome = htmlspecialchars($_SESSION['username']);


$form_username = $_POST['username'] ?? null;
$form_email = $_POST['email'] ?? null;
$form_role = $_POST['role'] ?? null;

if ($form_role == "admin") {
    $role_value = 1;
} else {
    $role_value = 0;
}

$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

$conn = new mysqli($servername, $db_username, $db_password, $dbname);

if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

if (!empty($form_username) && !empty($form_email)) {
    $stmt = $conn->prepare("INSERT INTO users (username, email, adm) VALUES (?, ?, ?)");
    $stmt->bind_param("ssi", $form_username, $form_email, $role_value);
    $stmt->execute();
    $stmt->close();

    $stmt = $conn->prepare("SELECT passw FROM users WHERE username = ?");
    $stmt->bind_param("s", $form_username);
    $stmt->execute();
    $stmt->bind_result($generated_password);
    $stmt->fetch();
    $stmt->close();
}

$headers = "From: ADM Inventories <adm3inventarios@gmail.com>\r\n";
$subject = "Welcome to ADM inventories!";
$message = "Hello $form_username,\n\nYour account has been created successfully.\nYour username is $form_username and your password is $generated_password.\n\nBest regards,\nADM Team";
mail($form_email, $subject, $message, $headers);
header("Location: /DEAPC/admin5.php");
exit();
?>