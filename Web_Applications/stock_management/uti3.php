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
    <title> Look up Stock/Customer</title>
    <link rel="stylesheet" href="styles/style.css">
</head>

<body class="general-body">
    <header class="site-header">
        <div class="logo">
            <img src="images/logo1.png" alt="Company Logo">
        </div>
        <div class="header-text-container">
            <h1>USER AREA</h1>
            <p>CLIENT AND PRODUCT INFO</p>
        </div>
    </header>
    <div class="top-left">
        <header class="User-header">
            <p><b>User:</b> <?= $nome ?>
                <button onclick="location.href='scripts/logout.php'">Logout</button>
            </p>
        </header>
    </div>
    <div style="text-align:left;">
        <button onclick="window.location.href='uti1.php'" class="go-back-btn">Go Back</button>
    </div>
    <div class="search-container">
        <p class="search-form"><label class="label-font"></label>
            <select id="searchFor" class="search-button" onChange="changePH(this)">
                <option value="clients">Client</option>
                <option value="products">Product</option>
            </select>
            <input type="text" id="searchID" name="searchID" class="search-input" placeholder="Client NIF"><button
                type="button" onClick="jsSubmit()" class="search-button">Search</button>
        </p>
        <input type="hidden" id="selected" name="url" value="uti3.php">

    </div>

    <div id="tableDiv" class="retangle center" style="display:none">

    </div>
</body>
<script>
    function changePH(e) {
        //kep in mind that e.textContent is a string that has several whitespaces, before, after words and also \n, it appears to be formatted like the options tags
        if (e.value == "products") {
            document.getElementById("searchID").placeholder = e.textContent.substring(e.textContent.indexOf("Product")).trim() + " Code";
            document.getElementById("searchID").value = "";
        } else if (e.value == "clients") {
            document.getElementById("searchID").placeholder = e.textContent.substring(e.textContent.indexOf("Client"), e.textContent.indexOf("Product")).trim() + " NIF";
            document.getElementById("searchID").value = "";
        }

    }

    function jsSubmit() {

        const patternNIF = /\d{9}/;

        const patternProd = /\d{1,6}/;

        const formData = new FormData();
        let searchID = document.getElementById("searchID").value;
        let selector = document.getElementById("searchFor");
        let selected = selector.options[selector.selectedIndex].value;

        if ((selected == "clients" && patternNIF.test(searchID)) || (selected == "products" && patternProd.test(searchID))) {


            formData.append("condition", searchID);
            formData.append("table", selected);
            formData.append("url", document.getElementById("selected").value);


            fetch("http://localhost/deapc/scripts/query.php", {
                // if instead of formData, I assign requestBody to body, it works!
                body: formData,
                method: "POST"
            }).then(function (response) {
                return response.json();
                //return response.formData();
                //return response.text();
            }).then(function (data) {
                //console.log(data);


                let table = document.createElement("table");
                table.setAttribute("border", "1");
                let header = table.createTHead();
                let h = header.insertRow(0);
                let body = table.createTBody();
                let cells = 0;

                for (let id of Object.keys(data[0])) {
                    if (id != "table") {
                        c = h.insertCell(cells);
                        c.textContent = id.toUpperCase();
                        cells++;
                    }
                }
                for (let row of Object.keys(data)) {
                    let r = body.insertRow(row);
                    cells = 0;
                    for (let id of Object.keys(data[row])) {
                        //console.log(id);
                        if (id != "table") {
                            c = r.insertCell(cells);
                            c.id = id + row;
                            c.textContent = data[row][id];
                            cells++;
                        }

                    }




                }


                document.getElementById("tableDiv").innerHTML = "";
                document.getElementById("tableDiv").appendChild(table);
                document.getElementById("tableDiv").style.display = "block";
            }).catch(function (err) {
                console.error(err);
            });
        } else if (selected == "clients" && !patternNIF.test(searchID)) {
            window.alert("Input is not a valid NIF");
        } else if (selected == "products" && !patternProd.test(searchID)) {
            window.alert("Input is not a valid product code");
        }
    }


</script>

</html>