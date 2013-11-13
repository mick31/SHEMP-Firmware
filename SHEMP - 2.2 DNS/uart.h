/*
 * uart.h
 *
 *  Created on: Jul 5, 2012
 *      Author: nabercro
 */

#ifndef UART_H_
#define UART_H_

#include <msp430.h>
#include <inttypes.h>
#include <string.h>

/* FUNCTIONS */
void init_uart();
void uart_send_string(uint8_t * string);
void uart_send_array(uint8_t * array, uint16_t length);

extern void store_uart_input(uint8_t input);

#endif /* UART_H_ */
