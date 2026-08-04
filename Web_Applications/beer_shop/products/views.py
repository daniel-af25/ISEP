from django.conf import settings
from django.shortcuts import render, get_object_or_404, redirect
from django.db.models import Sum
from django.contrib.auth.decorators import login_required
from .models import Product, Order, OrderItem
from beer_shop.cart import Cart
from django.http import JsonResponse
from accounts.models import Profile
from django.contrib.auth import get_user_model
from django.core.mail import send_mail


def index(request):
    """
    Página de catálogo geral de cervejas.
    """
    beers = Product.objects.all()
    return render(request, 'index.html', {'beers': beers})

def home(request):
    popular_beers = Product.objects.annotate(
        total_qty=Sum('order_items__quantity')
    ).filter(total_qty__gt=0).order_by('-total_qty')[:3]
    
    return render(request, 'home.html', {'popular_beers': popular_beers})

def beer_detail(request, pk):
    """
    Página de detalha de cerveja dinâmica.
    """
    beer = get_object_or_404(Product, pk=pk)
    return render(request, 'beer_detail.html', {'beer': beer})

def cart_summary(request):
    cart = Cart(request)
    # Passamos o carrinho para o template para podermos listar os itens
    return render(request, 'cart_summary.html', {'cart': cart})

def cart_add(request):
    cart = Cart(request)
    if request.POST.get('action') == 'post':
        product_id = int(request.POST.get('beerid'))
        product_qty = int(request.POST.get('beerqty'))
        
        product = get_object_or_404(Product, id=product_id)
        
        cart.add(product=product, qty=product_qty)

        request.session.modified = True 

        cart_quantity = cart.__len__()
        return JsonResponse({'qty': cart_quantity})
    
def cart_delete(request):
    cart = Cart(request)
    if request.POST.get('action') == 'post':
        product_id = int(request.POST.get('productid'))
        
        cart.delete(product_id=product_id)

        cart_quantity = cart.__len__()
        total_base = float(cart.get_total())
        total_com_portes = float(cart.get_total_with_shipping())

        return JsonResponse({
            'qty': cart_quantity,
            'total': total_base,
            'total_final': total_com_portes
        })
        
def cart_update(request):
    cart = Cart(request)
    if request.POST.get('action') == 'post':
        product_id = int(request.POST.get('productid'))
        product_qty = int(request.POST.get('productqty'))

        product = get_object_or_404(Product, id=product_id)

        if product.stock < product_qty:
            return JsonResponse({
                'error': 'Quantia fora de stock. Escolha uma quantia menor.',
                'qty': cart.__len__(),
                'total': float(cart.get_total())
            })
            
        cart.update(product_id=product_id, qty=product_qty)

        subtotal = float(product.price) * product_qty
        
        total_base = float(cart.get_total())
        total_com_portes = float(cart.get_total_with_shipping())

        return JsonResponse({
            'qty': cart.__len__(),
            'total': total_base,
            'total_final': total_com_portes,
            'subtotal': float(subtotal)
        })

@login_required(login_url='login') # o utilizador precisa de ter uma conta aberta para acessar ao checkout      
def checkout(request):
    cart = Cart(request)
    # Procuramos o perfil do utilizador logado
    try: # verificamos se já tem morada preenchida no perfil
        shipping_address = Profile.objects.get(user=request.user)
    except Profile.DoesNotExist:
        shipping_address = None # se não tem o form do checkout vai estar vazio

    return render(request, 'checkout.html', {
        'cart': cart,
        'shipping_address': shipping_address
    })
    
@login_required(login_url='login')    
def place_order(request):
    cart = Cart(request)
    
    if len(cart) == 0:
        return redirect('product_list')
    
    if request.method == 'POST':
        subtotal_produtos = cart.get_total()
        valor_portes = 0 if subtotal_produtos >= 30 else 3
        total_final = subtotal_produtos + valor_portes

        order = Order.objects.create(
            user=request.user,
            full_name=request.POST.get('full_name'),
            email=request.POST.get('email'),
            address1=request.POST.get('address1'),
            city=request.POST.get('city'),
            zip_code=request.POST.get('zip_code'),
            shipping_price=valor_portes,
            total_paid=total_final,
            payment_method=request.POST.get('payment_method')
        )

        for item in cart:
            product = item['product']
            
            # garantir que ainda há stock
            if product.stock < item['qty']:
                return redirect('cart_summary')

            OrderItem.objects.create(
                order=order,
                product=product,
                price=item['price'],
                quantity=item['qty']
            )
            
            product.stock -= item['qty']
            product.save()

        # Email e limpeza do cart
        send_pay_email(order.full_name, order.email, order.total_paid, order.payment_method, order.id)
        cart.clear()
        
        return render(request, 'order_success.html', {'order': order})
    
    return redirect('checkout')

@login_required
def order_detail(request, pk):
    order = get_object_or_404(Order, id=pk, user=request.user)
    items = order.items.all() 
    
    subtotal_produtos = sum(item.price * item.quantity for item in items)
    
    return render(request, 'accounts/order_detail.html', {
        'order': order,
        'items': items,
        'subtotal_produtos': subtotal_produtos
    })

def send_pay_email(full_name: str, email: str, total_paid: float, payment_method: str, order_id: int):
    #transformar em str o total pago porque é mais facil user
    amount_paid = f"{total_paid:.2f}€"
    #meter todo em uppers case
    method = payment_method.upper()
    #se o metodo for 'mbway', criar assunto e mensagem baseado no mesmo'
    if method == 'MBWAY': 
        problem = False
        assunto = f"FansFans Beer Shop - Instruções de Pagamento - MB WAY"
        mensagem = (
            f'Olá {full_name},\n\n'
            f'Para concluir a sua compra no valor de {amount_paid}, por favor siga estes passos:\n\n'
            f'1. Confirme a notificação que recebeu na sua app MB WAY.\n'
            f'2. Valide a transação com o seu PIN MB WAY.\n\n'
            f'O pagamento deve ser efetuado nos próximos 5 minutos para garantir a validade da encomenda.\n\n'
            f'Com os melhores cumprimentos,\n'
            f'A Equipa FansFans\n'
            f'------------------------------------------------------------\n'
            f'Beba com moderação. \n'
            f'Este é um e-mail gerado automaticamente, por favor não responda diretamente.'
            )
    #se o metodo for 'dinheiro', criar assunto e mensagem baseado no mesmo'
    elif method == 'DINHEIRO':
        problem = False 
        assunto = f"FansFans Beer Shop - Instruções de Pagamento - Numerário"
        mensagem = (
            f'Olá {full_name},\n\n'
            f'A sua encomenda no valor de {amount_paid} foi registada com sucesso.\n'
            f'Como selecionou o pagamento em "Dinheiro", lembramos que poderá efetuar o pagamento:\n\n'
            f'- Diretamente na nossa loja física no momento da recolha.\n'
            f'- No momento da entrega.\n\n'
            f'Com os melhores cumprimentos,\n'
            f'A Equipa FansFans\n'
            f'------------------------------------------------------------\n'
            f'Beba com moderação. \n'
            f'Este é um e-mail gerado automaticamente, por favor não responda diretamente.'
            )
    #se o metodo for 'multibanco', criar assunto e mensagem baseado no mesmo'
    elif method == 'MULTIBANCO':
        problem = False
        assunto = f"FansFans Beer Shop - Instruções de Pagamento - Multibanco"
        mensagem = (
            f'Olá {full_name},\n\n'
            f'A sua encomenda no valor de {amount_paid} foi registada com sucesso.\n\n'
            f'Para proceder ao pagamento da sua encomenda realize uma transferência bancária para os seguintes dados:\n\n'
            f'Entidade: 25786\n'
            f'Referência: 548 147 214\n'
            f'Montante: {amount_paid}\n\n'
            f'A sua encomenda será processada assim que recebermos a confirmação bancária.\n\n'
            f'Com os melhores cumprimentos,\n'
            f'A Equipa FansFans\n'
            f'------------------------------------------------------------\n'
            f'Beba com moderação. \n'
            f'Este é um e-mail gerado automaticamente, por favor não responda diretamente.'
            )
    #se o metodo nao for nenhum dos especificados previamente envia mensagem a dizer que houve um erro
    #obs: se for esta mensagem a calhar o email que é enviado como notificação á staff também é DIFERENTE!!!
    else:
        problem = True
        assunto = f"FansFans Beer Shop - Erro Encomenda"
        mensagem = (
            f'Olá {full_name},\n\n'
            f'Existiu um erro aquando processamento da encomenda, por favor entre em contacto com a nossa equipa de forma a que possamos resolver o problema.\n'
            f'Com os melhores cumprimentos,\n'
            f'A Equipa FansFans\n'
            f'------------------------------------------------------------\n'
            f'Beba com moderação. \n'
            f'Este é um e-mail gerado automaticamente, por favor não responda diretamente.'
            )
    try:
        send_mail(
            subject=assunto,
            message=mensagem,
            from_email=settings.DEFAULT_FROM_EMAIL, 
            recipient_list=[email],                 
            fail_silently=False,
        )
    except:
        print(f"Erro no envio do email.")
    else:
        send_notification_email(full_name, amount_paid, method, order_id, problem)


def send_notification_email(full_name: str, amount_paid: str, method: str, order_id: int, problem: bool):
    if problem:
        assunto = f"NOVA ENCOMENDA #{order_id} COM ERRO NAS DEFINIÇÕES DE PAGAMENTO"
        mensagem = (f"Encomenda #{order_id} deu entrada com erro, dar o respetivo seguimento através do backoffice.\n\n"
                    f'Com os melhores cumprimentos,\n'
                    f'Administração FansFans\n'
                    f'------------------------------------------------------------\n'
                    f'Este é um e-mail gerado automaticamente, por favor não responda diretamente.'
                    )
    else:
        assunto = f"NOVA ENCOMENDA #{order_id}"
        mensagem = (f"Encomenda #{order_id} deu entrada, dar o respetivo seguimento através do backoffice.\n\n"
                    f"Resumo dos detalhes:\n\t- Nome: {full_name}\n\t- Total: {amount_paid}\n\t- Método: {method}\n\n"
                    f"Para o resto dos detalhes verificar encomenda específica no backoffice.\n\n"
                    f'Com os melhores cumprimentos,\n'
                    f'Administração FansFans\n'
                    f'------------------------------------------------------------\n'
                    f'Este é um e-mail gerado automaticamente, por favor não responda diretamente.'
                    )
    User = get_user_model()
    staff_emails = list(User.objects.filter(is_staff=True).values_list('email', flat=True))
    try:
        send_mail(
            subject=assunto,
            message=mensagem,
            from_email=settings.DEFAULT_FROM_EMAIL, 
            recipient_list=staff_emails,                 
            fail_silently=False,
        )
    except:
        print(f"Erro no envio do email.")
 