<?php
if (session_status() === PHP_SESSION_NONE) {
    session_start();
}
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
    <title>Check Product/Client</title>
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
            <p>STOCK/CLIENT MANAGEMENT</p>
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
    <div class="order-radio-group">
        <label class="select_product">
            <input type="radio" id="Produto" name="produto_cliente" value="Produto" checked />
            Product
        </label>
        <label class="select_client">
            <input type="radio" id="Cliente" name="produto_cliente" value="Cliente" />
            Client
        </label>
    </div>
    <div class="search-container">
        <form method="POST" action="scripts/check_product_client.php" class="search-form">
            <input type="text" id="nif_product" name="nif_product" class="search-input"
                placeholder="Product ID/Client NIF" required list="productList"
                value="<?php echo htmlspecialchars($_POST['nif_product'] ?? ''); ?>" />
            <datalist id="productList">
            </datalist>
            <datalist id="clientList">
            </datalist>
            <input type="hidden" id="tipo" name="tipo" value="Produto">
            <button type="submit" class="search-button">&#128269; Search</button>
        </form>
    </div>

    <?php
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['nif_product'])) {
        include 'scripts/check_product_client.php';
    }
    ?>
    <?php if (isset($_GET['msg']) && $_GET['msg'] === 'updated'): ?>
        <div class="success-message">Updated successfully.</div>
    <?php elseif (isset($_GET['msg']) && $_GET['msg'] === 'deleted'): ?>
        <div class="success-message">Deleted successfully.</div>
    <?php endif; ?>
</body>

</html>

<script>
    // Update placeholder based on radio selection
    document.querySelectorAll('input[name="produto_cliente"]').forEach(radio => {
        radio.addEventListener('change', function () {
            const input = document.getElementById('nif_product');
            if (this.value === 'Produto') {
                input.setAttribute('list', 'productList');
                input.placeholder = 'Enter Product ID';
            } else {
                input.setAttribute('list', 'clientList');
                input.placeholder = 'Enter Client NIF';
            }
            document.getElementById('tipo').value = this.value;
        });
    });
    window.addEventListener('DOMContentLoaded', function () {
        const checked = document.querySelector('input[name="produto_cliente"]:checked');
        const input = document.getElementById('nif_product');
        if (checked.value === 'Produto') {
            input.setAttribute('list', 'productList');
            input.placeholder = 'Enter Product ID';
        } else {
            input.setAttribute('list', 'clientList');
            input.placeholder = 'Enter Client NIF';
        }
    });
    // Fetch form based on selected type
    function fetchForm() {
        const value = document.getElementById('nif_product').value.trim();
        if (!value) {
            alert('Please enter a Product ID or Client NIF.');
            return;
        }
        const formData = new FormData();
        formData.append('nif_product', value);
        formData.append('tipo', type)

        fetch('scripts/check_product_client.php', {
            method: 'POST',
            body: formData
        })
            .then(response => response.text())
            .then(html => {
                document.getElementById('dynamic-form-area').innerHTML = html;
            })
            .catch(err => {
                document.getElementById('dynamic-form-area').innerHTML = "<p>Error loading data.</p>";
                console.error(err);
            });
    }
</script>
<script>
    function updatePlaceholder() {
        const selected = document.querySelector('input[name="produto_cliente"]:checked').value;
        document.getElementById('nif_product').placeholder =
            selected === 'Produto' ? 'Enter Product ID' : 'Enter Client NIF';
    }

    // Set placeholder on page load
    updatePlaceholder();

    // Update placeholder when radio changes
    document.querySelectorAll('input[name="produto_cliente"]').forEach(radio => {
        radio.addEventListener('change', updatePlaceholder);
    });
</script>
<script>
    function loadDatalist(type) {
        const datalistId = type === 'Produto' ? 'productList' : 'clientList';
        fetch('scripts/datalists_management.php?type=' + (type === 'Produto' ? 'products' : 'clients'))
            .then(response => response.text())
            .then(data => {
                document.getElementById(datalistId).innerHTML = data;
                document.getElementById('nif_product').setAttribute('list', datalistId);
            });
    }

    function updateFormUI(type) {
        const input = document.getElementById('nif_product');
        input.placeholder = type === 'Produto' ? 'Enter Product ID' : 'Enter Client NIF';
        document.getElementById('tipo').value = type;
        loadDatalist(type);
    }

    // On page load, set up everything
    window.addEventListener('DOMContentLoaded', function () {
        const checked = document.querySelector('input[name="produto_cliente"]:checked');
        updateFormUI(checked.value);
    });

    // Switch datalist, placeholder, and hidden field when radio changes
    document.querySelectorAll('input[name="produto_cliente"]').forEach(radio => {
        radio.addEventListener('change', function () {
            updateFormUI(this.value);
        });
    });
</script>