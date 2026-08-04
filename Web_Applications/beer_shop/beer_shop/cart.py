from products.models import Product
from decimal import Decimal
import copy

class Cart():
    def __init__(self, request):
        self.session = request.session
        cart = self.session.get('session_key')
        if 'session_key' not in request.session:
            cart = self.session['session_key'] = {}
        self.cart = cart
        
    def clear(self):
        if 'session_key' in self.session:
            del self.session['session_key']
            self.save()
        
    def save(self):
        self.session.modified = True

    def add(self, product, qty):
        product_id = str(product.id)
        if product_id not in self.cart:
            self.cart[product_id] = {'price': str(product.price), 'qty': int(qty)}
        else:
            self.cart[product_id]['qty'] += int(qty)
        self.save()

    def __len__(self):
        return sum(item['qty'] for item in self.cart.values()) 
    
    def __iter__(self):
        product_ids = self.cart.keys()
        products = Product.objects.filter(id__in=product_ids)
        cart = copy.deepcopy(self.cart)

        for product in products:
            cart[str(product.id)]['product'] = product

        for item in cart.values():
            # Convertemos para float apenas no final para compatibilidade com o template
            item['total_price'] = float(Decimal(item['price']) * item['qty'])
            yield item 
            
    def get_total(self):
        total = sum(Decimal(item['price']) * item['qty'] for item in self.cart.values())
        return total
    
    def get_total_with_shipping(self):
        total = self.get_total()
        shipping = 0 if total >= 30 else 3
        return total + shipping
    
    def delete(self, product_id):
        product_id = str(product_id)
        if product_id in self.cart:
            del self.cart[product_id]
            self.save()
            
    def update(self, product_id, qty):
        product_id = str(product_id)
        if product_id in self.cart:
            self.cart[product_id]['qty'] = int(qty)
            self.save()