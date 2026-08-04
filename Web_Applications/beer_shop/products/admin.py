from django.contrib import admin
from django.db.models import Sum, Count
from django.db.models.functions import ExtractMonth, Round
from django.shortcuts import render
from django.urls import path
from django.http import HttpResponse
from django.utils.html import format_html
from django.shortcuts import get_object_or_404
from django.urls import reverse
from .models import Product, Category, Order, OrderItem, User, OrderStats


@admin.register(OrderStats)
class OrderStatsAdmin(admin.ModelAdmin):
    def has_add_permission(self, request): return False
    def has_delete_permission(self, request, obj=None): return False
    def has_change_permission(self, request, obj=None): return False

    def changelist_view(self, request, extra_context=None):
        # Recolhe os dados para a estatistica de + produtos vendidos
        product_sales = OrderItem.objects.values('product__name') \
            .annotate(total_qty=Sum('quantity')).order_by('-total_qty')[:5]
        
        # Dados para mapear por meses
        month_names = {1: 'Jan', 2: 'Fev', 3: 'Mar', 4: 'Abr', 5: 'Mai', 6: 'Jun', 
                       7: 'Jul', 8: 'Ago', 9: 'Set', 10: 'Out', 11: 'Nov', 12: 'Dez'}
        
        #Extrai vendas por mes
        monthly_raw = Order.objects.annotate(m=ExtractMonth('created')) \
            .values('m').annotate(total=Sum('total_paid')).order_by('m')
        
        monthly_labels = [month_names.get(x['m'], str(x['m'])) for x in monthly_raw]
        monthly_values = [float(x['total']) for x in monthly_raw]
        
        # Recolhe os dados para a estatistica de clientes com mais valor gasto
        best_clients = Order.objects.values('full_name', 'email') \
            .annotate(total_spent=Round(Sum('total_paid'),2)) \
            .order_by('-total_spent')[:10]
        extra_context = {
            **self.admin_site.each_context(request),
            'title': 'Painel de Estatísticas FansFans',
            'product_sales': product_sales,
            'product_labels': [item['product__name'] for item in product_sales],
            'product_values': [item['total_qty'] for item in product_sales],
            'monthly_labels': monthly_labels,
            'monthly_values': monthly_values,
            'best_clients': best_clients,
            'staff_ranking': Order.objects.filter(managed_by__isnull=False)
                            .values('managed_by__username')
                            .annotate(count=Count('id')).order_by('-count'),
        }
        return render(request, 'order_stats.html', extra_context)

class OrderItemInline(admin.TabularInline):
    model = OrderItem
    extra = 0
    readonly_fields = ['get_total', 'get_iva_item']
    
    def has_change_permission(self, request, obj=None):
        if obj and obj.delivery_status in ['enviada', 'entregue']:
            return False
        return True

    def has_add_permission(self, request, obj=None):
        if obj and obj.delivery_status in ['enviada', 'entregue']:
            return False
        return True

    def has_delete_permission(self, request, obj=None):
        if obj and obj.delivery_status in ['enviada', 'entregue']:
            return False
        return True

@admin.register(Product)
class ProductAdmin(admin.ModelAdmin):
    list_display = ('name','promotion_percentage', 'price', 'promotion_price', 'stock', 'abv', 'category')
    list_editable = ('price','promotion_percentage', 'stock') 
    search_fields = ('name',)

admin.site.register(Category)

@admin.register(Order)
class OrderAdmin(admin.ModelAdmin):
    inlines = [OrderItemInline]
    list_display = ['id', 'full_name','payment_method', 'billing_status', 'delivery_status', 'created', 'download_pdf_button']
    list_filter = ['billing_status', 'delivery_status']
    list_editable = ['billing_status'] 

    def download_pdf_button(self, obj):
        return format_html(
            '<a class="button" href="{}" target="_blank" style="background-color: #f89406; color: white;">📄 PDF</a>',
            reverse('admin:download_pdf_guia', args=[obj.pk])
        )
    download_pdf_button.short_description = 'Guia'

    def get_urls(self):
        urls = super().get_urls()
        custom_urls = [
            path('download-pdf/<int:order_id>/', self.admin_site.admin_view(self.process_download_pdf), name='download_pdf_guia'),
        ]
        return custom_urls + urls

    def process_download_pdf(self, request, order_id):
        order = get_object_or_404(Order, pk=order_id)
        pdf = order.gerar_pdf_guia()
        response = HttpResponse(pdf, content_type='application/pdf')
        response['Content-Disposition'] = f'attachment; filename="Guia_Remessa_{order.id}.pdf"'
        return response
    
    def get_readonly_fields(self, request, obj=None):
        if not obj:
            return ['managed_by', 'total_paid', 'created']
        if obj.delivery_status == 'entregue':
            return [f.name for f in self.model._meta.fields] + ['get_total_iva', 'get_subtotal_sem_iva']
        
        if obj.delivery_status == 'enviada':
            all_fields = [f.name for f in self.model._meta.fields] + ['get_total_iva', 'get_subtotal_sem_iva']
            return [f for f in all_fields if f != 'delivery_status']

        return ['managed_by', 'total_paid', 'created']
    
    def save_model(self, request, obj, form, change):
        if change:
            old_obj = Order.objects.get(pk=obj.pk)
            
            if old_obj.delivery_status == 'enviada' and obj.delivery_status == 'pendente':
                obj.delivery_status = 'enviada'
            

            if old_obj.delivery_status != 'enviada' and obj.delivery_status == 'enviada':
                obj.managed_by = request.user
                
        super().save_model(request, obj, form, change)

    def has_delete_permission(self, request, obj=None):
        if obj and obj.delivery_status in ['enviada', 'entregue']:
            return False
        return super().has_delete_permission(request, obj)