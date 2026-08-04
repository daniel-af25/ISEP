<?php
$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

// Conexão
$conn = new mysqli($servername, $db_username, $db_password, $dbname);
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Iniciar sessão
session_start();

// Receber dados do formulário
$username = $_POST['username'];
$password = $_POST['password'];

// Consulta SQL para obter o utilizador
$sql = "SELECT * FROM users WHERE username = ?";
$stmt = $conn->prepare($sql);
$stmt->bind_param("s", $username);
$stmt->execute();
$result = $stmt->get_result();

// Verifica se encontrou o utilizador
if ($result->num_rows === 1) {
    $user = $result->fetch_assoc();

    // Verificar se a password está correta
     if ($password === $user['passw']) {
        $_SESSION['username'] = $user['username'];
        $_SESSION['role'] = $user['adm'] == 1 ? 'admin' : 'user';

        // Redirecionar
        if ($user['adm'] == 1) {
            header("Location: /DEAPC/admin1.php");
        } else {
            header("Location: /DEAPC/uti1.php");
        }
        exit();
    } else {
        echo "<script>alert('Password incorreta'); window.location.href = '/DEAPC/Index.html';</script>";
    }
} else {
    echo "<script>alert('Utilizador não encontrado'); window.location.href = '/DEAPC/Index.html';</script>";
}

$conn->close();
?>