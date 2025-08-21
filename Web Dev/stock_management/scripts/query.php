<?php
/*session_start();
 if(!isset($_SESSION['logedin'])){
     header('Location:http://localhost/deapc/index.html');
     session_destroy();
 }*/


libxml_use_internal_errors(true);
$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $db = new mysqli($servername, $db_username, $db_password, $dbname);
    if ($db->connect_error) {
        die("Connection failed: " . $conn->connect_error);
    }
    $table = $_POST["table"];

    if ($table == "clients") {
        $query = "SELECT * FROM " . $table . " WHERE nif = ?";
    } elseif ($table == "products") {
        $query = "SELECT * FROM " . $table . " WHERE id = ?";
    }

    $stmt = $db->stmt_init();
    $stmt->prepare($query);
    $stmt->bind_param("s", $cond);

    $cond = $_POST["condition"];
    $stmt->execute();
    $results = $stmt->get_result();
    $out = $results->fetch_all(MYSQLI_ASSOC);
    $out[0]["table"] = $table;
    //var_export($out);
    echo json_encode($out,JSON_INVALID_UTF8_SUBSTITUTE);
    //var_export($j);

}


?>