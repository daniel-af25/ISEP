<?php
session_start();

// Verifica se o utilizador está autenticado
if (!isset($_SESSION['username'])) {
    header("Location: index.html"); // Manda para o login 
    exit();
}

$nome = htmlspecialchars($_SESSION['username']);
?>

<!DOCTYPE html>
<html>

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Manage Orders</title>
    <link rel="stylesheet" href="styles/style.css">
</head>

<body class="general-body">

    <header class="site-header">
        <div class="logo">
            <img src="images/logo1.png" alt="Company Logo">
        </div>
        <div class="header-text-container">
            <h1>USER AREA</h1>
            <p>MANAGE ORDERS</p>
        </div>
    </header>
    <header class="User-header">
        <p><b>User:</b> <?= $nome ?>
            <button onclick="location.href='scripts/logout.php'">Logout</button>
        </p>
    </header>
    <div style="text-align:left;">
        <button onclick="window.location.href='uti1.php'" class="go-back-btn">Go Back</button>
    </div>
    <div class="search-container">
        <form action="scripts/orderInfo.php" method="POST" onsubmit="return checkInput(event)" class="search-form">
            <input type="text" id="searchID" name="searchID" class="search-input" placeholder="Order Number"
                required><button type="Submit" class="search-button">Search</button>
            <input type="hidden" name="page" value="../uti2Template.html">
        </form>
    </div>
    <div class="retangle below-center" id="infoDiv" style="display:none">
        <table border="1" id="infoTable" class="retangle">
            <caption class=" label-font center">Order Information</caption>
            <tr>
                <th>ORDER ID</th>
                <th>CLIENT'S NIF</th>
                <th>CLIENT'S NAME</th>
                <th>STATUS</th>
            </tr>
        </table>
    </div>
    <div class="retangle below-center" id="tableDiv" style="display:none">
        <table border="1" id="orderTable" class="retangle">
            <caption class=" label-font center">Products</caption>
            <tr>
                <th>PROD ID</th>
                <th>NAME</th>
                <th>DESCRIPTION</th>
                <th>QUANTITY</th>
                <th>ADDED TO ORDER</th>
            </tr>

        </table>
        <div class="center">
            <table id="buttonsTable">
                <tr>
                    <th></th>
                    <th></th>
                </tr>
                <tr>
                    <td><button type="button" onClick="jsSubmit(this)" id="prepped">Prepared</button></td>
                    <td><button type="button" id="sent" onClick="jsSubmit(this)">Sent</button></td>
                </tr>
                <tr>
                    <td colspan="2"><button type="button" id="cancel" class="center-td"
                            onClick="jsSubmit(this)">Cancelled</button></td>
                </tr>

            </table>
        </div>
    </div>
    <script>

        function checkInput(f) {
            const patternOrder = /^[A-F0-9]{6}$/;

            if (!patternOrder.test(document.getElementById("searchID").value)) {
                f.preventDefault();
                window.alert("Invalid order code");
                return false;
            } else {
                console.log("here");
                return true;
            }
        }

        let checkboxes = document.querySelectorAll("input[type=checkbox][name=pRow]");
        checkboxes.forEach(grayOnCheck);
        function grayOnCheck(c) {
            c.addEventListener("change", function () {
                if (this.checked) {
                    this.parentElement.parentElement.style.backgroundColor = "gray";
                    //this.disabled=true;
                    console.log("gray");
                } else {
                    this.parentElement.parentElement.style.backgroundColor = "";
                }
            });
        }


        function allChecked() {
            let checked = Array.from(checkboxes).filter(i => i.checked);
            if (checked.length != checkboxes.length) {
                return false;
            }
            return true;
        }

        function jsSubmit(e) {
            if (!allChecked() && e.id != "cancel") {
                window.alert("All rows must be checked before submitting");
            } else {
                const formData = new FormData();
                var searchID = document.getElementById("orderID").value;
                formData.append("condition", searchID);
                formData.append("status", e.textContent.toUpperCase());

                fetch("http://localhost/deapc/scripts/alterTable.php", {
                    // if instead of formData, I assign requestBody to body, it works!
                    body: formData,
                    method: "POST"
                }).then(function (response) {

                    return response.json();
                    //return response.formData();
                    //return response.text();
                }).then(function (data) {
                    //console.log(data);
                    if ("noResult" in Object.keys(data[0])) {
                        window.alert("Entry couldn't be updated");
                    } else {

                        document.getElementById("status").textContent = data[0]["stat"];
                        if (data[0]["stat"] == " SENT") {

                            checkboxes.forEach(function (e) {

                                e.disabled = true;
                                e.checked = true;
                            });
                        } else if (data[0]["stat"] == " CANCELLED") {
                            checkboxes.forEach(function (e) {
                                e.disabled = true;
                                e.checked = false;
                                e.parentElement.parentElement.style.backgroundColor = "";
                            });
                        }
                        if (data["cancelled"]) {
                            window.alert("order cancelled");
                        }
                    }

                }).catch(function (err) {
                    console.error(err);
                });
            }

        }
    </script>
</body>

</html>