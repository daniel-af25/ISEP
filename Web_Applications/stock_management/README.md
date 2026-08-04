#  Sistema de Gestão de Stock e Encomendas

Aplicação web desenvolvida em PHP nativo e JavaScript para a gestão integral de inventário, controlo de encomendas e administração de acessos. O projeto foi estruturado para suportar múltiplos níveis de privilégio de utilizador, separando as vistas de administração do painel de cliente/utilizador final.
A Web App foi desenvolvida no âmbito académico da licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) no ISEP, dentro da unidade curricular DEAPC (Desenvolvimento de Aplicações Computacionais).

##  Funcionalidades Principais
* **Controlo de Acessos e Autenticação:** Sistema de sessões seguro com rotinas independentes para *login* (`login.php`) e *logout* (`logout.php`).
* **Painéis de Administração:** Interfaces dedicadas para administradores (`admin1.php` a `admin5.php`), permitindo a gestão avançada da plataforma e modificação de registos.
* **Gestão de Encomendas e Faturação:** Ciclo completo de processamento de compras, desde a criação (`create_order.php`) e pesquisa (`search_order.php`), até à consulta detalhada do estado da transação (`orderInfo.php`). Inclui um sistema integrado para o **envio automático de emails com as faturas** associadas a cada cliente (`add_mail_users.php`).
* **Painel de Cliente/Utilizador:** Vistas e *templates* simplificados para a interação do utilizador regular com o catálogo e o seu perfil (`uti1.php`, `uti2.php`, `uti3.php`, `uti2Template.html`).
* **Manutenção de Base de Dados:** Módulos dinâmicos em PHP para alteração estrutural e manutenção das tabelas do sistema em tempo real (`alterTable.php`, `alterTableADM.php`, `query.php`).

##  Arquitetura do Sistema
O projeto utiliza uma arquitetura baseada em scripts *server-side* que interagem diretamente com o cliente:
* **Frontend:** Interface estruturada em HTML5 e estilizada com CSS3 (`styles/style.css`), recorrendo a JavaScript para interatividade dinâmica e manipulação do DOM (`DOMtest.php`).
* **Backend:** Lógica de negócio, roteamento e processamento de formulários implementados integralmente em PHP puro (centralizado na pasta `scripts/`).
* **Base de Dados:** Sistema suportado por uma arquitetura relacional em SQL, com o esquema estrutural preservado e pronto a importar (`database/deapc.sql`).

##  Tecnologias e Ferramentas
* **Linguagens:** PHP, JavaScript, HTML, CSS, SQL
* **Base de Dados:** MySQL / MariaDB (via importação de ficheiro `.sql`)
* **Design e UI:** Recursos visuais otimizados e integração de ícones sociais/branding na diretoria de imagens (`images/`)

##  Como Executar Localmente

1. Instalar um ambiente de desenvolvimento local como o **XAMPP**, **WAMP** ou **MAMP**.
2. Clonar este repositório e move a pasta do projeto para o diretório de execução do  servidor desejado (por exemplo, a pasta `htdocs` no XAMPP ou `www` no WAMP).
3. Iniciar os serviços **Apache** e **MySQL** no painel de controlo do XAMPP/WAMP.
4. Aceder ao phpMyAdmin (normalmente em `http://localhost/phpmyadmin`), cria uma nova base de dados vazia e importa o ficheiro estrutural `deapc.sql` localizado na pasta `database/`.
5. Atualizar as credenciais de acesso à base de dados nos scripts PHP (se necessário, de acordo com o teu utilizador/password local do MySQL).
6. Aceder à aplicação através do browser no endereço correspondente (ex: `http://localhost/stock_management/Index.html`).

---
*Projeto desenvolvido para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*
