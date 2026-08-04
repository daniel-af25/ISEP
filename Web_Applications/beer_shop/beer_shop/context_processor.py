from .cart import Cart

def cart(request):
    # Retorna o carrinho como um dicionário disponível em todos os templates
    return {'cart': Cart(request)}