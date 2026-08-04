<?php
if (session_status() === PHP_SESSION_NONE) {
    session_start();
}

// Verifica se o utilizador está autenticado
if (!isset($_SESSION['username'])) {
    header("Location: index.html"); // Manda para o login 
    exit();
}

$nome = htmlspecialchars($_SESSION['username']);

$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

$conn = new mysqli($servername, $db_username, $db_password, $dbname);
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$sql = "SELECT username, email, adm FROM users";
$result = $conn->query($sql);

while ($row = $result->fetch_assoc()) {
    $role = $row['adm'] ? 'Admin' : 'User';
    echo "<tr>
        <td>{$row['username']}</td>
        <td>{$row['email']}</td>
        <td>{$role}</td>
        <td><span class='status-offline'>Offline</span></td>
    </tr>";
}

$conn->close();
?>