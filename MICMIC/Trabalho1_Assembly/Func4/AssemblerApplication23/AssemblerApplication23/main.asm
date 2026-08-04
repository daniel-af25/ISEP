.def estado        = r17  ; Guarda o estado atual, estado 1- Roleta , estado 2 - holder, estado 3 stop
.def temp          = r16  ; Temp register
.def cnt           = r18  ; Contador 
.def numero         = r19  ; Guardar o numero da roleta que foi clicado
.def LigarOuDesligar = r20  ; Flag para verificar o stop
.def cnt_Wait      = r21  ; Conta o numero de blinks quando o stop é pressionado
.equ start         = 0    ; Bit para start no PIND
.equ stop          = 1    ; Bit para stop no PIND.
.equ cnt_500ms     = 100  ; 500ms
.equ cnt_200ms     = 40   ; 200ms
.equ indicenumeros = 6   ; Começa no 5

.cseg
.org 0x0000
    jmp main         
	    
.cseg
.org 0x001E
    jmp interrupt        

.cseg
.org 0x46

tabela:                  
    .db 0X92, 0X99, 0XB0, 0XA4, 0XF9, 0XC0 ; Indices [0](0) a [5](5)
	
init:
    ldi temp, 0b11000000  ; Escolha do display mais á direita nos 2 MSB e o resto definimos os switches como entradas.
    out DDRD, temp			
    out PORTD, temp      ; Resistencias de PULL-UP.
    ser temp             ; Mete R16 a zeros.
    out DDRC, temp       ; Port C toda como inputs (0's) para servir como display de sete digitos.
    out PORTC, temp      ; Mete os display a zeros inicialente.
    ldi temp, 77
    out ocr0, temp       ; Atribui 77 
    ldi temp, 0b00001111 ; Configura-se PRESCALER a 1024 e o modo de funcionamento do timer a 2, ou seja contador.
    out tccr0, temp
    in temp, timsk
    ori temp, 0b00000010 
    out timsk, temp
    ldi estado, 0        ; Guarda 0 no R17.
    ldi LigarOuDesligar, 0 ; Inicializa a flag do stop a 0.
    ldi numero,indicenumeros  ; Atribui ao R19 o numero em que devera começar a sequência aleatória.
    sei                  ; Inicializa a flag global.
    ret					 ; Retorna o call.

main:
    ldi temp, high(ramend); Inicialização da Stack.
    out sph, temp
    ldi temp, low(ramend)
    out spl, temp
    call init            ; Chama a rotina inicialização.

ciclo:                   
    sbic pind, start     ; Verifica se existiu algum input nos switches, se sim salta a proxima instrução se não vai para a proxima instrução que é um loop infinito.
    rjmp ciclo           
    ldi numero, indicenumeros    ; Atribui ao R19 o indice de numeros (6).


estado1:
    ldi cnt, cnt_200ms   ; Atribui ao R18 o valor 40 para procedermos ao counter de 200ms
    ldi estado, 1        ; Coloca no R16 a 1.
loop1:
    sbic pind, stop      ; LOOP até Stop ser clicado
    rjmp loop1           
    call TestarTransicao ; Salta para a rotina apos o STOP.
    ldi estado, 2        ; Atribui ao R17 o valor 2.
    ldi cnt, cnt_500ms   ; Atribui ao R18 o valor 100 de forma a contar 500ms.
    ldi cnt_Wait, 6      ; Contador para o numero de Blinks.


estado2:
    brts estado3         ; Verifica estado flag global se estiver a 1 salta para o estado 3.
    sbic pind, stop      ; Verifica o se foi pressionad o stop(SW2)
    rjmp estado2         ; Se nao foi loop infinito
    call TestarTransicao ; Se pressionada delay de 1ms
    rjmp estado1         ; Volta para o Estado 1

estado3:
    ldi estado, 3        ; Atribui ao R17 o estado 3, para o programa saber que nos encontramos na para apos o STOP.
    clt                  ; Reseta a flag global
    rjmp ciclo           ; Vai de volta para o ciclo inicial de forma a reinicializar o programa quando clicado SW1.


interrupt:
    push temp            ; Puxa o registo para a Stack
    in temp, sreg        ; Guarda no R16 o estado da flag.
    dec cnt              ; Decrementa no registo
    brne end             ; Se o contador nao estiver 0
    cpi estado, 0
    breq end             ; Salta para o END se nao estiver a 0.
    cpi estado, 1
    breq atuarEstado1    ; Interrupçao do estado 1.
    cpi estado, 2
    breq atuarEstado2    ; Interrupçao do estado 2.
    cpi estado, 3
    breq atuarEstado3    ; Interrupçao do estado 3.
    jmp end


atuarEstado1:
    ldi cnt, cnt_200ms   ; Carrega o CNT para mais 200ms.  
    dec numero            ; Decrementa um numero no contador do index de numeros
    call LigarDisplay    ; Mostra numero
    cpi numero, 0         ; Verifica se o contador esta a 0 ou seja se ja conclui a sequencia 5 -> 0
    brne end             ; Se o contador nao estiver a 0 salta para o end.
    ldi numero, indicenumeros   ; Se concluida volta a registar no registo o valor  para ser repetida denovo
    rjmp end


atuarEstado2:
    ldi cnt, cnt_500ms   ; Reseta o counter para os proximos 500ms
    cpi LigarOuDesligar, 0 ; Verifica se o display esta on
    breq desligar        ; Se estiver on desliga-o.
    call LigarDisplay
    ldi LigarOuDesligar, 0 ; Seta flag para indica que o display se encontra ligado.
    dec cnt_Wait         ; Decrementa o contador de blinks.
    breq DarSetFlag      ; Se ja blinkou as 6x volta vai para a proxima etapa.
    rjmp end
desligar:
    ldi r30, 0xff        ; Coloca os segmentos todos a 1 ou seja off.
    out PORTC, r30
    ldi LigarOuDesligar, 1 ; Atribui 1 ao R20 para sinalizar que se encontra OFF o display.
    dec cnt_Wait         ; Decrementa o contador.
    breq DarSetFlag      ; Se blinkou as 6 vezes vai para a proxima rotina.
    rjmp end

DarSetFlag:
    out SREG, temp       
    set                  ; Seta a flag
    pop temp             ; Restaura o registo
    reti                 ; Retorna da rotina de interrupção.


atuarEstado3:
    call LigarDisplay    ; Coloca o numero final no display.
end:
    out SREG, temp       ; Restaura o registo da flag.
    pop temp             ; Restaura o registo.
    reti                 ; Retorna da interrupção.


LigarDisplay:
    push numero          
    ldi zl, low(tabela << 1) 
    ldi zh, high(tabela << 1)
    add zl, numero        
    brcc salto           
    inc zh               
salto:
    lpm numero, z         
    out PORTC, numero     
    pop numero         ; Limpa o Registo.
    ret					; Retorna


TestarTransicao:
    call delay           ; Espera 1ms para nao ser automatico o stop.
    sbis pind, stop      ; Verifica que o stop esta pressionado.
    rjmp TestarTransicao ; Loopa ate o stop ser pressionado denovo
    ret                  ; Retorna para onde foi chamado.

;Delay de 1ms
delay:
    push r22
    push r23
    push r24

    ldi r24, 53
ciclo0: ldi r23, 20
ciclo1: ldi r22, 4
ciclo2: dec r22
        brne ciclo2
        dec r23
        brne ciclo1
        dec r24
        brne ciclo0

    pop r24
    pop r23
    pop r22
    ret