#ifndef __AVR_ATmega128A__
    #define __AVR_ATmega128A__
#endif

#define F_CPU 11059200UL

#define BAUD 9600
#define HC05_UBRR ((F_CPU/16/BAUD)-1)

#define TX_BUFFER_SIZE 32
#define COMMAND_PARAM_BUFFER_SIZE 2

#define COMMAND_READ_VOLTAGE 0x01

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

struct TXBuffer {
    volatile uint8_t buffer[TX_BUFFER_SIZE];
    volatile uint8_t posRead;
    volatile uint8_t posWrite;
    volatile bool isReady; // true if the buffer is ready to transmit

    TXBuffer():posRead(0),posWrite(0),isReady(false){}
} tx0Buffer;

volatile uint8_t command = 0;
volatile uint8_t commandParam[COMMAND_PARAM_BUFFER_SIZE];
volatile uint8_t commandParamWPos = 0; // Write position for command parameter

void handleCommands();
void handleReadVoltageCommand();
void handleUnknownCommand();

void append2TxBuffer(uint8_t);
void append2TxBufferAsStr(uint16_t);
void transmitFromTxBuffer();


/**
 * Init USART0 To cummunicate with the HC-05 bluetooth module (Page 247)
 */
void setupUSART0() {
    // Set BAUD rate (Page 254)
    UBRR0H = (HC05_UBRR>>8);
    UBRR0L = HC05_UBRR;

    // Set TXD0 as output
    DDRE |= (1<<DDE1);

    // Enable receiver, transmitter and interrupts
    // RXCIE | RX Complete Interrupt Enable
    // TXCIE | TX Complete Interrupt Enable
    // RXEN  | Receiver Enable
    // TXEN  | Transmitter Enable
    UCSR0B |= (1<<RXCIE0)|(1<<TXCIE0)|(1<<RXEN0)|(1<<TXEN0);

    // Set frame format
    // UCSZ[2:0] - 0b011 | 8-bit Character Size
    UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
}

void setup() {
    cli();

    setupUSART0();

    DDRB |= (1 << DDB6);  // LED on / off port

    sei();
}

int main(void) {
    setup();

    while(1) {
        handleCommands();

        // UART0 | Start / Init transmit data if available
        if (UCSR0A & (1<<UDRE0)) { // If buffer is empty
            if (tx0Buffer.isReady) {
                tx0Buffer.isReady = false;
                transmitFromTxBuffer();
            }
        }

        // Reset tx buffer if transmission is completed
        if (tx0Buffer.posRead != 0 && tx0Buffer.posRead >= tx0Buffer.posWrite) {
            tx0Buffer.posRead = 0;
            tx0Buffer.posWrite = 0;
        }

        // for (uint8_t i=0; i<3; i++) {
        //     _delay_ms(100);
        //     PORTB |= (1 << PB6);
        //     _delay_ms(100);
        //     PORTB &= ~(1 << PB6);
        // }

        // _delay_ms(2000);
    }

    return 0;
}

/**
 * Handle commands incoming via UART if available
 */
void handleCommands() {
    switch(command) {
        case 0x00:
            // Skip if no command is available
        break;

        case COMMAND_READ_VOLTAGE:
            handleReadVoltageCommand();
        break;

        default:
            handleUnknownCommand();
    }
}

/**
 * 
 */
void handleReadVoltageCommand() {
    append2TxBuffer('r');
    append2TxBuffer('e');
    append2TxBuffer('a');
    append2TxBuffer('d');
    append2TxBuffer(' ');
    append2TxBuffer('V');
    append2TxBuffer('.');
    append2TxBuffer(':');
    append2TxBuffer('\n');
    tx0Buffer.isReady = true; // Ready to print out

    // Reset command
    command = 0;
    commandParamWPos = 0;
}

/**
 * 
 */
void handleUnknownCommand() {
    append2TxBuffer('u');
    append2TxBuffer('n');
    append2TxBuffer('k');
    append2TxBuffer('n');
    append2TxBuffer('o');
    append2TxBuffer('w');
    append2TxBuffer('n');
    append2TxBuffer(':');
    append2TxBuffer('[');
    append2TxBuffer(command);
    append2TxBuffer(']');
    append2TxBuffer('\n');
    tx0Buffer.isReady = true; // Ready to print out

    // Reset command
    command = 0;
    commandParamWPos = 0;
}

void append2TxBuffer(uint8_t data) {
    // If tx buffer is full, data will be ignored
    if (tx0Buffer.posWrite >= TX_BUFFER_SIZE) {
        return;
    }

    tx0Buffer.buffer[tx0Buffer.posWrite++] = data;
}

/**
 * Manual converter from uint16_t to char array and add this to the tx buffer.
 */
void append2TxBufferAsStr(uint16_t number) {
    uint16_t devider = 10000;

    // find the right size of the devider
    while ((number / devider) == 0 && devider >=  10) {
        devider /= 10;
    }

    uint16_t tempNumber = number;
    uint8_t numberPos = 0;
    for (; devider > 0; devider /= 10) {
        numberPos = tempNumber / devider;
        append2TxBuffer('0'+numberPos);
        tempNumber -= numberPos * devider;
    }
}

/**
 * UART
 * Transmitting the next available byte via UART
 */
void transmitFromTxBuffer() {
    // If tx read pos is at the end of the buffer, nothing there to transmit
    if (tx0Buffer.posRead >= TX_BUFFER_SIZE) {
        return;
    }

    // Transmit if we have still data in buffer
    if (tx0Buffer.posRead < tx0Buffer.posWrite) {
        UDR0 = tx0Buffer.buffer[tx0Buffer.posRead++];
    }
}

/**
 * USART0, Rx Complete
 */
ISR(USART0_RX_vect) {
    if (command == 0) {
        command = UDR0;
    }

    // Read command parameter
    else {
        // Read command parameter only if buffer has free space, ignore otherwise
        if (commandParamWPos < COMMAND_PARAM_BUFFER_SIZE) {
            commandParam[commandParamWPos++] = UDR0;
        }
    }
}

/**
 * USART0, Tx Complete
 */
ISR(USART0_TX_vect) {
    transmitFromTxBuffer();
}