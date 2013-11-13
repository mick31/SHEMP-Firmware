/*
 * auxlib.c
 *
 * Auxliary Library
 */

#include "auxlib.h"

void disable_watchdog() {
	 WDTCTL = WDTPW + WDTHOLD;
	 //watchdog control = wd password + wd hold
}



void write_int_to_string(uint8_t * string, int16_t number) {
	/*
	 * Takes a pointer to a string, and an integer
	 * Writes the integer to the string
	 * example:
	 * >> write_int_to_string(string, 5);
	 * "     5"
	 * >> write_int_to_string(string, -100);
	 * "-  100"
	 */
	if (number < 0) {
		string[0] = '-';
		number = number * (-1);
	}
	else string[0] = ' ';

	if(number > 10000) string[1] = '0' + number / 10000;
	else string[1] = ' ';

	if(number > 1000) string[2] = '0' + (number % 10000) / 1000;
	else string[2] = ' ';

	if(number > 100) string[3] = '0' + (number % 1000) / 100;
	else string[3] = ' ';

	if(number > 10) string[4] = '0' + (number % 100) / 10;
	else string[4] = ' ';

	string[5] = '0' + (number % 10);
}



void write_2_bytes_to_string(uint8_t * string, uint16_t number) {
	// Writes the number in little-endian byte-wise to the string
	// >> write_2_bytes_to_string(string, 13938)
	// "r6"
	string[0] = number & 0xFF;
	string[1] = (number>>8);
}

void write_4_bytes_to_string(uint8_t * string, uint32_t number) {
	// Same as above, but just more bytes
	string[3] = number & 0xFF;
	string[2] = ((uint32_t)number>>(uint32_t)8) & 0xFF;
	string[1] = ((uint32_t)number>>(uint32_t)16) & 0xFF;
	string[0] = ((uint32_t)number>>(uint32_t)24) & 0xFF;
}


uint32_t read_number_from_string(uint8_t * string, uint16_t length) {
	// Reads a string, and returns an integer
	uint32_t result = 0;
	uint16_t index = 0;

	while(index < length) {
		result = multiply(result, 10);
		result += string[index] - '0';
		index++;
	}

	return result;
}




int32_t recursive_avg(int32_t * start, int32_t * end) {
	// Averaging function, divide and conquer recursion
	// This has the problem of losing information in each division
	if (end-start == 0) return (*end);
	else {
		uint32_t a = recursive_avg(start, start+(end-start)/2);
		uint32_t b = recursive_avg(start+(end-start)/2+1, end);
		return ((a+b)/2);
	}
}

int32_t average(int32_t * start, int32_t * end) {
	// Averaging function, simply sums them all up and divides them
	// This has the problem that it could overflwo
	int32_t result = 0;
	int32_t * cur = start;

	int32_t sum = 0;
	uint16_t count = 0;

	while (cur <= end) {
		sum += *cur;
		count++;
		cur = &cur[1];
	}

	result = sum / count;
	return result;
}

int32_t multiply(int32_t a, int32_t b) {
	// Multiplies 2 numbers by using the hardware multiplier
	int32_t result;

	MPYS = a;
	OP2 = b;
	result = RES0;
	result |= (uint32_t)RES1<<16;

	return result;
}



uint8_t string_starts_with(uint8_t * token, uint8_t * string) {
	// Like strcmp, but only has to to start with the token, not
	// be equal to it
	uint16_t length = strlen(token);
	uint16_t counter = 0;

	while(counter < length) {
		if(string[counter] != token[counter]) return FALSE;
		counter++;
	}
	return TRUE;
}


uint8_t is_valid_string(uint8_t * string) {
	// This just checks if a user's WiFi SSID is valid,
	// this could be much better
	if(!string) return FALSE;

	uint8_t char_count = 0;


	//TODO Must check for all evil characters
	while(*string) {
		if(*string == '\n') return FALSE;
		if(*string == '+') return FALSE;

		char_count++;
		string++;
	}

	if(char_count == 0) return FALSE;

	return TRUE;
}


uint16_t number_of_characters_in_string(uint8_t * string, uint8_t character) {
	uint16_t count = 0;
	while(*string) {
		if(*string == character) count++;
		string++;
	}
	return count;
}


