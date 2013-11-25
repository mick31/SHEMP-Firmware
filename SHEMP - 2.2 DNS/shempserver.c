/*
 * shempserver.c
 *
 *  Created on: Jul 25, 2012
 *      Author: nabercro
 */

#include "shempserver.h"


const uint8_t SERIAL_NUMBER[] = "000007";


uint8_t output_buffer[OUTPUT_BUFFER_SIZE];
uint16_t write_ptr;
uint16_t read_ptr;
uint8_t reset_output_flag;
uint8_t hold_transmit_flag;


void init_transmits() {
	//d//debug_push(d_init_transmits);
	write_ptr = 0;
	read_ptr = 0;
	reset_output_flag = FALSE;

	hold_transmit_flag = FALSE;
	//d//debug_pop();
}


void reset_output_buffer() {
	reset_output_flag = TRUE;
}


// Size Calculation
// Lxx	3 for length
// Tx	2 for type
// lx	2 for loc
// t+14	15 for time
// P+14	15 for period
// Cxx		3 for count
// Dxx...	1 + count*2 for data
// X		1 for end
#define HEADER_SIZE 42




uint8_t encode_data_and_old_data_for_transmit(node_ref args) {
	if(reset_output_flag) {
		write_ptr = 0;
		read_ptr = 0;
		reset_output_flag = FALSE;
	}

	if(!args) return FAILURE;
	sensor_ref s = (sensor_ref) node_get_val(args);

	if(!s) return FAILURE;

	int16_t * data_array = sensor_get_data_array(s);
	int16_t * old_array = sensor_get_old_array(s);
	time_ref end_time = sensor_get_end_time(s);
	time_ref period = sensor_get_period(s);
	uint16_t size = sensor_get_size(s);

	if(!data_array) return FAILURE;
	if(!old_array) return FAILURE;
	if(!end_time) return FAILURE;
	if(!period) return FAILURE;
	if(!size) return FAILURE;

	uint16_t cur_size = 2*size*sizeof(uint16_t) + HEADER_SIZE;
	if(cur_size > OUTPUT_BUFFER_SIZE) return FAILURE; // Not ever going to fit

	uint16_t temp_ptr = write_ptr;
	if(temp_ptr + cur_size > OUTPUT_BUFFER_SIZE) {
		// Cannot fit it
		temp_ptr = 0;
		read_ptr = 0;
		write_ptr = 0;
		//return FAILURE;
		//TODO Handle this case better
	}

	output_buffer[temp_ptr++] = 'L';
	write_2_bytes_to_string(&output_buffer[temp_ptr], cur_size);
	temp_ptr += 2;

	output_buffer[temp_ptr++] = 'T';
	output_buffer[temp_ptr++] = sensor_get_type(s);

	output_buffer[temp_ptr++] = 'l';
	output_buffer[temp_ptr++] = sensor_get_loc(s);

	output_buffer[temp_ptr++] = 't'; //time
	write_time_to_string(&output_buffer[temp_ptr], end_time);
	temp_ptr += STRING_TO_TIME_LENGTH;

	output_buffer[temp_ptr++] = 'P'; //period
	write_time_to_string(&output_buffer[temp_ptr], period);
	temp_ptr += STRING_TO_TIME_LENGTH;

	output_buffer[temp_ptr++] = 'C';
	write_2_bytes_to_string(&output_buffer[temp_ptr], (size*2));
	temp_ptr += 2;

	output_buffer[temp_ptr++] = 'D';
	//data
	memcpy(&output_buffer[temp_ptr], old_array, size*sizeof(uint16_t));
	temp_ptr += size*sizeof(uint16_t);
	memcpy(&output_buffer[temp_ptr], data_array, size*sizeof(uint16_t));
	temp_ptr += size*sizeof(uint16_t);

	output_buffer[temp_ptr++] = 'X'; // end

	write_ptr = temp_ptr;
	return SUCCESS;
}


uint8_t encode_data_for_transmit(node_ref args) {
	if(reset_output_flag) {
		write_ptr = 0;
		read_ptr = 0;
		reset_output_flag = FALSE;
	}

	uint8_t ret = FAILURE;

	for(;;) {
		if(!args) break;
		sensor_ref s = (sensor_ref)node_get_val(args);

		if(!s) break;

		uint16_t cur_size = 0;
		int16_t * array = sensor_get_data_array(s);
		uint16_t temp_ptr = write_ptr;

		cur_size = sensor_get_size(s)*sizeof(uint16_t) + HEADER_SIZE;

		if(cur_size > OUTPUT_BUFFER_SIZE) return FAILURE; // Not ever going to fit

		if(temp_ptr + cur_size > OUTPUT_BUFFER_SIZE) {
			// Cannot fit it
			temp_ptr = 0;
			read_ptr = 0;
			write_ptr = 0;
			//return FAILURE;
			//TODO Handle this case better
		}

		output_buffer[temp_ptr++] = 'L';
		write_2_bytes_to_string(&output_buffer[temp_ptr], cur_size);
		temp_ptr += 2;

		output_buffer[temp_ptr++] = 'T';
		output_buffer[temp_ptr++] = sensor_get_type(s);

		output_buffer[temp_ptr++] = 'l';
		output_buffer[temp_ptr++] = sensor_get_loc(s);

		output_buffer[temp_ptr++] = 't'; //time
		write_time_to_string(&output_buffer[temp_ptr], sensor_get_end_time(s));
		temp_ptr += STRING_TO_TIME_LENGTH;

		output_buffer[temp_ptr++] = 'P'; //period
		write_time_to_string(&output_buffer[temp_ptr], sensor_get_period(s));
		temp_ptr += STRING_TO_TIME_LENGTH;

		output_buffer[temp_ptr++] = 'C';
		write_2_bytes_to_string(&output_buffer[temp_ptr], sensor_get_size(s));
		temp_ptr += 2;



		output_buffer[temp_ptr++] = 'D';
		//data
		memcpy(&output_buffer[temp_ptr], array, sensor_get_size(s)*sizeof(uint16_t));
		temp_ptr += sensor_get_size(s) * 2;

		output_buffer[temp_ptr++] = 'X'; // end


		write_ptr = temp_ptr;

		ret = SUCCESS;
		break;
	}
	//d//debug_pop();
	return ret;
}


uint8_t have_data_to_transmit() {
	if(reset_output_flag) return FALSE;
	if (read_ptr != write_ptr) return TRUE;
	return FALSE;
}

void hold_transmits() {
	hold_transmit_flag = TRUE;
}

void continue_transmits() {
	hold_transmit_flag = FALSE;
}

uint8_t transmit_data() {
	if(!have_data_to_transmit()) {
		return SUCCESS;
	}
	if(hold_transmit_flag) return SUCCESS;

	//Hold onto it
	uint16_t end_ptr = write_ptr;

	reset_ack();

	uart_send_array(&output_buffer[read_ptr], end_ptr-read_ptr);

	if(wait_for(&have_ack, 500)) {
		read_ptr = end_ptr;
		return SUCCESS;
	}
	return FAILURE;
}

uint8_t header[] = "L00THSxxxxxxt00000000000000X";

#define HEADER_LENGTH 28 // 0x1C00
#define HEADER_LENGTH_PTR 1

#define HEADER_TIME_PTR 13

#define HEADER_SERIAL_PTR 6
#define HEADER_SERIAL_LENGTH 6

uint8_t transmit_header() {
	// Set up header
	write_2_bytes_to_string(&header[HEADER_LENGTH_PTR], HEADER_LENGTH);
	memcpy(&header[HEADER_SERIAL_PTR], SERIAL_NUMBER, HEADER_SERIAL_LENGTH);

	reset_ack();
	while(!have_ack()) {
		write_time_to_string(&header[HEADER_TIME_PTR], global_time());
		uart_send_array((uint8_t *)header, HEADER_LENGTH);
		wait_for(&have_ack, 500);
	}

	return SUCCESS;
}


action_ref new_transmit_action(sensor_ref s) {
	//d//debug_push(d_new_transmit_action);
	action_ref tx = new_action();
	action_set_func(tx, &encode_data_for_transmit);
	node_ref arg1 = new_node(s, 0);
	action_set_args(tx, arg1);

	//d//debug_pop();
	return tx;
}
