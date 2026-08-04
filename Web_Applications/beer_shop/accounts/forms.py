from django import forms
from .models import Profile
from django.contrib.auth.models import User
from django.contrib.auth.forms import UserCreationForm

class ProfileForm(forms.ModelForm):
    class Meta:
        model = Profile
        fields = ['phone', 'address1', 'address2', 'city', 'zip_code']
        labels = {
            'phone': 'Telemóvel',
            'address1': 'Rua e Número',
            'address2': 'Apartamento, andar, etc.',
            'city': 'Cidade',
            'zip_code': 'Código-Postal',
        }
        
class CustomUserCreationForm(UserCreationForm):
    email = forms.EmailField(required=True, help_text="Obrigatório. Digite um e-mail válido.")

    class Meta(UserCreationForm.Meta):
        model = User
        # ordem dos campos que aparecem no {{ form.as_p }}
        fields = UserCreationForm.Meta.fields + ('email',)

    def save(self, commit=True):
        user = super().save(commit=False)
        user.email = self.cleaned_data["email"]
        if commit:
            user.save()
        return user