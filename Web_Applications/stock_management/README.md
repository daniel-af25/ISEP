# Sistema de Gestão de Stock e Encomendas

Aplicação web desenvolvida em **PHP** nativo e **JavaScript** para gestão de inventário, clientes, encomendas e controlo de acessos por níveis de privilégio (Administração vs. Utilizador). Projeto académico desenvolvido na unidade curricular **DEAPC (Desenvolvimento de Aplicações Computacionais)** da LEEC no ISEP.

## Funcionalidades Principais

* **Controlo de Acessos e Autenticação:** Sistema de sessões com *login* (`scripts/login.php`) e *logout* (`scripts/logout.php`) e separação entre área administrativa e área de utilizador.
* **Painéis de Administração (`admin1.php`–`admin5.php`):** Gestão de produtos, clientes, criação/pesquisa/atualização de encomendas e criação de utilizadores com notificação por email.
* **Gestão de Encomendas:** Criação (`create_order.php`), pesquisa (`search_order.php`), consulta detalhada (`orderInfo.php`) e atualização de estados com reposição de stock.
* **Painel de Utilizador (`uti1.php`–`uti3.php`):** Consulta de catálogo, clientes e acompanhamento de encomendas.

## Arquitetura do Sistema

* **Frontend:** HTML5, CSS3 (`styles/style.css`) e JavaScript para interatividade dinâmica.
* **Backend:** Scripts PHP centralizados em `scripts/`, com configuração isolada em [`scripts/db_config.php`](./scripts/db_config.php).
* **Base de Dados:** Esquema MySQL/MariaDB com dados de demonstração pronto a importar em [`database/deapc.sql`](./database/deapc.sql).

## Tecnologias e Ferramentas

* **Linguagens:** PHP, JavaScript, HTML5, CSS3, SQL
* **Base de Dados:** MySQL / MariaDB

## Como Executar Localmente

1. Instalar um ambiente local como **XAMPP**, **WAMP** ou **MAMP**.
2. Copiar a pasta `stock_management` para o diretório raiz do servidor (ex.: `htdocs/DEAPC` no XAMPP).
3. Iniciar os serviços **Apache** e **MySQL**.
4. Aceder ao **phpMyAdmin** (`http://localhost/phpmyadmin`), criar uma base de dados chamada `deapc` e importar [`database/deapc.sql`](./database/deapc.sql).
5. *(Opcional)* Copiar [`scripts/db_config.example.php`](./scripts/db_config.example.php) para `scripts/db_config.local.php` e ajustar o utilizador/password do MySQL (por omissão usa `localhost`, utilizador `root`, password vazia ou variáveis de ambiente `DB_HOST`, `DB_USER`, `DB_PASS`, `DB_NAME`).
6. Abrir `http://localhost/DEAPC/Index.html` no browser.

### Contas de Demonstração (`deapc.sql`)

| Perfil | Utilizador | Password |
| :--- | :--- | :--- |
| Administrador | `admin` | `demo1234` |
| Staff Admin | `staff_demo` | `demo1234` |
| Utilizador | `user_demo` | `demo1234` |

---
*Projeto desenvolvido para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*
