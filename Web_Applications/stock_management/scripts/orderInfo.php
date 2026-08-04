<?php
session_start();

// Verifica se o utilizador está autenticado
if (!isset($_SESSION['username'])) {
  header("Location: index.html"); // Manda para o login 
  exit();
}

$nome = htmlspecialchars($_SESSION['username']);



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
    $sql1 = "select * from orders_products where orderID = \"" . $_POST["searchID"] . "\"";
    $sql2 = "select * from orders_client where orderID = \"" . $_POST["searchID"] . "\"";


    $url = "localhost/deapc/" . $_POST["page"];

    $htmlDoc = new DOMDocument('1.0', 'utf-8');
    
    $htmlDoc->loadHTMLFile($_POST["page"]);
    $htmlDoc->getElementById("cssLink")->setAttribute("href","../styles/style.css");
      
    $goBack=$htmlDoc->createDocumentFragment();$logout=$htmlDoc->createDocumentFragment();
    $goBack->appendXML("<p><button onClick=\"document.location='../uti1.php'\" class=\"go-back-btn\" id=\"goBack\">Go Back</button></p>");
    $logout->appendXML("<b>User: ".$nome."</b><button onclick=\"location.href='logout.php'\">Logout</button>");
    //$htmlDoc->getElementById("divGoBack")->removeChild($htmlDoc->getElementById("divGoBack")->firstChild);
    $htmlDoc->getElementById("divGoBack")->appendChild($goBack);
    $htmlDoc->getElementById("userLogout")->appendChild($logout);
    $htmlDoc->getElementById("form")->setAttribute("action","orderInfo.php");
    $htmlDoc->getElementsByTagName("img")->item(0)->setAttribute("src","../images/logo1.png");

    $results = $db->query($sql2);

    if ($results->num_rows > 0) {
        while ($row = $results->fetch_assoc()) {
            $tr = $htmlDoc->createElement("tr", "");
            $tr->setAttribute("class", "label-font2");
            foreach ($row as $v) {
                $td = $htmlDoc->createElement("td", $v);
                $td->setAttribute("class", "label-font2");
                $tr->appendChild($td);
            }
            $td->setAttribute("id", "status");
            $htmlDoc->getElementById("infoTable")->appendChild($tr);

        }
        $htmlDoc->getElementById("infoDiv")->setAttribute("style", "display:block");

        //echo $htmlDoc->saveHTML();
    } else {
        echo $htmlDoc->saveHTML();
    }
    $status = trim($td->nodeValue);

    $checkbox = $htmlDoc->createElement("input", "");

    $results = $db->query($sql1);
    if ($results->num_rows > 0) {
        while ($row = $results->fetch_assoc()) {
            $tr = $htmlDoc->createElement("tr", "");
            $tr->setAttribute("class", "label-font2");
            foreach ($row as $v) {
                if ($v != $_POST["searchID"]) {
                    $td = $htmlDoc->createElement("td", $v);
                    $td->setAttribute("class", "label-font2");
                    $tr->appendChild($td);
                }

            }
            $td = $htmlDoc->createElement("td", "");
            $checkbox = $htmlDoc->createElement("input", "");
            $checkbox->setAttribute("type", "checkbox");
            $checkbox->setAttribute("name", "pRow");
            if (strcmp($status, "CANCELLED") == 0) {
                $checkbox->setAttribute("disabled", "");//for boolean attributes you can't use true or false, the moment you create the attribute, it's read as true.
                //so if you want the attribute to have a true value, simply create it, if you want a false value don't create the attribute
                //$checked = $htmlDoc->createAttribute("checked");
                //$checkbox->appendChild($checked);
            } elseif (strcmp($status, "SENT") == 0) {
                $checkbox->setAttribute("disabled", "");

                $checkbox->setAttribute("checked", "");

            } elseif (strcmp($status, "PREPARED") == 0) {
                $checkbox->setAttribute("checked", "");
            }
            $td->appendChild($checkbox);
            $tr->appendChild($td);
            $htmlDoc->getElementById("orderTable")->appendChild($tr);

        }
        $td = $htmlDoc->createElement("td", "");
        $tr = $htmlDoc->createElement("tr", "");
        $hid = $htmlDoc->createElement("input", "");
        $hid->setAttribute("type", "hidden");
        $hid->setAttribute("id", "orderID");
        $hid->setAttribute("value", $_POST["searchID"]);
        $td->appendChild($hid);
        $htmlDoc->getElementById("buttonsTable")->appendChild($td);
        $htmlDoc->getElementById("tableDiv")->setAttribute("style", "display:block");

        echo $htmlDoc->saveHTML();
    } else {
        echo $htmlDoc->saveHTML();
    }


}

?>