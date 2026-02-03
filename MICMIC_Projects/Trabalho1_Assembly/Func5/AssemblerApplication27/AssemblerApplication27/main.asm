.equ SWstart=0         ; SW1
.equ SWnext=3         ; SW3
.def temp_int = R6     
.def val_disp_0 = R12    ; Valor a mostrar no display 0
.def val_disp_1 = R13    ; Valor a mostrar no display 1
.def val_disp_2 = R14    ; Valor a mostrar no display 2
.def val_disp_3 = R15    ; Valor a mostrar no display 3
.def cnt_ecra = R16    ; Display 1, 2 , 3 , 4
.def temp = R19        ; Registo temporário de uso geral
.def cnt_int = R20     ; Contador para a temporização de 250ms (feito na ISR)
.def nplaca = R21      ; Dígito (0-9) a ser mostrado durante a seleção
.def var_mostrar = R22 ; Flag (bandeira) que autoriza 'nplaca' a incrementar
.def temp_2 = R23      ; Registo temporário de uso geral 
.def display = R25     ; Dígito ativo durante a seleção (0, 1, 2, 3)
.def estado = R24      ; Máquina de estados (0=Idle, 1=Select, 2=Blink, 3=Reset)
.def cnt_blink = R28   ; Contador decrescente para o tempo de blink
.def blink_flag = R29  ; Flag que autoriza 'cnt_blink' a decrementar


.cseg                 ; Início do Segmento de Código
.org 0x0000           
rjmp main
.cseg                
.org 0x001E          
 rjmp inttimer
.cseg                 
.org 0x0046           ; Início do programa 


table: .db 0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90
table_display: .db 0b00000000,0b01000000,0b10000000,0b11000000


init:
    ; Configura PORTD: PD6,PD7 como saída (controlo display), resto entrada
    ldi temp,0b11000000
    out DDRD,temp
    ldi temp,0b11111111
    out DDRC,temp
    ldi temp,0b11111111
    out PORTC,temp
    ; Configura Timer0 para modo CTC, prescaler=64
    ldi temp,249          
    out OCR0,temp
    ldi temp,0b00001011   
    out TCCR0,temp
    ; Zera todas as variáveis de estado e contagem
    ldi temp,0
    mov nplaca,temp
    mov display,temp
    mov cnt_ecra,temp
    mov var_mostrar,temp
    mov estado,temp
    mov blink_flag,temp
    
    ; Carrega "0000" nos displays (estado inicial)
    ldi ZH, high(table<<1) 
    ldi ZL, low(table<<1)
    lpm temp, Z+           
    mov val_disp_0, temp   ; Guarda '0' em todos os 4 registos de display
    mov val_disp_1, temp
    mov val_disp_2, temp
    mov val_disp_3, temp
    
    ; Define contadores e liga interrupções
    ldi cnt_int,250        ; Prepara contador para 250ms
    ldi cnt_blink,10       ; Prepara contador para 10*250ms = 2.5s
    ldi temp,0b00000010    ; Ativa Interrupção de Comparação A do Timer0 (OCIE0A)
    out TIMSK,temp
    sei                    ; Ativa interrupções globalmente
    ret                    ; Retorna da sub-rotina


main:
    ; Configuração de Stack
    ldi temp,low(RAMEND)
    out SPL,temp
    ldi temp,high(RAMEND)
    out SPH,temp
    call init              


start:
    sbic PIND,SWstart      ; Espera que SW1 seja premido
    rjmp start				; Loop até o SW1 ser premido
    ldi estado,1           ; Atribui ao estado o valor 1.

loop:
    cpi estado,1           ; Verifica se o estado = 1.
    breq state1            ; Se sim salta para state1.
    cpi estado,2           ; Verifica se o estado = 2.
    breq state2            ; Se sim salta para state2.
    cpi estado,3           ; Verifica se o estado = 3.
    breq state3            ; Se sim salta para state3.
    rjmp start              ; Se estado = 0, volta para o start.


state1:
    cpi var_mostrar,0      ; A ISR já deu ordem para atualizar? (flag=1)
    breq check_next        ; Não, então vai verificar o botão 'Next'
    ldi var_mostrar,0      ; Rearma a flag
    inc nplaca             ; Avança o número a mostrar (0->1->2...)
    cpi nplaca, 10         ; Já chegou a 9?
    brne loop              ; Não, volta ao loop
    ldi nplaca,0           ; Sim, recomeça no 0
    rjmp loop              ; Volta ao loop

check_next:
    sbic PIND,SWnext       ; SW3 premido?
    rjmp loop              ; Não, volta ao loop-
    rcall Delay20          ; Espera 20ms (debounce)
    sbic PIND,SWnext       ; Confirma que ainda está premido
    rjmp loop              ; Não (foi ruído), volta ao loop
    ; Sim, é um clique válido:
    rcall valor            ; Chama sub-rotina 'valor' (guarda valor do display)
    cpi display,0
    breq save0
    cpi display,1
    breq save1
    cpi display,2
    breq save2
    mov val_disp_3,R17     ; Guarda no dígito 3
    rjmp adv_next
save0:
    mov val_disp_0,R17     ; Guarda no dígito 0
    rjmp adv_next
save1:
    mov val_disp_1,R17     ; Guarda no dígito 1
    rjmp adv_next
save2:
    mov val_disp_2,R17     ; Guarda no dígito 2

adv_next:
    inc display            ; Avança para o próximo display
    ldi nplaca,0           ; Reinicia o contador de
    ldi cnt_int,250        ; Reinicia o timer de 250ms
    cpi display,4          ; Já preenchemos os 4 dígitos?
    brne wait_rel          ; Não, espera que o botão seja libertado
    ; Sim, 4 dígitos preenchidos:
    ldi estado,2           ; Avança para o Estado 2 (Blink)
    
wait_rel:
    sbiS PIND,SWnext       ; Espera aqui até o botão 'Next' ser libertado
    rjmp wait_rel
    rjmp loop              ; Botão libertado, volta ao loop principal


state2:
    cpi blink_flag,0       ; A ISR já deu ordem para decrementar? (flag=1)
    breq loop              ; Não, volta ao loop
    ldi blink_flag,0       ; Rearma a flag
    dec cnt_blink          ; Decrementa o contador de blink
    brne loop              ; Se ainda não é 0, volta ao loop
    ldi estado,3           ; Avança para o Estado 3 (Reset)
    rjmp loop


state3:
;Reset, para voltarmos ao estado inicial
    ldi temp,0
    mov nplaca,temp
    mov display,temp
    mov cnt_ecra,temp
    mov var_mostrar,temp
    mov estado,temp        
    mov blink_flag,temp
    ldi cnt_int,250
    ldi cnt_blink,10
    ldi ZH, high(table<<1)
    ldi ZL, low(table<<1)
    lpm temp, Z+
    mov val_disp_0, temp
    mov val_disp_1, temp
    mov val_disp_2, temp
    mov val_disp_3, temp
    rjmp start            


inttimer:
    in temp_int,SREG
    push temp_int
    push temp
    push temp_2
    push R17
    push ZL
    push ZH
    dec cnt_int            
    brne scan              
    ldi cnt_int,250        
    cpi estado,1          
    breq flag_show         
    cpi estado,2          
    breq flag_blink        
    rjmp scan              
flag_show:
    ldi var_mostrar,1     
    rjmp scan
flag_blink:
    ldi blink_flag,1       

scan:
    ldi ZH,high(table_display<<1) 
    ldi ZL,low(table_display<<1)
    add ZL,cnt_ecra       
    lpm R17,Z             
    in temp_2,PORTD      
    ldi temp,0b00111111    
    and temp_2,temp       
    or temp_2,R17         
    out PORTD,temp_2       
    cp cnt_ecra,display    
    brne show_saved        
    ldi ZH,high(table<<1)
    ldi ZL,low(table<<1)
    add ZL,nplaca
    lpm R17,Z              
    out PORTC,R17
    rjmp next_digit

show_saved:
    cpi estado, 2          
    brne show_digits_normally
    mov temp_2, cnt_blink  
    andi temp_2, 0x01      
    brne show_blank        

show_digits_normally:
    cpi cnt_ecra,0
    breq show0
    cpi cnt_ecra,1
    breq show1
    cpi cnt_ecra,2
    breq show2
    out PORTC,val_disp_3   ; Mostra o valor do dígito 3
    rjmp next_digit
show0:
    out PORTC,val_disp_0   ; Mostra o valor do dígito 0
    rjmp next_digit
show1:
    out PORTC,val_disp_1   ; Mostra o valor do dígito 1
    rjmp next_digit
show2:
    out PORTC,val_disp_2   ; Mostra o valor do dígito 2
    rjmp next_digit

show_blank:
    ldi temp, 0xFF         
    out PORTC, temp
    
next_digit:
    inc cnt_ecra           ; Avança para o próximo dígito (0->1->2->3)
    cpi cnt_ecra,4         ; Já passou o 3?
    brne endint            ; Não, continua
    ldi cnt_ecra,0         ; Sim, volta ao dígito 0
    
endint:
    pop ZH
    pop ZL
    pop R17
    pop temp_2
    pop temp
    pop temp_int
    out SREG,temp_int
    reti                  


valor:
    push ZL
    push ZH
    ldi ZH,high(table<<1)
    ldi ZL,low(table<<1)
    add ZL,nplaca         
    lpm R17,Z              
    pop ZH
    pop ZL
    ret

Delay20:
    push r27
    push r26
    push r18
    ldi r27,100
d3: ldi r26,10
d4: ldi r18,105
d5: dec r18
    brne d5
    dec r26
    brne d4
    dec r27
    brne d3
    pop r18
    pop r26
    pop r27
    ret