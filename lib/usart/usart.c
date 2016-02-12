#ifndef USART_C_LHI7YWEJ
#define USART_C_LHI7YWEJ

#include <avr/io.h>
#include <avr/interrupt.h>

/*
	USART handler functions.
	
	char buffer[BUFFER_LEN];
	uint8_t i = 0;
	
	ISR(USART1_RX_vect) {
		uint8_t c;

		c = UDR1;
		buffer[i] = c;
		i++;
	}
*/

// Initialize the USART
void USARTinit(uint32_t baud) {
	cli();
	UBRR1 = (F_CPU / 4 / baud - 1) / 2;
	UCSR1A = (1<<U2X1);							//Double speed operation
	UCSR1B = (1<<RXEN1) | (1<<RXCIE1);		//Enable only RX and it's interrupt
	UCSR1C = (1<<UCSZ11) | (1<<UCSZ10);		//8 bit data
	sei();
}

#endif /* end of include guard: USART_C_LHI7YWEJ */