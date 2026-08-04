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
  <title>New Client/Product</title>
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
      <p>NEW CLIENT/PRODUCT</p>
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
  <div class="fieldset-wrapper">
    <fieldset class="select_product_client">
      <div class="radio-group">
        <label class="select_product">
          <input type="radio" id="Produto" name="produto_cliente" value="Produto" checked />
          Product
        </label>
        <label class="select_client">
          <input type="radio" id="Cliente" name="produto_cliente" value="Cliente" />
          Client
        </label>
      </div>
    </fieldset>
  </div>
  <div class="forms-container">
    <form class="styled-form" id="add_product_form" action="scripts/add_product_client.php" method="POST">
      <h2>Add Product</h2>
      <input type="text" name="product_name" placeholder="Product Name" required><br>
      <input type="text" name="supplier" placeholder="Suppliers" required><br>
      <input type="number" name="price" placeholder="Price" required><br>
      <div class="payment-method-group category-group">
        <div id="category_display" class="payment-method-display" tabindex="0">Choose Category</div>
        <input type="hidden" name="category" id="category" required>
        <div id="categoryList" class="methods-list" style="display:none;">
          <div class="method-option category-option">Electronics</div>
          <div class="method-option category-option">Office Supplies</div>
          <div class="method-option category-option">Furniture</div>
          <div class="method-option category-option">Software</div>
          <div class="method-option category-option">Other</div>
        </div>
      </div>
      <input type="text" name="notes" placeholder="NOTES:"><br>
      <input type="date" name="date" placeholder="Date:" required><br>
      <button type="submit">Save Product</button>
      <button type="reset" class="clear-btn">Clear</button>
    </form>
    <form class="styled-form" id="add_client_form" action="scripts/add_product_client.php" method="POST">
      <h2>Add Client</h2>
      <input type="text" name="client_name" placeholder="Client Name" required><br>
      <input type="email" name="email" placeholder="Email" required><br>
      <input type="text" name="address" placeholder="Adress" required><br>
      <input type="text" name="nif" placeholder="NIF" required pattern="\d{9}" title="NIF must be exactly 9 digits"><br>
      <div class="payment-method-group">
        <div id="payment_method_display" class="payment-method-display" tabindex="0">Choose Payment Method</div>
        <input type="hidden" name="payment_method" id="payment_method" required>
        <div id="methodsList" class="methods-list" style="display:none;">
          <div class="method-option">Prompt Payment</div>
          <div class="method-option">30 Days</div>
          <div class="method-option">Credit 180 days</div>
        </div>
      </div>
      <input type="tel" name="phone" placeholder="Phone" pattern="\d{9}" title="Phone must be 9 digits"><br>
      <input type="date" name="date" placeholder="Date:" required><br>
      <button type="submit">Save Client</button>
      <button type="reset" class="clear-btn">Clear</button>
    </form>
  </div>
  <script>
    const productForm = document.getElementById('add_product_form');
    const clientForm = document.getElementById('add_client_form');
    const productRadio = document.getElementById('Produto');
    const clientRadio = document.getElementById('Cliente');

    function toggleForms() {
      if (productRadio.checked) {
        productForm.style.display = 'block';
        clientForm.style.display = 'none';
      } else {
        productForm.style.display = 'none';
        clientForm.style.display = 'block';
      }
    }
    toggleForms();
    productRadio.addEventListener('change', toggleForms);
    clientRadio.addEventListener('change', toggleForms);
  </script>
  <script>
    window.addEventListener('DOMContentLoaded', function () {
      const params = new URLSearchParams(window.location.search);
      if (params.get('success') === 'product') {
        alert('Product added successfully!');
        params.delete('success');
        history.replaceState(null, '', window.location.pathname + '?' + params.toString());
      } else if (params.get('success') === 'client') {
        alert('Client added successfully!');
        params.delete('success');
        history.replaceState(null, '', window.location.pathname + '?' + params.toString());
      }
    });
  </script>
  <script>
    const display = document.getElementById('payment_method_display');
    const list = document.getElementById('methodsList');
    const input = document.getElementById('payment_method');

    display.onclick = function () {
      list.style.display = list.style.display === 'block' ? 'none' : 'block';
    };

    document.querySelectorAll('.method-option').forEach(function (option) {
      option.onclick = function () {
        display.textContent = this.textContent;
        input.value = this.textContent;
        list.style.display = 'none';
      };
    });

    // Hide dropdown if clicked outside
    document.addEventListener('click', function (e) {
      if (!e.target.closest('.payment-method-group')) {
        list.style.display = 'none';
      }
    });
  </script>
  <script>
    const categoryDisplay = document.getElementById('category_display');
    const categoryList = document.getElementById('categoryList');
    const categoryInput = document.getElementById('category');

    categoryDisplay.onclick = function () {
      categoryList.style.display = categoryList.style.display === 'block' ? 'none' : 'block';
    };

    // Only select options inside the categoryList
    document.querySelectorAll('#categoryList .category-option').forEach(function (option) {
      option.onclick = function () {
        categoryDisplay.textContent = this.textContent;
        categoryInput.value = this.textContent;
        categoryList.style.display = 'none';
      };
    });

    // Hide dropdown if clicked outside
    document.addEventListener('click', function (e) {
      if (!e.target.closest('.category-group')) {
        categoryList.style.display = 'none';
      }
    });
  </script>
</body>

</html>