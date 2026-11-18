# Plataforma E-Commerce: Beer Shop

Aplicação web modular desenvolvida em **Python** com a *framework* **Django** para gestão de uma loja online de cervejas. O projeto segue a arquitetura **MVT (*Model-View-Template*)**, com separação clara de responsabilidades, gestão de utilizadores, processamento de encomendas e geração de documentos PDF.

## Funcionalidades Principais

* **Catálogo e Cesto de Compras:** Apresentação dinâmica de produtos com imagens dedicadas, suporte para promoções (`promotion_percentage`, `promotion_price`) e carrinho persistido em sessão (`cart.py`).
* **Gestão de Utilizadores:** Registo de clientes, autenticação (*login*/*logout*) e áreas de perfil e histórico de encomendas (`profile.html`, `register.html`, `order_detail.html`).
* **Processamento de Encomendas (*Checkout*):** Fluxo completo até à confirmação (`checkout.html`), com métodos de pagamento (MB WAY, Multibanco, Numerário), cálculo automático de portes e rastreio de estado (`delivery_status`).
* **Notificações e Faturação:** Geração de guias de remessa em PDF (`guia_remessa_pdf.html` via `xhtml2pdf`) e envio de *newsletters* promocionais (`promotion_newsletter.html`).
* **Painel Analítico (Admin):** Visualização de estatísticas de vendas mensais e produtos mais vendidos (`order_stats.html`).

## Arquitetura do Sistema (Apps Django)

* **`beer_shop`:** Aplicação central com configurações, roteamento global (`urls.py`) e lógica do carrinho (`cart.py`).
* **`accounts`:** Autenticação, formulários de registo (`forms.py`) e gestão de perfis (`Profile`).
* **`products`:** Modelos de catálogo (`Category`, `Product`), encomendas (`Order`, `OrderItem`), estatísticas (`OrderStats`) e geração de PDFs/emails.

## Tecnologias e Ferramentas

* **Linguagens:** Python, HTML5, CSS3
* **Framework:** Django 6 (ORM, Views, Templates, Admin)
* **Base de Dados:** SQLite (`db.sqlite3`)
* **Bibliotecas:** `xhtml2pdf`, `Pillow`, `ReportLab`

## Como Executar Localmente

> **Nota:** Para facilitar a avaliação em portefólio, o repositório inclui uma base de dados SQLite (`db.sqlite3`) pré-preenchida apenas com **dados de demonstração** e as imagens dos produtos em `media/beer_images/`.

1. Clonar o repositório e entrar na pasta do projeto:
   ```bash
   cd Web_Applications/beer_shop
   ```
2. Criar e ativar um ambiente virtual:
   ```bash
   python -m venv venv
   source venv/bin/activate      # Linux/macOS
   # ou
   venv\Scripts\activate         # Windows
   ```
3. Instalar as dependências:
   ```bash
   pip install -r requirements.txt
   ```
4. *(Opcional)* Copiar `.env.example` para `.env` e definir `SECRET_KEY` ou credenciais SMTP (se omitidas, os emails são mostrados no terminal):
   ```bash
   cp .env.example .env
   ```
5. Executar o servidor local:
   ```bash
   python manage.py runserver
   ```

### Contas de Demonstração (`db.sqlite3`)

| Perfil | Utilizador | Password |
| :--- | :--- | :--- |
| Administrador (Superuser) | `admin` | `demo1234` |
| Staff | `staff_demo` | `demo1234` |
| Cliente | `cliente_demo` | `demo1234` |

---
*Projeto desenvolvido para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*
