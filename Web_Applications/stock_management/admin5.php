<?php
session_start();

// Verifica se o utilizador está autenticado
if (!isset($_SESSION['username'])|| $_SESSION['role'] == 'user') {
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
    <title>Admin - Manage Users</title>
    <link rel="stylesheet" href="/DEAPC/styles/style.css">
</head>

<body class="general-body">
    <header class="site-header">
        <div class="logo">
            <a href="index.html">
                <img src="images/logo1.png">
            </a>
        </div>
        <div class="header-text-container">
            <h1>ADMIN AREA</h1>
            <p>USER MANAGEMENT</p>
        </div>
    </header>

    <header class="User-header">
        <p><b>User:</b> <?= $nome ?>
            <button onclick="location.href='scripts/logout.php'">Logout</button>
        </p>
    </header>

    <div style="text-align:left;">
        <button onclick="window.location.href='admin1.php'" class="go-back-btn">Go Back</button>
    </div>
    <div class="users-container">
        <form class="add-user-form" action="scripts/add_mail_users.php" method="POST">
            <h2>Add User</h2>
            <input type="text" name="username" placeholder="Username" required>
            <input type="email" name="email" placeholder="Email" required>
            <select required name="role">
                <option value="">Select Role</option>
                <option value="admin">Admin</option>
                <option value="user">User</option>
            </select>
            <button type="submit">Add User</button>
        </form>
        <div class="users-list">
            <h2>Registered Users</h2>
            <table class="users-table">
                <tr>
                    <th>Username</th>
                    <th>Email</th>
                    <th>Role</th>
                    <th>Status</th>
                </tr>
                <?php include 'scripts/get_users.php'; ?>
            </table>
        </div>
    </div>
</body>

</html>