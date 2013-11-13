/*
 * auxlib.h
 * Author: Nathan Abercrombie
 *
 * This is an auxiliary library for functions I needed
 */

#ifndef AUXLIB_H_
#define AUXLIB_H_

#include <inttypes.h>
#include <msp430.h>
#include <stdio.h>
#include "bool.h"

void disable_watchdog();

void write_int_to_string(uint8_t * string, int16_t number);
void write_2_bytes_to_string(uint8_t * string, uint16_t number);
void write_4_bytes_to_string(uint8_t * string, uint32_t number);

uint32_t read_number_from_string(uint8_t * string, uint16_t length);

int32_t recursive_avg(int32_t * start, int32_t * end);
int32_t average(int32_t * start, int32_t * end);

int32_t multiply(int32_t a, int32_t b);


uint8_t string_starts_with(uint8_t * token, uint8_t * string);

uint8_t is_valid_string(uint8_t * string);

uint16_t number_of_characters_in_string(uint8_t * string, uint8_t character);

#endif /* AUXLIB_H_ */
