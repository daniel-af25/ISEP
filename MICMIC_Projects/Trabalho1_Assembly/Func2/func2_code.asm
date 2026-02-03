.def temp       = r16 ; Registo temporário para atribuições.
.def loop_cnt   = r18 ; Contador para a sequencias de LEDS.
.def check_btn  = r19 ; Butao usado para verificar se algum dos SWITCHES é pressionado.
.def pause_handler = r21 ; Registo utilizado na reconfiguração de LEDS ligados apos pausa e no delay.
.def led_mask   = r22 ; Mascara de Bits.
.def mode_flag  = r23 ; Registo da sequencia atual, SW1 / SW2
.def saved_temp = r25 ; Registo do estado dos LEDS aquando a pausa.

.cseg 
.org 0x00 
jmp init 
.cseg
.org 0x46 

init:
    ldi  r16, high(RAMEND) ; Inicialização da Stack, obrigatorio pois vamos utilizar delays que trabalham com o clock do CPU.
    out  SPH, r16          ; 
    ldi  r16, low(RAMEND)  ; 
    out  SPL, r16          ; 
    clr  r16                ; Coloca R16 com tudo a 0's.
    out  DDRA, r16          ; Configura DDRA como entradas (SWITCHES).
    ser  r16                ; Coloca R16 com tudo a 1's.
    out  PORTA, r16         ; Configura resistencias de pull-up.
    out  DDRC, r16          ; Configura DDRC como saídas (LEDS).
    rjmp  ciclo              ; Salta para a rotina principal.


ciclo:
    ser  r16                ; Coloca a 1's o registo temporário.
    out  PORTC, r16         ; Desligado o LED0 - LED7.
    clr  mode_flag          ; Coloca a flag do modo de sequencia a 0 pois ainda nao foi ativado nenhuma.
wait_for_start: ; Loop para esperar/verificar uma inicialização de sequência.
    in   r16, PINA          ; Lê o estado dos switches.
    cpi  r16, 0b11111110    ; Faz uma comparação com 0b11111110, ou seja verifica se o PINA0 foi pressionado.
    breq handle_sw1         ; Se sim, saltamos para a Sequencia 1.
    cpi  r16, 0b11111101    ; Faz uma comparação com 0b11111101, ou seja verifica se o PINA1 foi pressionado.
    breq handle_sw2         ; Se sim, saltamos para a Sequencia 2.
    rjmp  wait_for_start     ; Se chega aqui é porque nem a sequencia 1 nem a sequencia 2 foi ativada, e recomeçamos o loop.


handle_sw1:
    ldi  mode_flag, 1       ; Coloca a flag do modo de sequencia a 1, ou seja sequencia 1 ativada.
    ser  temp               ; Coloca o registo temporário tudo a 1's.
    ldi  led_mask, 0b00000001 ; Inicializa a mascara de bits com 1 no PINA0.
    ldi  loop_cnt, 8        ; Coloca no registo que conta os switches 8, inicializa o contador.
sw1_loop:
    rcall check_pause       ; Antes de cada ativação de leds verifica se o PINA5 foi ativado.
    eor  temp, led_mask     ; Inverte os leds que estão a 1 na led_mask e coloca-os no respetivo bit no registo temporário, mantendo os outros iguais.
    out  PORTC, temp        ; Coloca na PORTC o estado definido na linha anterior.
    ldi  r20, 82            ; Atribui ao r20 o valor 82 para realizarmos um delay de 1s.
    rcall delay             ; Chama a rotina de delay.
    lsl  led_mask           ; Shifta a mascara um bit para a esquerda. Ex: 0b00000001 --> 0b00000010
    dec  loop_cnt           ; Decrementa o contador de loop.
    brne sw1_loop           ; Se o contador não estiver a 0, repete o loop até o mesmo estiver a 0.
    rjmp  ciclo              ; Quando todos os LEDS estiverem ligados voltamos á rotina principal para a espera de algum input nos switches.


handle_sw2:
    ldi  mode_flag, 2       ; Coloca a flag do modo de sequencia a 2, ou seja sequencia 2 ativada.
    clr  temp               ; Coloca o registo temporário tudo a 0's.
    ldi  led_mask, 0b10000000 ; Inicializa a mascara de bits com 1 no PINA7.
    ldi  loop_cnt, 8        ; Coloca no registo que conta os switches 8, inicializa o contador.
sw2_loop:
    rcall check_pause       ;  Antes de cada ativação de leds verifica se o PINA5 foi ativado.
    eor  temp, led_mask     ; Inverte os leds que estão a 1 na led_mask e coloca-os no respetivo bit no registo temporário, mantendo os outros iguais.
    out  PORTC, temp        ; Coloca na PORTC o estado definido na linha anterior.
    ldi  r20, 41            ; Atribui ao r20 o valor 82/2 para realizarmos um delay de 0.5s.
    rcall delay             ; Chama a rotina de delay.
    lsr  led_mask           ; Shifta a mascara um bit para a direita. Ex: 0b10000000 --> 0b01000000
    dec  loop_cnt           ; Decrementa o contador de loop.
    brne sw2_loop           ; Se o contador não estiver a 0, repete o loop até o mesmo estiver a 0.
    rjmp  ciclo              ; Quando todos os LEDS estiverem ligados voltamos á rotina principal para a espera de algum input nos switches.


check_pause:
    sbis PINA, 5            ; Se o SW6 nao estiver pressionado skipa a proxima instrução.
    rcall pause_sequence    ; Se o SW6 for de facto pressionado chama a sequencia de pausa.
    ret                     ; Retorna de onde foi chamado.

pause_sequence:
    in   saved_temp, PORTC  ; Guarda o estado de LEDS atual.
wait_pause_release:
    sbis PINA, 5            ; Skipa a proxima instruçao se o SW6 nao estiver mais pressionado.
    rjmp wait_pause_release ; Se o SW6 estiver pressionado loop infinito ate nao estar mais pressionado.
pause_wait:
    in   check_btn, PINA    ; Verifica o estado dos SW's.
    cpi  check_btn, 0b11111110 ; Verifica se o SW0  foi pressionado.
    breq resume_with_sw1    ; Se sim salta para a subrotina que trata o sw1.
    cpi  check_btn, 0b11111101 ; Verifica se o SW0  foi pressionado.
    breq resume_with_sw2    ; Se sim salta para a subrotina que trata o sw2.
    rjmp pause_wait         ; Loop infinito até um switch ser pressionado.

resume_with_sw1:
    in   check_btn, PINA      ; Guarda o estado do PINA0 no registo.
    cpi  check_btn, 0b11111110 ; Verifica se o SW1 já parou de ser pressionado.
    breq resume_with_sw1 ; Se não loop infinito, se sim proxima instrução.
    cpi  mode_flag, 1       ; Verifica se era a a sequencia 1 que estava ativa antes da pausa.
    breq continue_sequence  ; Se sim, saltamos para o continue sequence.
    ldi  mode_flag, 1       ; Estado ativo antes da pausa era o 2, atualizamos a nossa flag para 2.
    out  PORTC, saved_temp  ; Coloca nos LEDS o estado guardado dos LEDS antes da pausa.
    mov  temp, saved_temp   ; Coloca no registo temporário o estado dos LEDS atual.
    clr  r20                ; Coloca o registo R20 a 0's, contador de LEDS.
    ldi  r21, 8             ; Atribui o valor 8 ao R21, será o nosso contador de bits.
    mov  r26, saved_temp    ; Atribui ao R26 o nosso estado atual de LEDS de forma a podermos
count_on_leds:
    lsr  r26                ; Shifta os bits 1 posição para a direita, o bit que se encontra mais á direita vai para a carry flag.
    brcs skip_on_count      ; Se o bit que estava na carry flag era 1 salta para a rotina associada.
    inc  r20                ; Se o bit que estava na carry flag era 0, incrementamos o nosso contador de LEDS.
skip_on_count:
    dec  r21                ; Decrementa o contador de bits.
    brne count_on_leds      ; Loop 8 vezes.
    ldi  led_mask, 0b00000001 ; Começa a mascara na primeira posiçao, pois é a sequencia 1.
    mov  pause_handler, r20; Copia o numero de LEDS ligados antes da pausa para o pause_handler.
    tst  pause_handler            ; Testa se R16 está a zero, pois a tst faz uma operação AND entre dois registos, mas como apenas colocamos 
								  ; um registo no tst e R20 pode ter valores entre 0 e 8 ele só irá ser ativo quando o pause_handler tiver valor 0.
    breq mask_setup_sw1_done; Se o registo estiver a 0 vai para a subtorina mas_setup_sw1_done se não repete a subrotina shift_mask_sw1 até receber o pause_handler for 0.
shift_mask_sw1:
    lsl  led_mask           ; Shifta a mascara para a esquerda até encontrar a posiçao que se encontrava antes da pausa.
    dec  pause_handler      ; Decrementa o contador de LEDS.
    brne shift_mask_sw1     ; Faz o loop até o o pause_handler chegar a 0 ou seja o ultimo led aceso antes da pausa.
mask_setup_sw1_done:
    ldi  loop_cnt, 8        ; Atribui 8 ao loop_cnt
    sub  loop_cnt, r20      ; Subtrai o numero de LEDS que ja se encontravam ligados antes da pausa.
    tst  loop_cnt           ; Verifica se o loop_cnt está a 0 ou seja se todos os LEDS ja estao ligados.
    breq resume1_done       ; Se o loop_cnt estiver a 0 a sequencia está terminado salta para resume1_done.
    rjmp  sw1_loop           ; Se o loop_cnt nao estiver a 0 saltamos para sw1_loop, mas com a mascara de bits na posiçao correta e o numero restante de loops necessários para completar a sequencia corretos tambem.
resume1_done:
    rjmp  ciclo              ; A sequencia está completa, voltamos á rotina principal.

resume_with_sw2:
    in   check_btn, PINA      ; Guarda o estado do PINA1 no registo.
    cpi  check_btn, 0b11111101 ; Verifica se o SW2 já parou de ser pressionado.
    breq resume_with_sw2 ; Se não loop infinito, se sim proxima instrução.
    cpi  mode_flag, 2       ; Verifica se nos encontravamos na sequencia 2.
    breq continue_sequence  ; Se ja nos encontrarmos na sequencia 2 saltamos para continue_sequence, se não vamos para a proxima intrução.
    ldi  mode_flag, 2       ; Mudamos de sequencia, atualizamos a nossa flag.
    out  PORTC, saved_temp  ; Coloca nos LEDS o estado guardado antes da pausa.
    mov  temp, saved_temp   ; Atribui ao registo temporário o estado dos LEDS antes da pausa.
    clr  r20                ; Coloca o R20 a 0's.
    ldi  r21, 8             ; Atribui ao R21 O valor 8, será o nosso contador de bits.
    mov  r26, saved_temp    ; Copiamos o estado dos LEDS antes da pausa para o registo R26.
count_off_leds:
    lsr  r26                ; Shifta os bits 1 posição para a direita, o bit que se encontra mais á direita vai para a carry flag.
    brcc skip_off_count     ; Se o bit que estava na carry flag era 0 salta para a rotina associada.
    inc  r20                ; Se o bit que estava na carry flag era 1, incrementamos o nosso contador de LEDS.
skip_off_count:
    dec  r21                ; Decrementa o contador de bits.
    brne count_off_leds     ; Loop 8 times.
    ldi  led_mask, 0b10000000 ; Começa a mascara na ultima posiçao.
    mov  pause_handler, r20   ; Copiamos o numero de LEDS off para o registo pause_handler.
    tst  pause_handler          ; Testa se R16 está a zero, pois a tst faz uma operação AND entre dois registos, mas como apenas colocamos 
	; um registo no tst e R20 pode ter valores entre 0 e 8 ele só irá ser ativo quando o pause_handler tiver valor 0   
    breq mask_setup_sw2_done; Se o registo estiver a 0 vai para a subrotina mask_setup_sw2_done se não repete a subrotina shift_mask_sw1 até  o pause_handler for 0.
shift_mask_sw2:
    lsr  led_mask           ; Shifta a mascara para a direita até encontrar a posiçao que se encontrava antes da pausa.
    dec  pause_handler      ; Decrementa o contador de leds.
    brne shift_mask_sw2     ; Faz o loop até o o pause_handler chegar a 0 ou seja o ultimo led desligado antes da pausa.
mask_setup_sw2_done:
    ldi  loop_cnt, 8        ; Atribui 8 ao loop_cnt
    sub  loop_cnt, r20      ; Subtrai o numero de LEDS que ja se encontravam ligados antes da pausa.
    tst  loop_cnt           ; Verifica se o loop_cnt está a 0 ou seja se todos os LEDS ja estao desligados.
    breq resume2_done       ; Se o loop_cnt estiver a 0 a sequencia está terminada salta para resume2_done.
    rjmp  sw2_loop           ; Se o loop_cnt nao estiver a 0 saltamos para sw1_loop, mas com a mascara de bits na posiçao correta e o numero restante de loops necessários para completar a sequencia corretos tambem.
resume2_done:
    rjmp  ciclo              ; A sequencia está completa, voltamos á rotina principal.
continue_sequence:
    ret                     ; Retorna para a sequencia que estava ativada previamente através do ret.


delay:
    push r18                
    push r19
    push r20
    push pause_handler
    ldi  r18, 254           
d_loop0:
    ldi  r19, 255           
d_loop1:
    mov  pause_handler, r20       
d_loop2:
    dec  pause_handler            
    brne d_loop2            
    dec  r19                
    brne d_loop1            
    dec  r18               
    brne d_loop0            
    pop  pause_handler            
    pop  r20
    pop  r19
    pop  r18
    ret                    
