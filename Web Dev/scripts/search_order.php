<?php
libxml_use_internal_errors(true);

$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

$conn = new mysqli($servername, $db_username, $db_password, $dbname);
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $searchID = $_POST["searchID"] ?? '';
    $template = "../admin4.php";

    if (empty($searchID)) {
        die("No order ID specified.");
    }
    if (!file_exists($template)) {
        die("Template file not found.");
    }
    ob_start();
    include $template;
    $html = ob_get_clean();
    $htmlDoc = new DOMDocument();
    $htmlDoc->loadHTML($html);

    // Get order info
    $sql2 = "SELECT * FROM orders_client WHERE orderID = '" . $conn->real_escape_string($searchID) . "'";
    $result2 = $conn->query($sql2);

    if ($result2 && $result2->num_rows > 0) {
        $row = $result2->fetch_assoc();
        $tr = $htmlDoc->createElement("tr");
        foreach (['orderID', 'client_nif', 'client_name', 'stat'] as $col) {
            $td = $htmlDoc->createElement("td", htmlspecialchars($row[$col]));
            if ($col === 'stat') {
                $td->setAttribute("id", "status");
            }
            $tr->appendChild($td);
        }
        $infoTable = $htmlDoc->getElementById("infoTable");
        if ($infoTable) $infoTable->appendChild($tr);
        $infoDiv = $htmlDoc->getElementById("infoDiv");
        if ($infoDiv) $infoDiv->setAttribute("style", "display:block");
        $status = $row['stat'];
    } else {
        echo "Order not found.";
        header("Location: /DEAPC/admin4.php");
        exit;
    }

    // Get products info
    $sql1 = "SELECT * FROM orders_products WHERE orderID = '" . $conn->real_escape_string($searchID) . "'";
    $result1 = $conn->query($sql1);

    if ($result1 && $result1->num_rows > 0) {
        while ($row = $result1->fetch_assoc()) {
            $tr = $htmlDoc->createElement("tr");
            foreach (['prodID', 'prod_name', 'prod_desc', 'qty'] as $col) {
                $value = isset($row[$col]) ? htmlspecialchars($row[$col]) : '';
                $td = $htmlDoc->createElement("td", $value);
                $tr->appendChild($td);
            }
            // Add checkbox for "ADDED TO ORDER"
            $checkboxTd = $htmlDoc->createElement("td");
            $checkbox = $htmlDoc->createElement("input");
            $checkbox->setAttribute("type", "checkbox");
            $checkbox->setAttribute("name", "pRow");
            // Set checkbox state based on status
            if ($status === "CANCELLED") {
                $checkbox->setAttribute("disabled", "");
            } elseif ($status === "SENT") {
                $checkbox->setAttribute("disabled", "");
                $checkbox->setAttribute("checked", "");
            } elseif ($status === "PREPARED") {
                $checkbox->setAttribute("checked", "");
            }
            $checkboxTd->appendChild($checkbox);
            $tr->appendChild($checkboxTd);

            $orderTable = $htmlDoc->getElementById("orderTable");
            if ($orderTable) $orderTable->appendChild($tr);
        }
        $tableDiv = $htmlDoc->getElementById("tableDiv");
        if ($tableDiv) $tableDiv->setAttribute("style", "display:block");
    }

    // Add hidden input for orderID (for JS)
    $td = $htmlDoc->createElement("td");
    $hid = $htmlDoc->createElement("input");
    $hid->setAttribute("type", "hidden");
    $hid->setAttribute("id", "orderID");
    $hid->setAttribute("value", $searchID);
    $td->appendChild($hid);
    $buttonsTable = $htmlDoc->getElementById("buttonsTable");
    if ($buttonsTable) $buttonsTable->appendChild($td);

    // Add status buttons
    if ($buttonsTable) {
        $statuses = ['PREPARED', 'SENT', 'CANCELLED'];
        $tr = $htmlDoc->createElement("tr");
        foreach ($statuses as $s) {
            $td = $htmlDoc->createElement("td");
            $button = $htmlDoc->createElement("button", $s);
            $button->setAttribute("type", "button");
            $button->setAttribute("onclick", "jsSubmit(this)");
            $button->setAttribute("id", strtolower($s));
            $td->appendChild($button);
            $tr->appendChild($td);
        }
        $buttonsTable->appendChild($tr);
    }

    echo $htmlDoc->saveHTML();
}
?>