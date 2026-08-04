from django.contrib import admin
from django.urls import path, include
from django.conf import settings
from django.conf.urls.static import static
from products import views as product_views
from accounts import views as account_views

urlpatterns = [
    path('admin/', admin.site.urls),
    path('accounts/', include('django.contrib.auth.urls')), # Built-in Login/Logout
    path('accounts/register/', account_views.register_view, name='register'), # Your Register
    path('', product_views.home, name='home'),                      
    path('catalog/', product_views.index, name='product_list'),
    path('catalog/<int:pk>/', product_views.beer_detail, name = 'beer_detail'),
    path('cart/', product_views.cart_summary, name='cart_summary'),
    path('add/', product_views.cart_add, name='cart_add'),
    path('delete/', product_views.cart_delete, name='cart_delete'),
    path('update/', product_views.cart_update, name='cart_update'),
    path('checkout/', product_views.checkout, name='checkout'),
    path('accounts/profile/', account_views.profile_view, name='profile'),
    path('place-order/', product_views.place_order, name='place_order'),
    path('encomenda/<int:pk>/', product_views.order_detail, name='order_detail'),
] + static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
