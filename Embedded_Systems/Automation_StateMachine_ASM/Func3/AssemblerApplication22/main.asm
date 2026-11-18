.def temp            = r16  ; Registo temporário para operações gerais.
.def currentNumber   = r17  ; Guarda o índice atual (0-8), que corresponde aos números (1-9) no display.
.def loop_counter    = r18  ; Contador de furações.
.def LED_MOTOR_M     = r20  ; Guarda o valor 0b01111111 (liga atuador).
.def LED_MOTOR_MF    = r21  ; Guarda o valor 0b11111110 (pisca atuador).
.def FIM_CICLO       = r22  ; Registo para guardar o valor de fim de ciclo (0xC0).
.equ SW_START        = 0    ; SW1.
.equ SW_INC          = 1    ; SW2.
.equ SW_DEC          = 2    ; SW3.
.equ SW_SENSOR       = 5    ; SW6.



.cseg                   
.org 0x00               
jmp init            
.cseg                   
.org 0x46               
                        


init: 
    ldi     temp, high(RAMEND) ; Carrega a parte alta do último endereço da RAM
    out     SPH, temp          ; Aponta o Stack Pointer High para lá
    ldi     temp, low(RAMEND)  ; Carrega a parte baixa do último endereço da RAM
    out     SPL, temp          ; Aponta o Stack Pointer Low para lá
    ser     temp               ; Coloca temp a 1's
    out     DDRA, temp       ; Define PORTA como SAÍDA
    out     DDRC, temp       ; Define PORTC como SAÍDA
    out     PORTA, temp      ; Escreve 1s em PORTA (apaga atuador)
    ldi     temp, 0b11000000 ; Carrega máscara para PORTD
    out     DDRD, temp       ; PD6 e PD7 são SAÍDAS, PD0-PD5 são ENTRADAS
    ser     temp         ; Coloca temp a 1's
    out     PORTD, temp      ; Ativa resistências de PULL-UP nas entradas (PD0-PD5)
    ldi     LED_MOTOR_M, 0b01111111 
    ldi     LED_MOTOR_MF, 0b11111110 
    ldi     FIM_CICLO, 0xC0;   ; Define o valor para fim de ciclo, 0.
    ldi     currentNumber, 8   ; Define o índice inicial como 8, numero 9.
    rcall   update_display   ; Chama a subrotina para mostrar inicialmente o 9 no display.
    rjmp    main_loop
    

displayNumbers:
    .db 0XF9, 0XA4, 0XB0, 0X99, 0X92, 0X82, 0XF8, 0x80, 0x90 ; Índices [0] (Nº '1') a [8] (Nº '9')


main_loop:
    sbic    PIND, SW_INC        ;Salta se o SW2 (bit=0, premido)
    rjmp    check_dec_btn       ; Se não premido (bit=1), verifica próximo botão.
    rcall   delay_2ms           ; Debounce 2ms.
    sbic    PIND, SW_INC        ; Verifica de novo. Salta se ainda estiver premido (bit=0).
    rjmp    check_dec_btn       ; Se foi solto (bit=1), foi ruído. Salta.
    inc     currentNumber       ; Incrementa o índice
    cpi     currentNumber, 9    ; Compara o índice com 9
    brlo    update_inc_display  ; Salta se for menor que 9 (ou seja, 0-8)
    ldi     currentNumber, 8    ; Se for 9, repõe para 8 (saturação no máx., índice 8 = '9')
update_inc_display:
    rcall   update_display      ; Atualiza o display com o novo número
    rcall   wait_for_release_inc; Espera até o botão ser largado.
check_dec_btn:
    sbic    PIND, SW_DEC        ; Salta se o SW3 (bit=0, premido)
    rjmp    check_start_btn     ; Se não for premido, verifica próximo botão.
    rcall   delay_2ms           ; Debounce
    sbic    PIND, SW_DEC        ; Salta se o SW3 ainda estiver premido (bit=0).
    rjmp    check_start_btn     ; Se foi solto (bit=1), foi ruído. Salta.
    dec     currentNumber       ; Decrementa o índice.
    cpi     currentNumber, 0xFF ; Compara com 0xFF (deteta underflow de 0 para -1)
    brne    update_dec_display  ; Salta se NÃO FOR 0xFF (ou seja, 0-8)
    ldi     currentNumber, 0    ; Se FOR 0xFF, repõe para 0 (saturação no mín., índice 0 = '1')
update_dec_display:
    rcall   update_display      ; Atualiza o display com o novo número
    rcall   wait_for_release_dec; Espera até o botão ser largado.

check_start_btn:
    sbic    PIND, SW_START       ; Salta se SW1 (bit=0, premido).
    rjmp    main_loop            ; Se não premido (bit=1), volta ao início do loop
    rcall   delay_2ms            ; Debounce
    sbic    PIND, SW_START       ; Salta se o SW1 ainda estiver premido (bit=0).
    rjmp    main_loop            ; Se foi solto (bit=1), foi ruído. Salta.
    rcall   wait_for_release_start ; Espera até o botão deixar de ser pressionado.
    rcall   run_action_loop        ; Salta para a rotina de furação.
    rjmp    main_loop            ; Quando a rotina de furação estiver concluida volta para a rotina principal.


run_action_loop:
    mov     loop_counter, currentNumber ; Copia o índice (0-8) para o contador.
    inc     loop_counter                ; Incrementa (para 1-9) para o número de loops.
for_loop_start:
    push    currentNumber       ; Salva o índice atual (0-8) na stack.
    mov     currentNumber, loop_counter ; Copia o contador de furações para currentNumber
    dec     currentNumber       ; Decrementa  para usar como índice 
    rcall   update_display      ; Mostra o número do ciclo atual 
    pop     currentNumber       ; Restaura o índice original 
    out     PORTA, LED_MOTOR_M ; Liga o LED associado ao motor M.
    
wait_sensor_low:
    sbic    PIND, SW_SENSOR   ; Salta se o SW6 (sensor) for ativado (bit=0).
    rjmp    wait_sensor_low       ; Loop ate o SW6 ser ativado.
    out     PORTA, LED_MOTOR_MF ; Liga o LED associado ao motor MF.
    rcall   delay_500ms 
    ser         temp             ; Coloca o registo temporário a 1's.
    out     PORTA, temp         ; Desliga todos os LEDS.
    rcall   delay_500ms
    dec     loop_counter        ; Decrementa o contador de furações.
    brne    for_loop_start      ; Se o contador de furações não estiver a 0 volta a repetir o ciclo.
    out     PORTC, FIM_CICLO    ; Mostra 0 no display.
    rcall   delay_2000ms        ; Espera 2 segundos
    ldi     currentNumber, 8    ; Repõe o índice para 8 (número '9')
    rcall   update_display      
    ret                         ; Regressa à rotina principal.


update_display:
    ldi     ZL, low(displayNumbers << 1)  
    ldi     ZH, high(displayNumbers << 1)
    add     ZL, currentNumber    
    brcc    no_carry             
    inc     ZH                   
no_carry:
    lpm     temp, Z              
    out     PORTC, temp          
    ret


wait_for_release_inc: ; Verificação para esperar o SW2 ser solto.
    sbis    PIND, SW_INC   
    rjmp    wait_for_release_inc 
    ret
wait_for_release_dec: ; Verificação para esperar o SW3 ser solto.
    sbis    PIND, SW_DEC   
    rjmp    wait_for_release_dec 
    ret
wait_for_release_start: ; Verificação para esperar o SW1 ser solto.
    sbis    PIND, SW_START 
    rjmp    wait_for_release_start 
    ret



delay_2ms:
    push    r18
    push    r19
    ldi     r19, 42         
d2_loop1:  
    ldi     r18, 255        
d2_loop2:  
    dec     r18
    brne    d2_loop2       
    dec     r19
    brne    d2_loop1        
    pop     r19
    pop     r18
    ret

delay_500ms:
    push    r18
    push    r19
    push    r20
    ldi     r20, 82         
d500_loop1: 
    ldi     r19, 43         
d500_loop2: 
    ldi     r18, 0          
d500_loop3: 
    dec     r18
    brne    d500_loop3      
    dec     r19
    brne    d500_loop2      
    dec     r20
    brne    d500_loop1      
    pop     r20
    pop     r19
    pop     r18
    ret

delay_2000ms:
    push    r18
    push    r19
    push    r20
    ldi     r20, 82         
d2s_loop1:  
    ldi     r19, 170        
d2s_loop2:  
    ldi     r18, 0          
d2s_loop3:  
    dec     r18
    brne    d2s_loop3       
    dec     r19
    brne    d2s_loop2       
    dec     r20
    brne    d2s_loop1       
    pop     r20
    pop     r19
    pop     r18
    ret