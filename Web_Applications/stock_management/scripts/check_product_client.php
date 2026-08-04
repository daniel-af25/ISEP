<?php
if (session_status() === PHP_SESSION_NONE) {
    session_start();
}

// Verifica se o utilizador está autenticado
if (!isset($_SESSION['username'])) {
    header("Location: /DEAPC/index.html"); // Manda para o login 
    exit();
}

$nome = htmlspecialchars($_SESSION['username']);

$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

$conn = new mysqli($servername, $db_username, $db_password, $dbname);
if ($conn->connect_error) {
    die("<div class='error-message'>Connection failed: " . $conn->connect_error . "</div>");
}

$nif_idproduct = $_POST['nif_product'] ?? null;
$tipo = $_POST['tipo'] ?? null; // "Produto" or "Cliente"
$action = $_POST['action'] ?? null;
$message = "";
$table = '';
$id_field = '';
$row = null;

// Determine table and id field based on radio selection
if ($tipo === "Cliente") {
    $table = "clients";
    $id_field = "nif";
} elseif ($tipo === "Produto") {
    $table = "products";
    $id_field = "id";
}

// Handle update
if ($action === "update" && $nif_idproduct && $table && $id_field) {
    $fields = [];
    $params = [];
    $types = "";
    foreach ($_POST as $key => $value) {
        if ($key !== 'action' && $key !== 'nif_product' && $key !== 'tipo') {
            $fields[] = "$key=?";
            $params[] = $value;
            $types .= "s";
        }
    }
    $params[] = $nif_idproduct;
    $types .= "s";
    $sql = "UPDATE $table SET " . implode(",", $fields) . " WHERE $id_field=?";
    $stmt = $conn->prepare($sql);
    $stmt->bind_param($types, ...$params);
    if ($stmt->execute()) {
        $message = "<div class='success-message'><strong>Updated successfully.</strong></div>";
        header("Location: /DEAPC/admin2.php?msg=updated");
        exit();
    } else {
        $message = "<div class='error-message'>Update failed.</div>";
    }
}

// Handle delete
if ($action === "delete" && $nif_idproduct && $table && $id_field) {
    $sql = "DELETE FROM $table WHERE $id_field=?";
    $stmt = $conn->prepare($sql);
    $stmt->bind_param("s", $nif_idproduct);
    if ($stmt->execute()) {
        $message = "<div class='success-message'>Deleted successfully.</div>";
        header("Location: /DEAPC/admin2.php?msg=deleted");
        exit();
    } else {
        $message = "<div class='error-message'>Delete failed.</div>";
    }
}

// Fetch and display row
if ($nif_idproduct && !$action && $table && $id_field) {
    $stmt = $conn->prepare("SELECT * FROM $table WHERE $id_field = ?");
    $stmt->bind_param("s", $nif_idproduct);
    $stmt->execute();
    $result = $stmt->get_result();
    $row = $result->fetch_assoc();
}
?>
<!DOCTYPE html>
<html>
<head>
  <link rel="stylesheet" href="/DEAPC/styles/style.css">
</head>
<body>
<?php
echo $message;

if ($row) {
    echo '<div class="dynamic-form">';
    echo '<form method="POST" action="check_product_client.php">';
    foreach ($row as $key => $value) {
        $readonly = ($key === $id_field) ? 'readonly' : '';
        echo "<div class='form-row'>";
        echo "<label for='" . htmlspecialchars($key) . "'>" . htmlspecialchars(ucfirst($key)) . ":</label>";
        echo "<input id='" . htmlspecialchars($key) . "' type='text' name='" . htmlspecialchars($key) . "' value='" . htmlspecialchars($value) . "' $readonly>";
        echo "</div>";
    }
    echo "<input type='hidden' name='nif_product' value='" . htmlspecialchars($nif_idproduct) . "'>";
    echo "<input type='hidden' name='tipo' value='" . htmlspecialchars($tipo) . "'>";
    echo '<div class="form-actions">';
    echo '<button class="update" type="submit" name="action" value="update">Save Alterations</button>';
    echo '<button class="delete" type="submit" name="action" value="delete" onclick="return confirm(\'Are you sure you want to remove this record?\')">Remove</button>';
    echo '</div>';
    echo '</form></div>';
} elseif ($nif_idproduct && !$row && !$message) {
    echo '<div class="error-message"><strong>No record found.</strong></div>';
}
?>
</body>
</html>