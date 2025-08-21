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
<html>

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Manage Orders</title>
    <link rel="stylesheet" href="/DEAPC/styles/style.css">
</head>

<body class="general-body">
    <?php if (isset($_GET['order_success']) && $_GET['order_success'] == 1 && isset($_GET['order_id'])): ?>
        <script>
            alert("Order created successfully! Order ID: <?php echo htmlspecialchars($_GET['order_id']); ?>");
        </script>
    <?php endif; ?>
    <header class="site-header">
        <div class="logo">
            <img src="images/logo1.png" alt="Company Logo">
        </div>
        <div class="header-text-container">
            <h1>ADMIN AREA</h1>
            <p>PREPARE / CREATE ORDER</p>
        </div>
    </header>
    <header class="User-header">
        <p><b>User:</b> <?= $nome ?>
            <button onclick="location.href='scripts/logout.php'">Logout</button>
        </p>
    </header>
    <div style="text-align:left;">
        <button onclick="window.location.href='/DEAPC/admin1.php'" class="go-back-btn">Go Back</button>
    </div>
    <!-- Radio Buttons -->
    <div class="order-radio-group">
        <label>
            <input type="radio" name="order_action" id="Search" value="search" checked>
            Search Order
        </label>
        <label>
            <input type="radio" name="order_action" id="Create" value="create">
            Create Order
        </label>
    </div>

    <!-- CREATE ORDER SECTION -->
    <div id="create_order_section">
        <div class="retangle below-center">
            <form id="createOrderForm" action="scripts/create_order.php" method="POST">
                <table border="1" id="orderProductsTable">
                    <caption><strong>Create New Order</strong></caption>
                    <tr>
                        <th>CODE</th>
                        <th>QUANTITY</th>
                    </tr>
                    <tr>
                        <td>
                            <input list="product_codes" name="code[]" id="product_code" required>
                            <datalist id="product_codes"></datalist>
                            <script>
                                fetch('scripts/datalists_orders.php?type=products')
                                    .then(response => response.text())
                                    .then(data => {
                                        document.getElementById('product_codes').innerHTML = data;
                                    });
                            </script>
                        </td>
                        <td><input type="number" name="quantity[]" min="1" required></td>
                        <td>
                            <button type="button" onclick="this.closest('tr').remove()"
                                aria-label="Remove row">🗑️</button>
                        </td>
                    </tr>
                </table>
                <div class="center" style="margin-top: 10px;">
                    <button type="button" onclick="addProductRow()">Add Product</button>
                </div>
                <div class="center" style="margin-top: 10px;">
                    <label for="clientNIF"><strong>Client NIF:</strong></label>
                    <input list="client_nifs" id="clientNIF" name="clientNIF" class="wide-input" required
                        pattern="\d{9}" title="NIF must be exactly 9 digits" required>
                    <datalist id="client_nifs"></datalist>
                    <script>
                        fetch('scripts/datalists_orders.php?type=clients')
                            .then(response => response.text())
                            .then(data => {
                                document.getElementById('client_nifs').innerHTML = data;
                            });
                    </script>
                </div>
                <div class="center" style="margin-top: 10px;">
                    <button type="submit" id="createOrder">Create Order</button>
                </div>
            </form>
        </div>
    </div>

    <!-- SEARCH ORDER SECTION -->
    <div id="search_order_section" style="display:none">
        <div class="retangle below-center search-retangle">
            <form action="scripts/search_order.php" method="POST">
                <label class="label-font" for="searchID"><strong>Order Number</strong></label>
                <input list="order_numbers" type="text" name="searchID" id="searchID" required pattern="[A-Za-z0-9]{6}"
                    title="Orders are alphanumeric with 6 digits" required>
                <datalist id="order_numbers"></datalist>
                <script>
                    fetch('scripts/datalists_orders.php?type=orders')
                        .then(response => response.text())
                        .then(data => {
                            document.getElementById('order_numbers').innerHTML = data;
                        });
                </script>
                <input type="hidden" name="page" value="admin4.php">
                <button type="submit">Search</button>
            </form>
        </div>
        <div class="retangle below-center" id="infoDiv" style="display:none">
            <table border="1" id="infoTable" class="retangle">
                <caption class="label-font center">Order Information</caption>
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
                <caption class="label-font center">Products</caption>
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
                    <!-- Buttons will be added dynamically by PHP/DOMDocument -->
                </table>
            </div>
        </div>
    </div>

    <script>
        const createOrderSection = document.getElementById('create_order_section');
        const searchOrderSection = document.getElementById('search_order_section');
        const createRadio = document.getElementById('Create');
        const searchRadio = document.getElementById('Search');

        function toggleSections() {
            if (createRadio.checked) {
                createOrderSection.style.display = 'block';
                searchOrderSection.style.display = 'none';
            } else {
                createOrderSection.style.display = 'none';
                searchOrderSection.style.display = 'block';
            }
        }

        createRadio.addEventListener('change', toggleSections);
        searchRadio.addEventListener('change', toggleSections);
        toggleSections();

        function addProductRow() {
            const table = document.getElementById('orderProductsTable');
            if (!table) return;

            const row = table.insertRow(-1);

            // Product code cell
            const codeCell = row.insertCell(0);
            codeCell.innerHTML = `<input list="product_codes" name="code[]" required aria-label="Product Code">`;

            // Quantity cell
            const qtyCell = row.insertCell(1);
            qtyCell.innerHTML = `<input type="number" name="quantity[]" min="1" required aria-label="Quantity">`;

            // Remove button cell
            const removeCell = row.insertCell(2);
            removeCell.innerHTML = `<button type="button" onclick="this.closest('tr').remove()" aria-label="Remove row">🗑️</button>`;
        }
    </script>
    <script>
        // Event delegation for dynamically generated checkboxes
        document.addEventListener('change', function (e) {
            if (e.target.matches('input[type=checkbox][name=pRow]')) {
                if (e.target.checked) {
                    e.target.parentElement.parentElement.style.backgroundColor = "gray";
                } else {
                    e.target.parentElement.parentElement.style.backgroundColor = "";
                }
            }
        });

        function allChecked() {
            let checkboxes = document.querySelectorAll("input[type=checkbox][name=pRow]");
            let checked = Array.from(checkboxes).filter(i => i.checked);
            return checked.length === checkboxes.length;
        }

        function jsSubmit(e) {
            let checkboxes = document.querySelectorAll("input[type=checkbox][name=pRow]");
            if (!allChecked() && e.id !== "cancelled") {
                window.alert("All rows must be checked before submitting");
            } else {
                const formData = new FormData();
                var searchID = document.getElementById("orderID").value;
                formData.append("condition", searchID);
                formData.append("status", e.textContent.toUpperCase());

                fetch("/DEAPC/scripts/alterTableADM.php", {
                    body: formData,
                    method: "POST"
                }).then(function (response) {
                    return response.json();
                }).then(function (data) {
                    if ("noResult" in data[0]) {
                        // Show the error message if present, otherwise show a generic message
                        window.alert(data[0].error || "Entry couldn't be updated");
                    } else {
                        document.getElementById("status").textContent = data[0]["stat"];
                        if (data[0]["stat"].trim() === "SENT") {
                            checkboxes.forEach(function (e) {
                                e.disabled = true;
                                e.checked = true;
                            });
                        } else if (data[0]["stat"].trim() === "CANCELLED") {
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