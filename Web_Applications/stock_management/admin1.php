<?php
session_start();

// Verifica se o utilizador está autenticado
if (!isset($_SESSION['username']) || $_SESSION['role'] == 'user') {
  header("Location: index.html"); // Manda para o login
  session_destroy();
  exit();
}

$nome = htmlspecialchars($_SESSION['username']);
?>
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Admin Area</title>
  <link rel="stylesheet" href="/DEAPC/styles/style.css">
</head>

<body>
  <header class="site-header">
    <div class="logo">
      <a href="index.html">
        <img src="images/logo1.png">
      </a>
    </div>
    <div class="header-text-container">
      <h1>ADMIN AREA</h1>
      <p>ADM Inventories</p>
    </div>
  </header>

  <header class="User-header">
    <p><b>User:</b> <?= $nome ?>
      <button onclick="location.href='scripts/logout.php'" class="centered">Logout</button>
    </p>
  </header>

  <button onclick="window.location.href='admin4.php'" class="admin-button">Manage or Create Order</button><br><br>
  <button onclick="window.location.href='admin3.php'" class="admin-button">Add Client or Product</button><br><br>
  <button onclick="window.location.href='admin2.php'" class="admin-button">Stock/Client Management</button><br><br>
  <button onclick="window.location.href='admin5.php'" class="admin-button">Add/View Users</button><br><br>

</body>

</html>