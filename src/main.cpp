#ifndef __AVR_ATmega128A__
    #define __AVR_ATmega128A__
#endif

#define F_CPU 11059200UL

#include <avr/io.h>
#include <util/delay.h>

void setup() {
    DDRB |= (1 << DDB6);  // LED on / off port
}

int main(void) {
    setup();

    while(1) {
        for (uint8_t i=0; i<3; i++) {
            _delay_ms(100);
            PORTB |= (1 << PB6);
            _delay_ms(100);
            PORTB &= ~(1 << PB6);
        }

        _delay_ms(2000);
    }

    return 0;
}