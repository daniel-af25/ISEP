from django.shortcuts import render, redirect
from django.contrib.auth.forms import UserCreationForm
from django.contrib import messages
from django.contrib.auth import login
from django.contrib.auth.decorators import login_required
from .forms import ProfileForm
from .models import Profile
from .forms import CustomUserCreationForm
from products.models import Order

def register_view(request):
    """
    Verifica se o registo foi realizado com sucesso, caso afirmativo guarda o utilizador na base de dados,caso negativo mantém
    a página par ao utilizador poder realizar o registo.
    """
    if request.method == 'POST':
        form = CustomUserCreationForm(request.POST)
        if form.is_valid():
            user = form.save() # Guarda o user na db
            login(request, user) # Faz login imediato
            return redirect('home')
    else:
        form = CustomUserCreationForm() # mudei aqui para adicionar o campo de email no registo
    return render(request, 'accounts/register.html', {'form': form})

@login_required
def update_profile(request):
    # Pega no perfil do utilizador
    profile = Profile.objects.get(user=request.user)
    
    # Se ele fizer "POST" atualiza os dadods
    if request.method == 'POST':
        form = ProfileForm(request.POST, instance=profile)
        if form.is_valid():
            form.save()
            return redirect('home') # em vez de redirect fazer uma messagebox com "alterado com sucesso"
    else:
        # Senão ele só mostra os dados
        form = ProfileForm(instance=profile)
    
    return render(request, 'accounts/profile.html', {'form': form})

@login_required
def profile_view(request):
    profile, _ = Profile.objects.get_or_create(user=request.user)
    orders = Order.objects.filter(user=request.user).order_by('-created')
    
    if request.method == 'POST':
        form = ProfileForm(request.POST, instance=profile)
        if form.is_valid():
            form.save()
            #Mensagem de successo caso as informações sejam atualizadas
            messages.success(request, 'Informação atualizada com sucesso!') 
            return redirect('profile')
    else:
        form = ProfileForm(instance=profile)
    
    return render(request, 'accounts/profile.html', {
        'form': form, 
        'profile': profile,
        'orders': orders
    })
    
@login_required
def user_orders(request):
    # Vamos buscar as encomendas deste utilizador específico
    orders = Order.objects.filter(user=request.user).order_by('-created')
    
    return render(request, 'accounts/user_orders.html', {
        'orders': orders
    })