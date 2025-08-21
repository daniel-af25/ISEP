<?php
$servername = "localhost";
$db_username = "datfonso25";
$db_password = "lasanha123";
$dbname = "deapc";

$conn = new mysqli($servername, $db_username, $db_password, $dbname);
if ($conn->connect_error) {
    echo json_encode([["noResult" => true, "error" => "DB connection failed"]]);
    exit;
}

$orderID = $_POST['condition'] ?? '';
$status = $_POST['status'] ?? '';

if ($orderID && $status) {
    // Check stock availability if status is PREPARED
    if (strtoupper($status) === "PREPARED") {
        $stmt = $conn->prepare("SELECT prodID, qty FROM orders_products WHERE orderID = ?");
        $stmt->bind_param("s", $orderID);
        $stmt->execute();
        $stmt->bind_result($prodID, $ordered_qty);

        $products = [];
        while ($stmt->fetch()) {
            if (!empty($prodID)) {
                $products[] = ['prodID' => $prodID, 'qty' => $ordered_qty];
            }
        }
        $stmt->close();

        // Check stock
        $insufficient = [];
        foreach ($products as $product) {
            $stmt2 = $conn->prepare("SELECT qty FROM products WHERE id = ?");
            if ($stmt2 === false) {
                error_log("Prepare failed: " . $conn->error);
                continue;
            }
            $stmt2->bind_param("i", $product['prodID']);
            $stmt2->execute();
            $stmt2->bind_result($stock_qty);
            $stmt2->fetch();
            $stmt2->close();

            if ($stock_qty === null || $stock_qty < $product['qty']) {
                $insufficient[] = $product['prodID'];
            }
        }

        if (!empty($insufficient)) {
            echo json_encode([["noResult" => true, "error" => "Insufficient stock for product(s): " . implode(", ", $insufficient)]]);
            exit;
        }

        // DEDUCT STOCK
        foreach ($products as $product) {
            $stmt2 = $conn->prepare("UPDATE products SET qty = qty - ? WHERE id = ?");
            $stmt2->bind_param("ii", $product['qty'], $product['prodID']);
            $stmt2->execute();
            $stmt2->close();
        }
    }

    // Get previous status BEFORE updating
    $stmt = $conn->prepare("SELECT stat FROM orders_client WHERE orderID=?");
    $stmt->bind_param("s", $orderID);
    $stmt->execute();
    $stmt->bind_result($previous_status);
    $stmt->fetch();
    $stmt->close();

    $previous_status = $previous_status ? trim($previous_status) : '';

    // Update status
    $stmt = $conn->prepare("UPDATE orders_client SET stat=? WHERE orderID=?");
    $stmt->bind_param("ss", $status, $orderID);
    $stmt->execute();
    $stmt->close();

    // Only send email if previous status was PREPARED and new status is SENT
    if (strtoupper($previous_status) === "PREPARED" && strtoupper($status) === "SENT") {
        $stmt = $conn->prepare("SELECT c.email, c.name, c.payment FROM clients c JOIN orders_client o ON c.nif = o.client_nif WHERE o.orderID = ?");
        $stmt->bind_param("s", $orderID);
        $stmt->execute();
        $stmt->bind_result($client_email, $client_name, $payment_method);
        if ($stmt->fetch() && !empty($client_email)) {
            $stmt->close();

            // Calculate total price
            $total = 0.0;
            $orderDetails = "";
            $stmt2 = $conn->prepare("SELECT op.prod_name, op.qty, p.price FROM orders_products op JOIN products p ON op.prodID = p.id WHERE op.orderID = ?");
            $stmt2->bind_param("s", $orderID);
            $stmt2->execute();
            $stmt2->bind_result($prod_name, $qty, $price);
            while ($stmt2->fetch()) {
                $lineTotal = $price * $qty;
                $total += $lineTotal;
                $orderDetails .= "{$prod_name} (x{$qty}) - " . number_format($price, 2) . "€ each = " . number_format($lineTotal, 2) . "€\n";
            }
            $stmt2->close();

            $headers = "From: ADM Inventories <adm3inventarios@gmail.com>\r\n";
            $subject = "Your Order Has Been Sent!";
            $message = "Hello $client_name,\n\n";
            $message .= "Your order (ID: $orderID) has been marked as SENT and is on its way.\n\n";
            $message .= "Order details:\n$orderDetails\n";
            $message .= "Total to pay: " . number_format($total, 2) . "€\n";
            $message .= "Payment method: $payment_method\n\n";

            // Payment reminder logic
            if (!empty($payment_method)) {
                if (stripos($payment_method, "prompt") !== false) {
                    $message .= "Please be ready to pay promptly as per your payment terms.\n";
                } elseif (stripos($payment_method, "credit") !== false) {
                    $message .= "You have a credit period: $payment_method.\n";
                } else {
                    $message .= "Please follow your selected payment method: $payment_method.\n";
                }
            } else {
                $message .= "Please contact us to confirm your payment method.\n";
            }

            $message .= "\nBest regards,\nADM Team";

            // Send email and check for errors
            $sent = mail($client_email, $subject, $message, $headers);
            if (!$sent) {
                error_log("Mail function failed for $client_email");
            } else {
                error_log("Mail sent to $client_email");
            }
        } else {
            $stmt->close();
            error_log("Could not fetch client email for order $orderID");
        }
    }

    // Fetch updated status
    $stmt = $conn->prepare("SELECT stat FROM orders_client WHERE orderID=?");
    $stmt->bind_param("s", $orderID);
    $stmt->execute();
    $stmt->bind_result($newStat);
    $stmt->fetch();
    $stmt->close();

    echo json_encode([["stat" => $newStat]]);
    exit;
} else {
    echo json_encode([["noResult" => true, "error" => "Missing orderID or status"]]);
    exit;
}
?>
