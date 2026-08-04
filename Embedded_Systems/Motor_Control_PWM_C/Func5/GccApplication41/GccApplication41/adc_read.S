#include <avr/io.h>

.global ler_valor_asm

ler_valor_asm:
    lds r18, ADCSRA
    ori r18, (1 << ADSC)
    sts ADCSRA, r18

wait_conversion:
    lds r18, ADCSRA
    sbrc r18, ADSC
    rjmp wait_conversion
    lds r24, ADCH
    ret
