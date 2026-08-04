import io
from django.contrib.auth.models import User
from django.core.mail import send_mail
from django.conf import settings
from django.db import models
from django.core.exceptions import ValidationError
from django.template.loader import render_to_string
from django.core.validators import MinValueValidator, MaxValueValidator
from django.core.mail import EmailMultiAlternatives 
from django.utils.html import strip_tags
from PIL import Image
from xhtml2pdf import pisa
from decimal import Decimal #para calculos de subtotal sem arrendondamentos estupidos

class Category(models.Model):
    """
    Classe associada á Categoria do produto:
        -> Beer
        -> Pack
        -> ...
    """
    name = models.CharField(max_length=100)
    
    def __str__(self):
        return self.name

class Product(models.Model):
    """
    Classe associada ao produto cerveja
    """
    name = models.CharField(max_length=200)
    description = models.TextField()
    price = models.DecimalField(max_digits=10, decimal_places=2)
    promotion_percentage = models.PositiveIntegerField(
        default=0, 
        validators=[MinValueValidator(0), MaxValueValidator(50)],
        help_text="Percentagem de desconto entre 0 e 50%"
    )
    promotion_price = models.DecimalField(
        max_digits=10, 
        decimal_places=2, 
        default=0.00,
        editable=False 
    )
    stock = models.PositiveIntegerField(default=0)
    abv = models.DecimalField(max_digits=3, decimal_places=1, verbose_name="ABV %")
    category = models.ForeignKey(Category, on_delete=models.SET_NULL, null=True)
    image = models.ImageField(upload_to='beer_images/', blank=True, null=True)

    
    def save(self, *args, **kwargs):
        """
        Código que é rodado quando existe uma atualização em algum dado da DB Produtos, quer seja nova adição
        alteração de um produto já existente...
        O que faz resumidamente: Lógica de emails de promoção, e compressão de imagens.
        """
        # Verifica se a promoção em questão é nova.
        enviar_notificacao = False
        if self.pk:
            old_instance = Product.objects.get(pk=self.pk)
            # Se houve alteração na promoção prossegue com a logica de envio
            if old_instance.promotion_percentage != self.promotion_percentage and self.promotion_percentage > 0:
                enviar_notificacao = True
        elif self.promotion_percentage > 0:
            #Para os casos que o produto criado seja inicializado com promoção
            enviar_notificacao = True

        # 2. Lógica de cálculo do promotion_price (já existente)
        if self.promotion_percentage > 0:
            discount = (Decimal(self.promotion_percentage) / Decimal('100')) * self.price
            self.promotion_price = (self.price - discount).quantize(Decimal('0.01'))
        else:
            self.promotion_price = 0.00
        # Guarda as alterações na DB
        super().save(*args, **kwargs) 

        #Se a promoção foi ativada chama o método que envia os emails para os clientes
        if enviar_notificacao:
            self.notificar_clientes_promocao()
        #Compressão de imagem do produto
        if self.image:
            img = Image.open(self.image.path)
            output_size = (600, 600)
            if img.height > 600 or img.width > 600:
                img.thumbnail(output_size)
                img.save(self.image.path, quality=85, optimize=True)
    
    def notificar_clientes_promocao(self):
        """
        Método de envio de email com as promoções.
        """
        #Fetch da DB de todos os emails presentes em users
        emails = User.objects.filter(is_active=True).exclude(email='').values_list('email', flat=True)
        
        #Caso a lista com os emails esteja vazia retornamos da função
        if not emails:
            return

        assunto = f'Fans Fans Beers - {self.promotion_percentage}% Desconto em {self.name}!'
        base_url = getattr(settings, 'SITE_URL', 'http://127.0.0.1:8000')
        # Prepara o contexto para o template HTML
        context = {
            'product_name': self.name,
            'old_price': self.price,
            'new_price': self.promotion_price,
            'promo_percent': self.promotion_percentage,
            'description': self.description,
            'stock': self.stock,
            'image_url': f"{base_url}{self.image.url}" if self.image else None,
            'product_url': f"{base_url}/catalog/{self.id}/"
        }

        # Pega no html do newsletter e renderiza para o corpo do email
        html_content = render_to_string('email/promotion_newsletter.html', context)
        # Cria uma versão em texto simples para segurança
        text_content = strip_tags(html_content)

        # EmailMultiAlternatives permite enviar para uma lista de destinatários (BCC é recomendado para privacidade)
        msg = EmailMultiAlternatives(
            subject=assunto,
            body=text_content,
            from_email=settings.EMAIL_HOST_USER,
            to=[settings.EMAIL_HOST_USER],
            bcc=list(emails) # Email será enviado para todos os clientes, email enviado para os clientes em BCC..
        )
        msg.attach_alternative(html_content, "text/html")
        
        try:
            msg.send()
        except:
            print(f"Erro ao enviar e-mail para toda a base de dados de emails.")

    def __str__(self):
        return self.name
    
    
    
class Order(models.Model):
    '''
    Classe associada às encomendas
    '''
    DELIVERY_CHOICES = [
        ('pendente', 'Pendente de Envio'),
        ('enviada', 'Enviada'),
        ('entregue', 'Entregue'),
        ('cancelada', 'Cancelada'),
        
    ]
    
    user = models.ForeignKey(User, on_delete=models.CASCADE, related_name='orders', null=True, blank=True)
    full_name = models.CharField(max_length=250)
    email = models.EmailField(max_length=250)
    address1 = models.CharField(max_length=250)
    address2 = models.CharField(max_length=250, blank=True)
    city = models.CharField(max_length=100)
    zip_code = models.CharField(max_length=20)
    total_paid = models.DecimalField(max_digits=10, decimal_places=2)
    shipping_price = models.DecimalField(max_digits=10, decimal_places=2, default=0.00)
    created = models.DateTimeField(auto_now_add=True)
    delivery_status = models.CharField(
        max_length=20,
        choices=DELIVERY_CHOICES,
        default='pendente'
    )
    billing_status = models.BooleanField(default=False) # Se já foi pago
    managed_by = models.ForeignKey(User,on_delete=models.SET_NULL,null=True,blank=True,related_name='managed_orders',editable=False ) #guarda quem fechou a encomenda
    payment_method = models.CharField(max_length=20, default='multibanco') # método de pagamento
    

    class Meta:
        '''
        Metadata do django
        '''
        ordering = ('-created',) # garante que as encomendas mais recentes estejam sempre em primeiro nas listas

    def gerar_pdf_guia(self):
        """
        Função para renderizar o html em pdf
        """
        html = render_to_string('email/guia_remessa_pdf.html', {'order': self})
        result = io.BytesIO()
        pisa.pisaDocument(
            io.BytesIO(html.encode("utf-8")), 
            result, 
            encoding='utf-8' # Força o processamento em UTF-8
        )   
        return result.getvalue()

    def save(self, *args, **kwargs):    
        """
        funçao para enviar a guia de remessa quando colocamos a order como "enviada" pela primeira vez
        """
        # verifica se tem id do pedido na db
        if self.pk:
            old_order = Order.objects.get(pk=self.pk)
            # garante que o enviada foi colocado na hora de forma a evitar o envio de vários mails
            if old_order.delivery_status != 'enviada' and self.delivery_status == 'enviada':
                self.enviar_email_guiaremessa()
        super().save(*args, **kwargs)

    def enviar_email_guiaremessa(self):
        #funçao para enviar o email com a guia deremessa
        assunto = f'Encomenda FansFans [{self.id}]'
        #raw message
        mensagem = (
            f'Olá {self.full_name},\n\n'
            f'Temos o prazer de informar que a sua encomenda #{self.id} foi processada com sucesso '
            f'e respetivamente enviada!\n\n'
            f'Detalhes da Entrega:\n'
            f'Morada: {self.address1}, {self.city}\n\n'
            f'Enviamos em anexo a respetiva guia de remessa associada a esta encomenda.\n\n'
            f'Agradecemos a sua preferência pela FansFans Beers! Esperamos que aprecie a sua seleção.\n\n'
            f'Com os melhores cumprimentos,\n'
            f'A Equipa FansFans\n\n'
            f'------------------------------------------------------------\n'
            f'Beba com moderação. \n'
            f'Este é um e-mail gerado automaticamente, por favor não responda diretamente.'
        )

        from django.core.mail import EmailMessage # classe para o email poder cotner anexos
        
        email = EmailMessage(
            assunto,
            mensagem,
            settings.EMAIL_HOST_USER,
            [self.email],
        )
        
        pdf = self.gerar_pdf_guia()
        if pdf:
            email.attach(f'Guia_Remessa_{self.id}.pdf', pdf, 'application/pdf')
        try:
            email.send()
        except:
            print(f"Erro ao enviar email.")

    def validar_pagamento_enviada(self):
        """
        Garantir que apenas é possível colocar como enviada se o billing status já estiver True,
        se o metodo for dinheiro é possível colocar a mesma como enviada.
        """
        method = self.payment_method.lower()
        if self.delivery_status == 'enviada' and not self.billing_status and method != 'dinheiro':
            raise ValidationError({
                'delivery_status': 'ERRO: Não é possível enviar uma encomenda que não foi paga.'
            })

    def clean(self):
        self.validar_pagamento_enviada()

    def __str__(self):
        return f'Encomenda {self.id}'
    
    @property
    def get_total_iva(self):
        """calcula o valor do iva"""
        return (self.total_paid - (self.total_paid / Decimal('1.23'))).quantize(Decimal('0.01'))

    @property
    def get_subtotal_sem_iva(self):
        """calcula base sem iva"""
        return (self.total_paid / Decimal('1.23')).quantize(Decimal('0.01'))

class OrderItem(models.Model):
    order = models.ForeignKey(Order, related_name='items', on_delete=models.CASCADE)
    product = models.ForeignKey(Product, related_name='order_items', on_delete=models.CASCADE)
    price = models.DecimalField(max_digits=10, decimal_places=2)
    quantity = models.PositiveIntegerField(default=1)

    @property
    def get_total(self):
        '''
        Subtotal correto
        '''
        if self.price is None or self.quantity is None:
            return Decimal('0.00')
        total = self.price * self.quantity
        return total.quantize(Decimal('0.01'))
    
    @property
    def get_iva_item(self):
        """calcula o iva do item em especifico"""
        total_item = self.get_total
        return (total_item - (total_item / Decimal('1.23'))).quantize(Decimal('0.01'))

    def __str__(self):
        return f'Product {self.product.pk} - {self.product.name}'
    
class OrderStats(Order):
    """
    Classe para a estisticas
    """
    class Meta:
        proxy = True
        verbose_name = 'Estatística de Venda'
        verbose_name_plural = 'Estatísticas de Vendas'