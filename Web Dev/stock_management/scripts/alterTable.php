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
    $query = "UPDATE orders_client SET stat=' " . $_POST["status"] . "' WHERE orderID = ?";
    $query2 = "SELECT * FROM orders_client WHERE orderID = ?";
    $products = "SELECT prodID from orders_products WHERE orderID = ?";
    $restock = "UPDATE products set qty =(select qty from orders_products where orderID=? AND prodID =?)+ (select qty from products where id=?)where id=?";

    $stmt = $db->stmt_init();

    $stmt->prepare($query2);
    $stmt->bind_param("s", $cond);
    $cond = $_POST["condition"];
    $stmt->execute();
    $results = $stmt->get_result();
    $out = $results->fetch_all(MYSQLI_ASSOC);
    $prevStat = trim($out[0]["stat"]);

    $stmt->prepare($query);
    $stmt->bind_param("s", $cond);



    if ($stmt->execute()) {

        if (strcmp($_POST["status"], "CANCELLED") == 0 && strcmp($prevStat, "CANCELLED") != 0) {
            $stmt->prepare($products);
            $stmt->bind_param("s", $cond);

            $stmt->execute();
            $results = $stmt->get_result();
            $rows = $results->fetch_all(MYSQLI_ASSOC);

            $stmt->prepare($restock);
            $stmt->bind_param("ssss", $cond, $id, $id, $id);
            foreach ($rows as $prodID) {
                $id = $prodID["prodID"];
                $stmt->execute();
            }
            $stmt->prepare($query2);
            $stmt->bind_param("s", $cond);

            $stmt->execute();
            $results = $stmt->get_result();
            $out = $results->fetch_all(MYSQLI_ASSOC);

            $out["cancelled"] = true;
            //$out["prevstat"]=$prevStat;
            echo json_encode($out);
        } else {
            $stmt->prepare($query2);
            $stmt->bind_param("s", $cond);

            $stmt->execute();
            $results = $stmt->get_result();
            $out = $results->fetch_all(MYSQLI_ASSOC);
            echo json_encode($out);
        }




    } else {
        $out["noResult"] = true;
        echo json_encode($out);
    }
    /*update products set qty =(select qty from orders_products where prod_name='box')+ (select qty from products where id=1)where id=1;
    update products set qty =(select qty from orders_products where orderID='123DRE' AND prodID =)+ (select qty from products where id=1)where id=1;*/

}


?>