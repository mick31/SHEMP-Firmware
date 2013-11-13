/*
 * uart.c
 *
 *  Created on: Jul 5, 2012
 *      Author: nabercro
 */


#include "uart.h"

void init_uart() {
	//d//debug_push(d_init_uart);
	// 9.5 = RX
	// 9.4 = TX
	 P9SEL = 0x30;

	UCA2CTL1 |= UCSWRST;                      // **Put state machine in reset**
	UCA2CTL1 |= UCSSEL_2;                     // SMCLK


	//UCA2BR0 = 204;                            // 23.5 mHz 115200
	//UCA2BR0 = 193;							// 22.2 mHz 115200
	//UCA2BR0 = 179;							// 20.6 mHz 115200
	UCA2BR0 = 164;							// 18 mHz 115200
	//UCA2BR0 = 96;								// 11 mHz 115200

	UCA2BR1 = 0;

	UCA2CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	UCA2IE |= UCRXIE;                         // Enable USCI_A2 RX interrupt
	//d//debug_pop();
}

void uart_send_string(uint8_t * string) {
	uint16_t length = strlen(string);
	uart_send_array(string, length);
}


void uart_send_array(uint8_t * array, uint16_t length) {
	uint16_t counter = 0;
	while(counter < length) {
	    while (!(UCA2IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
		UCA2TXBUF = *array++;                  // TX -> RXed character
		counter++;
	}
}


#pragma vector=USCI_A2_VECTOR
__interrupt void USCI_A2_ISR(void) {
	switch(__even_in_range(UCA2IV,4)) {
	case 0:break;                             // Vector 0 - no interrupt
	case 2: store_uart_input(UCA2RXBUF);                                // Vector 2 - RXIFG
		break;
	case 4:break;                             // Vector 4 - TXIFG
	default: break;
	}

}

