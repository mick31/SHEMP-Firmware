/*
 * sensors.c
 *
 *  Created on: Jul 13, 2012
 *      Author: nabercro
 */

#include "sensors.h"

uint8_t sample_queued_sensors();


node_ref sensor_list;
int8_t end_of_sequence;
#define IDLE 0
#define SAMPLING 1
#define DONE_SAMPLING 2


enum sampling_state {paused, ready, sampling} sampling_state;
sensor_ref ADC12_SENSORS[16];

volatile uint16_t * ADC12MEMX;

uint8_t init_adc() {
	// init ADC
	ADC12CTL0 = ADC12SHT0_0 + ADC12ON + ADC12MSC; //on and 16 clocks per sample
	ADC12CTL1 =  ADC12SHP + ADC12CONSEQ0 + ADC12SSEL_2 + ADC12DIV_1; //triggered by sample clock  //single conversion sequence //MCLK/2 is source
	ADC12CTL2 = ADC12RES_2; //12 bits
	// Note: ADC12DIV_1 was chosen because without it, the CLK is too fast.  Dividng by 2 fixes this

	sensor_list = 0;

	ADC12MCTL0 = ADC12INCH_12; //I+
	ADC12MCTL1 = ADC12INCH_14; //V+

	ADC12MCTL2 = ADC12INCH_7; //TEMP

	ADC12MCTL3 = ADC12INCH_3; //PORTA
	ADC12MCTL4 = ADC12INCH_2; //PORTB

	ADC12MCTL4 |= ADC12EOS;
	ADC12IE = 0x1<<4;

	ADC12MEMX = &ADC12MEM0;

	sampling_state = paused;

	return SUCCESS;
}

void start_sampling() {
	sampling_state = ready;
}
void stop_sampling() {
	sampling_state = paused;
}
uint8_t ready_to_sample() {
	if(sampling_state == ready) return TRUE;
	else return FALSE;
}



sensor_ref new_sensor(uint8_t type, uint8_t channel, time_ref period, uint16_t data_array_size) {
	sensor_ref s = 0;
	node_ref n = 0;
	time_ref tmp_time = 0;

	for(;;) {
		s = malloc(sizeof(struct sensor));
		if(!s) break;

		s->trigger_time = 0;
		s->period = 0;
		s->end_time = 0;
		s->type = type;
		s->adc_channel = channel;
		s->size = data_array_size;
		s->count = 0;
		s->data_ready = FALSE;
		s->on_full = 0;
		s->enabled = FALSE;
		s->delete_this = 0;
		s->enable_this = 0;
		s->disable_this = 0;

		s->write_array = malloc(data_array_size * sizeof(uint16_t));
		if(!s->write_array) break;

		s->data_array = malloc(data_array_size * sizeof(uint16_t));
		if(!s->data_array) break;

		s->old_array = malloc(data_array_size * sizeof(uint16_t));
		if(!s->old_array) break;

		tmp_time = time_copy(period);
		if(!tmp_time) break;
		s->period = tmp_time;

		tmp_time = time_copy(period);
		time_ref time = global_time();
		add_time_to_time(tmp_time, time);
		if(!tmp_time) break;
		s->trigger_time = tmp_time;

		tmp_time = new_time();
		if(!tmp_time) break;
		s->end_time = tmp_time;

		// Sensor is created, now add it to the sensor_list
		// We don't include the delete function because that would create a loop in sensor_delete
		n = new_node(s, 0);
		if(!n) break;

		if(sensor_list) node_append(sensor_list, n);
		else sensor_list = n;

		return s;
	}

	// We don't need to free the tmp_time, because if it was allocated, then
	// it is assigned to one of the sensors pointers, so it will be freed by sensor_delete

	sensor_delete(&s);

	return FAILURE;
}


uint8_t sensor_clear_state(sensor_ref s) {
	if(!s) return FAILURE;
	s->count = 0;
	s->data_ready = 0;
	time_clear(s->trigger_time);
	return SUCCESS;
}


sensor_ref new_one_time_sensor(uint8_t type, uint8_t channel, time_ref period, uint16_t data_array_size) {
	sensor_ref s = 0;
	node_ref n = 0;
	time_ref tmp_time = 0;

	action_ref turn_off_action = 0;
	node_ref sensor_to_turn_off = 0;
	for(;;) {
		s = malloc(sizeof(struct sensor));
		if(!s) break;

		s->trigger_time = 0;
		s->period = 0;
		s->end_time = 0;
		s->type = type;
		s->adc_channel = channel;
		s->size = data_array_size;
		s->count = 0;
		s->data_ready = FALSE;
		s->on_full = 0;
		s->enabled = FALSE;
		s->delete_this = 0;
		s->enable_this = 0;
		s->disable_this = 0;

		s->write_array = malloc(data_array_size * sizeof(uint16_t));
		if(!s->write_array) break;

		//s->data_array = malloc(data_array_size * sizeof(uint16_t));
		//if(!s->data_array) break;

		//s->old_array = malloc(data_array_size * sizeof(uint16_t));
		//if(!s->old_array) break;

		tmp_time = time_copy(period);
		if(!tmp_time) break;
		s->period = tmp_time;

		tmp_time = time_copy(period);
		time_ref time = global_time();
		add_time_to_time(tmp_time, time);
		if(!tmp_time) break;
		s->trigger_time = tmp_time;

		tmp_time = new_time();
		if(!tmp_time) break;
		s->end_time = tmp_time;

		// Sensor is created, now add it to the sensor_list
		// We don't include the delete function because that would create a loop in sensor_delete
		n = new_node(s, 0);
		if(!n) break;

		if(sensor_list) node_append(sensor_list, n);
		else sensor_list = n;


		// ALL THE OTHER STUFF!


		// Since we dont need the second array, lets just remove it.
		//free(s->data_array);
		s->data_array = s->write_array;

		//free(s->old_array);
		s->old_array = s->write_array;


		// Now we have to add the single use functionality
		// Create the action
		turn_off_action = new_action();
		if(!turn_off_action) break;

		// Set the functions
		action_set_func(turn_off_action, &disable_sensor_args);
		if(s->on_full) action_append_action(s->on_full, turn_off_action);
		else s->on_full = turn_off_action;

		// Create the argument
		sensor_to_turn_off = new_node(s, 0);
		if(!sensor_to_turn_off) break;
		action_set_args(turn_off_action, sensor_to_turn_off);
		return s;
	}

	sensor_delete(&s);
	return FAILURE;
}


uint8_t sensor_add_action_on_data_full(sensor_ref s, action_ref a) {
	if(!s) return FAILURE;
	if(!a) return FAILURE;

	if(s->on_full) action_append_action(s->on_full, a);
	else s->on_full = a;

	return SUCCESS;
}


uint8_t sensor_delete(sensor_ref * sensor_ptr_ptr) {
	if (!sensor_ptr_ptr) return FAILURE; //pointer is null
	sensor_ref s = *sensor_ptr_ptr;
	if (!(s)) return SUCCESS; //pointer is to a null pointer, sensor is already deleted?
	//if (s->data_count > 0) return FAILURE; //cannot delete it yet, data must be sent

	node_ref cur_node = 0;
	node_ref prev_node = 0;
	// If there are sensors in the sensor_list, perhaps this one is, so we must remove it
	if(sensor_list) {
		cur_node = sensor_list;
		// So we search through each entry in the list
		while(cur_node) {
			if(node_get_val(cur_node) == s) {
				// Found it, must remove it.
				if (cur_node == sensor_list) {
					// If it is the first, just move the pointer to the next
					sensor_list = node_get_next(cur_node);
					// And delete this one
					node_delete(&cur_node);
				} else {
					// Else we have to go back one step, to remove the next.
					node_delete_next(prev_node);
				}
				break;
			}
			prev_node = cur_node;
			cur_node = node_get_next(cur_node);
		}
	}

	//Delete the actions
	action_delete_list(&s->on_full);

	// Free the arrays
	free(s->write_array);

	if(s->data_array != s->write_array) {
		// Had to add this if statement, because the "single use" sensors share the same data and write array
		free(s->data_array);
		free(s->old_array);
	}

	// Free itself
	free(s);

	(*sensor_ptr_ptr) = 0;

	return SUCCESS;
}

uint8_t enable_sensor_args(node_ref args) {
	if (!args) return FAILURE;
	node_ref cur_node = args;
	sensor_ref s = 0;
	while(cur_node) {
		s = (sensor_ref)node_get_val(args);
		enable_sensor(s);
		cur_node = node_get_next(cur_node);
	}
	return SUCCESS;
}

uint8_t enable_sensor(sensor_ref s) {
	if(!s) return FAILURE;
	if(s->enabled == TRUE) return SUCCESS;

	time_set_to_sum(s->trigger_time, s->period, global_time());

	s->enabled = TRUE;
	s->count = 0;
	return SUCCESS;
}

uint8_t disable_sensor(sensor_ref s) {
	if(!s) return FAILURE;
	s->enabled = FALSE;
	return SUCCESS;
}


uint8_t disable_sensor_args(node_ref args) {
	if (!args) return FAILURE;
	node_ref cur_node = args;
	sensor_ref s = 0;
	while(cur_node) {
		s = (sensor_ref)node_get_val(args);
		disable_sensor(s);
		cur_node = node_get_next(cur_node);
	}
	return SUCCESS;
}



uint8_t sensor_set_delete_func(sensor_ref s, uint8_t (*delete_func)()) {
	if(!s) return FAILURE;
	if(!delete_func) return FAILURE;

	s->delete_this = delete_func;
	return SUCCESS;
}

uint8_t sensor_set_disable_func(sensor_ref s, uint8_t (*disable_func)()) {
	if(!s) return FAILURE;
	if(!disable_func) return FAILURE;

	s->disable_this = disable_func;
	return SUCCESS;
}
uint8_t sensor_set_enable_func(sensor_ref s, uint8_t (*enable_func)()) {
	if(!s) return FAILURE;
	if(!enable_func) return FAILURE;

	s->enable_this = enable_func;
	return SUCCESS;
}
uint8_t sensor_delete_this(sensor_ref s) {
	if(!s) return FAILURE;
	if(!s->delete_this) return FAILURE;
	s->delete_this();
	return SUCCESS;
}
uint8_t sensor_enable_this(sensor_ref s) {
	if(!s) return FAILURE;
	if(!s->enable_this) return FAILURE;
	s->enable_this();
	return SUCCESS;
}
uint8_t sensor_disable_this(sensor_ref s) {
	if(!s) return FAILURE;
	if(!s->disable_this) return FAILURE;
	s->disable_this();
	return SUCCESS;
}





int16_t * sensor_get_data_array(sensor_ref s) {
	if(!s) return FAILURE;
	return s->data_array;
}
int16_t * sensor_get_old_array(sensor_ref s) {
	if(!s) return FAILURE;
	return s->old_array;
}

uint8_t sensor_get_type(sensor_ref s) {
	if(!s) return FAILURE;
	return s->type;
}
uint16_t sensor_get_size(sensor_ref s) {
	if(!s) return FAILURE;
	return s->size;
}
time_ref sensor_get_period(sensor_ref s) {
	if(!s) return FAILURE;
	return s->period;
}
time_ref sensor_get_end_time(sensor_ref s) {
	if(!s) return FAILURE;
	return s->end_time;
}
time_ref sensor_get_trigger_time(sensor_ref s) {
	if(!s) return FAILURE;
	return s->trigger_time;
}
uint8_t sensor_get_loc(sensor_ref s) {
	if(!s) return FAILURE;
	return channel_to_loc(s->adc_channel);
}

uint8_t loc_to_channel(uint8_t loc) {
	if(loc == 'A') {
		return PORT_A_ADC;
	}
	if(loc == 'B') {
		return PORT_B_ADC;
	}
	return 0; // unknown?
}

uint8_t channel_to_loc(uint8_t channel) {
	if(channel == PORT_A_ADC) {
		return 'A';
	}
	if(channel == PORT_B_ADC) {
		return 'B';
	}
	return 'I'; // Internal
}




uint8_t sample_enabled_sensors() {
	if(!ready_to_sample()) return FAILURE;

	node_ref cur_node = sensor_list;
	sensor_ref cur_sensor = 0;

	int8_t queue_counter = 0;

	while(cur_node && (queue_counter <= 15)) {
		// For each sensor
		cur_sensor = (sensor_ref)node_get_val(cur_node);

		if(cur_sensor->enabled) {

			if(check_time(cur_sensor->period, cur_sensor->trigger_time)) {
				// The sensor should be sampled
				ADC12_SENSORS[queue_counter] = cur_sensor;
				queue_counter++;
			}
		}
		cur_node = node_get_next(cur_node);
	}

	if(queue_counter > 0) {
		ADC12_SENSORS[queue_counter] = 0; //nullify the next one
		sample_queued_sensors();
	}

	return SUCCESS;
}



uint8_t sample_queued_sensors() {
	sampling_state = sampling;

	ADC12CTL0 |= ADC12ENC + ADC12SC; //enable and start

	return SUCCESS;
}

uint8_t store_data_point(sensor_ref s, uint16_t measurement) {
	if (!s) return FAILURE;

	// Store the point
	s->write_array[s->count] = measurement;
	// Increment the counter
	s->count++;

	// Data is full
	if(s->count == s->size) {
		// Reset count
		s->count = 0;

		// Store time
		time_set_current(s->end_time);

		// Swap arrays
		int16_t * tmp = s->data_array;
		s->data_array = s->write_array;
		s->write_array = s->old_array;
		s->old_array = tmp;

		// Data is ready
		s->data_ready = TRUE;
	}
	return SUCCESS;
}


uint8_t handle_adc_interrupt() {

	uint8_t queue_itor = 0;
	sensor_ref cur_sensor;

	while(1) {
		cur_sensor = ADC12_SENSORS[queue_itor];
		if(cur_sensor) {
			store_data_point(cur_sensor, ADC12MEMX[cur_sensor->adc_channel]);
			queue_itor++;
		} else {
			break;
		}
	}


	sampling_state = ready;
	//reset it
	ADC12IFG = 0;

	return SUCCESS;
}

uint8_t handle_full_sensors() {
	//d//debug_push_int(d_handle_full_sensors);

	node_ref cur_node = sensor_list;
	sensor_ref cur_sensor = 0;

	while(cur_node) {
		cur_sensor = (sensor_ref)node_get_val(cur_node);

		if(cur_sensor->data_ready) {
			do_actions(cur_sensor->on_full);
			cur_sensor->data_ready = FALSE;
		}
		cur_node = node_get_next(cur_node);
	}

	//d//debug_pop_int();
	return SUCCESS;
}


