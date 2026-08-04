# Plataforma E-Commerce: Beer Shop

Esta é uma aplicação web modular desenvolvida em Python com recurso à *framework* Django para gestão de uma loja online. O projeto implementa uma arquitetura MVT (*Model-View-Template*) robusta, com separação clara de responsabilidades, gestão de base de dados e um catálogo de produtos interativo.

## Funcionalidades Principais
* **Catálogo e Cesto de Compras:** Apresentação dinâmica de produtos com imagens dedicadas, suporte para descontos (`promotion_percentage`, `promotion_price`) e um sistema completo de carrinho de compras (`cart.py`).
* **Gestão de Utilizadores:** Sistema seguro de registo de clientes, autenticação (*login*) e áreas de perfil personalizadas (`profile.html`, `register.html`).
* **Processamento de Encomendas (*Checkout*):** Ciclo completo de vendas, desde o cesto até à confirmação (`checkout.html`). Inclui rastreio do estado da entrega (`delivery_status`), métodos de pagamento (`payment_method`) e cálculo de custos de envio (`shipping_price`).
* **Notificações e Faturação:** Módulo integrado para geração de guias de remessa em formato PDF (`guia_remessa_pdf.html`) e envio de *newsletters* promocionais para clientes (`promotion_newsletter.html`).
* **Painel Analítico:** Área para visualização de estatísticas sobre as encomendas e produtividade da loja (`order_stats.html`).

##  Arquitetura do Sistema (Apps Django)
O projeto está estruturado em módulos independentes para garantir a escalabilidade e a manutenção do código:
* **`beer_shop`:** Aplicação central (Core) que gere as configurações principais, roteamento geral e a lógica de sessão do carrinho de compras.
* **`accounts`:** Responsável por toda a lógica de autenticação, segurança, formulários de registo e perfis de utilizador.
* **`products`:** Gere os modelos de dados cruciais da plataforma: inventário de produtos, estruturação de encomendas (`Order`, `OrderItem`) e histórico de vendas.

##  Tecnologias e Ferramentas
* **Linguagens:** Python, HTML5, CSS3
* **Framework:** Django (Routing, ORM, Views, Templates)
* **Base de Dados:** SQLite (`db.sqlite3`)
* **Outros:** Gestão de ficheiros estáticos/media e geração dinâmica de PDFs

##  Como Executar Localmente
*Nota:* Para facilitar a avaliação deste projeto em ambiente de portefólio, a base de dados (`db.sqlite3`) pré-preenchida e a pasta de imagens (`media/beer_images`) foram deixadas de prepósito no repositório. Desta forma, é possível testar a aplicação de imediato sem necessidade de correr migrações ou preencher a basa de dados manualmente...


1. Clonar o repositório para o computador desejado.
2. Criar e ativar um ambiente virtual (recomendado):
   ```bash
   python -m venv venv
   source venv/bin/activate  # Em Linux/macOS
   # ou
   venv\Scripts\activate     # Em Windows
3. Instalar as dependências necessárias:
    pip install -r requirements.txt
4. Executar o servidor:
    python manage.py runserver


---
*Projeto desenvolvido para a Licenciatura em Engenharia Eletrotécnica e de Computadores (LEEC) do ISEP.*