/*
 * custom_sensors.c
 *
 *  Created on: Jul 23, 2012
 *      Author: nabercro
 */
//TODO REMOVE DEBUG INCLUDES
#include <msp430.h>

#include "wattage_sensor.h"

#define VOL_CUR_PERIOD (12000/2400)
#define VOL_CUR_SIZE	(12000/(VOL_CUR_PERIOD*60) * 5) //5 60hz cycles
#define WATTAGE_SIZE	(12) //1 second

uint8_t calculate_wattage();

sensor_ref current_sensor;
sensor_ref voltage_sensor;
sensor_ref wattage_sensor;

uint8_t current_is_ready;
uint8_t voltage_is_ready;

uint8_t should_transmit_current;
uint8_t should_transmit_voltage;
uint8_t should_transmit_wattage;

uint8_t voltage_is_interesting;
uint8_t send_next_voltage;
uint8_t current_is_interesting;
uint8_t send_next_current;

int32_t power_sum;
int32_t power_counter;

int32_t power_average;
int32_t power_low_passed;

int32_t current_reference;
int32_t voltage_reference;

#define WATTAGE_THRESHOLD_BITS 4
int32_t new_voltage;
int32_t new_current;

int32_t stable_voltage;
int32_t stable_current;
int32_t stable_wattage;

uint16_t wattage_settling_counter;
uint16_t voltage_settling_counter;
uint16_t current_settling_counter;
#define SETTLING_TIME 12 //one second


void init_internal_wattage_sensor() {
	/* init some variables */
	voltage_sensor = 0;
	current_sensor = 0;
	wattage_sensor = 0;

	should_transmit_voltage = FALSE;
	should_transmit_current = FALSE;
	should_transmit_wattage = FALSE;

	voltage_is_interesting = FALSE;
	send_next_voltage = FALSE;
	current_is_interesting = FALSE;

	current_reference = 2036; // experimentally (close to half of 3.3 V)
	voltage_reference = 2036;

	stable_current = 0;
	stable_voltage = 0;
	stable_wattage = 0;

	voltage_is_ready = FALSE;
	current_is_ready = FALSE;

	power_average = 0;
	power_low_passed = 0;


	wattage_settling_counter = SETTLING_TIME;
	current_settling_counter = SETTLING_TIME;
	voltage_settling_counter = SETTLING_TIME;

}


uint8_t sync_wattage_sensors() {
	stop_sampling();

	sensor_clear_state(current_sensor);
	sensor_clear_state(voltage_sensor);
	sensor_clear_state(wattage_sensor);

	voltage_is_ready = FALSE;
	current_is_ready = FALSE;

	wattage_settling_counter = SETTLING_TIME;
	voltage_settling_counter = SETTLING_TIME;
	voltage_settling_counter = SETTLING_TIME;

	start_sampling();

	return SUCCESS;
}


uint8_t current_on_full(node_ref args) {

	int16_t * current_array = 0;
	int16_t current_value = 0;
	int16_t previous_current_value = 0;
	int16_t current_reading = 0;

	// For "interesting" detection
	new_current = 0;

	current_array = sensor_get_data_array(current_sensor);

	// With the data acquired, now we can loop through it and fix it for the reference
	int32_t current_sum = 0;
	uint16_t itor;
	for(itor = 0; itor < VOL_CUR_SIZE; itor++) {
		current_reading = current_array[itor];


		// Since it is a sine wave, it is centered at 0, so lets sum them up
		current_sum += current_reading;
	}
	int32_t current_average = (int32_t)current_sum/(int32_t)VOL_CUR_SIZE;

	// Could low pass it right here
	int32_t current_difference = (int32_t)current_average-(int32_t)current_reference;
	current_reference += (int32_t)current_difference>>(int32_t)1;


	for(itor = 0; itor < VOL_CUR_SIZE; itor++) {
		current_reading = current_array[itor];

		// Calculate Current
		// Current is the current reading minus its reference (centered at 3.3/2)
		current_value = (int16_t)(current_reading-(int32_t)current_reference);
		//current_value = current_reference;

		// Check our threshold.  If it is under 10, then it must be noise
		if(current_value <= 10 && current_value >= -10) { // TODO magic number
			if(previous_current_value == 0) {
				current_value = 0;
			} else {
				previous_current_value = 0;
			}
		} else {
			previous_current_value = current_value;
		}


		current_array[itor] = current_value;

		if(current_value > 0) {
			new_current = (int32_t)new_current + (int32_t)current_value;
		} else {
			new_current = (int32_t)new_current - (int32_t)current_value;
		}
	}

	if(current_settling_counter > 0) {
		current_settling_counter--;
		stable_current = new_current;
		current_is_ready = FALSE;
	} else {
		current_is_ready = TRUE;
	}


	if(should_transmit_current) {
		//detect if there is any changes
		uint8_t bit_itor = 0;
		uint32_t stable_msb_bit = (uint32_t)0x80000000; //just the top bit

		// Find the MSB of stable current
		for(bit_itor = 0; bit_itor < 32; bit_itor++) {
			if((uint32_t)stable_current & (uint32_t)stable_msb_bit) break;
			stable_msb_bit = (uint32_t)stable_msb_bit>>1;
		}

		// Find the difference in current levels
		uint32_t diff_current;
		if((uint32_t)stable_current > (uint32_t)new_current) diff_current = (uint32_t)stable_current - (uint32_t)new_current;
		else diff_current = (uint32_t)new_current - (uint32_t)stable_current;

		// Check the size of of diff_current vs the msb bit
		uint32_t threshold_msb_bit = (uint32_t)stable_msb_bit >>(uint32_t)WATTAGE_THRESHOLD_BITS;
		if((uint32_t)diff_current > (uint32_t)threshold_msb_bit) {
			current_is_interesting = TRUE;

			if(stable_current < 3000 && new_current < 3000) { //this check is to see if both are in the noise threshold
				current_is_interesting = FALSE;  // I calculated this by doing average(abs(sin(x))) * NOISE * 200 data pts
			}

			stable_current = new_current;
		}


		if(send_next_current) {
			send_next_current = FALSE;
			current_is_interesting = FALSE;

			encode_data_for_transmit(args);
			continue_transmits();
		} else if(current_is_interesting) {
			current_reference -= (int32_t)current_difference>>(int32_t)1; // If its interesting, we shouldn't use it to figure
			// out the reference

			current_is_interesting = FALSE;

			send_next_current = TRUE;
			hold_transmits(); //want to get the next one too!
			encode_data_and_old_data_for_transmit(args);
		}
	}




	if(should_transmit_wattage && voltage_is_ready && current_is_ready) {
		calculate_wattage();
		current_is_ready = FALSE;
		voltage_is_ready = FALSE;
	}


	return SUCCESS;
}



uint8_t voltage_on_full(node_ref args) {

	//For "interesting" detection
	new_voltage = 0;

	int16_t * voltage_array = 0;
	int16_t voltage_value = 0;
	uint16_t voltage_reading = 0;

	voltage_array = sensor_get_data_array(voltage_sensor);

	// With the data acquired, now we can loop through it and fix it for the reference
	int32_t voltage_sum = 0;
	uint16_t itor;
	for(itor = 0; itor < VOL_CUR_SIZE; itor++) {
		voltage_reading = voltage_array[itor];


		// Since it is a sine wave, it is centered at 0, so lets sum them up
		voltage_sum += voltage_reading;
	}
	int32_t voltage_average = (int32_t)voltage_sum/(int32_t)VOL_CUR_SIZE;

	// Could low pass it right here
	int32_t voltage_difference = (int32_t)voltage_average-(int32_t)voltage_reference;
	voltage_reference += (int32_t)voltage_difference>>(int32_t)1;

	for(itor = 0; itor < VOL_CUR_SIZE; itor++) {
		voltage_reading = voltage_array[itor];

		// Calculate Current
		// Current is the voltage reading minus its reference (centered at 3.3/2)
		voltage_value = (int16_t)(voltage_reading-voltage_reference);

		// Check our threshold.  If it is under 30, then it must be under 5 volts and must be noise
		if(voltage_value < 30 && voltage_value > -30) {// TODO magic number
		  voltage_value = 0;
		}

		voltage_array[itor] = voltage_value;

		// Add up the abs value of the voltage
		if(voltage_value > 0) {
			new_voltage = (int32_t)new_voltage + (int32_t)voltage_value;
		} else {
			new_voltage = (int32_t)new_voltage - (int32_t)voltage_value;
		}

	}


	if(voltage_settling_counter > 0) {
		voltage_settling_counter--;
		stable_voltage = new_voltage;
		voltage_is_ready = FALSE;
	} else {
		voltage_is_ready = TRUE;
	}

	if(should_transmit_voltage) {
		//detect if there is any changes
		uint8_t bit_itor = 0;
		uint32_t stable_msb_bit = (uint32_t)0x80000000; //just the top bit

		// Find the MSB of stable voltage
		for(bit_itor = 0; bit_itor < 32; bit_itor++) {
			if((uint32_t)stable_voltage & (uint32_t)stable_msb_bit) break;
			stable_msb_bit = (uint32_t)stable_msb_bit>>1;
		}

		// Find the difference in voltage levels
		uint32_t diff_voltage;
		if((uint32_t)stable_voltage > (uint32_t)new_voltage) diff_voltage = (uint32_t)stable_voltage - (uint32_t)new_voltage;
		else diff_voltage = (uint32_t)new_voltage - (uint32_t)stable_voltage;

		// Check the size of of diff_voltage vs the msb bit
		uint32_t threshold_msb_bit = (uint32_t)stable_msb_bit >>(uint32_t)WATTAGE_THRESHOLD_BITS;
		if((uint32_t)diff_voltage > (uint32_t)threshold_msb_bit) {
			voltage_is_interesting = TRUE;
			if(stable_voltage < 3000 && new_voltage < 3000) { //this check is to see if both are in the noise threshold
				voltage_is_interesting = FALSE;  // I calculated this by doing average(abs(sin(x))) * NOISE * 200 data pts
			}

			stable_voltage = new_voltage;

		}


		if(send_next_voltage) {
			send_next_voltage = FALSE;
			voltage_is_interesting = FALSE;

			encode_data_for_transmit(args);
			continue_transmits();
		} else if(voltage_is_interesting) {
			voltage_is_interesting = FALSE;

			send_next_voltage = TRUE;
			hold_transmits(); //want to get the next one too!
			encode_data_and_old_data_for_transmit(args);
		}
	}



	if(should_transmit_wattage && current_is_ready && voltage_is_ready) {
		calculate_wattage();
		current_is_ready = FALSE;
		voltage_is_ready = FALSE;
	}


	return SUCCESS;
}


uint8_t calculate_wattage() {

	int16_t * current_array = 0;
	int16_t * voltage_array = 0;

	int16_t cur_current = 0;
	int16_t cur_voltage = 0;

	current_array = sensor_get_data_array(current_sensor);
	voltage_array = sensor_get_data_array(voltage_sensor);

	power_sum = 0;

	// With the data acquired, now we can loop through it and calculate wattage
	uint16_t itor;
	for(itor = 0; itor < VOL_CUR_SIZE; itor++) {
		cur_current = current_array[itor];
		cur_voltage = voltage_array[itor];

		// Calculate Instantaneous Wattage
		// Multiply current and voltage together.
		// Eventually we will add them up and divide, so, we might as well add now
		MPYS = cur_current;
		OP2 = cur_voltage;
		uint32_t current_power = 0;
		current_power |= RES0;
		current_power |= (int32_t)RES1<<(int32_t)16;
		power_sum += (int32_t)current_power;

	}


	power_average = (int32_t)power_sum / (int32_t)VOL_CUR_SIZE; //todo get rid of this division

	//low pass the average
	power_low_passed = (int32_t)3*(int32_t)power_low_passed + (int32_t)power_average;
	power_low_passed = (int32_t)power_low_passed>>(int32_t)2;

	int16_t result_wattage = power_low_passed >>5;

	//noise thresholds
	// 3 watts = 5 / 0.165 = 5 * 6 = 30 )
	if(result_wattage < 30 && result_wattage > -30) result_wattage = 0;

	// give some settling time
	if(wattage_settling_counter > 0) wattage_settling_counter--;

	if(check_time(sensor_get_period(wattage_sensor), sensor_get_trigger_time(wattage_sensor))) {
		if(wattage_settling_counter == 0) {
			store_data_point(wattage_sensor, result_wattage);
		}
	}
	return SUCCESS;
}




/// *********************** CREATE SENSORS

uint8_t create_internal_current_sensor(time_ref period, uint16_t size) {
	if(current_sensor) {
		// We already have one
		enable_internal_current_sensor();
		return SUCCESS;
	}

	action_ref full_action = 0;
	node_ref sensor_node = 0;

	for(;;) {

		time_clear(period);
		time_set_clock_time(period, VOL_CUR_PERIOD);

		current_sensor = new_sensor('I', CURRENT_ADC, period, VOL_CUR_SIZE);
		if(!current_sensor) break;

		// Create the transmit action
		full_action = new_action();
		if(!full_action) break;
		action_set_func(full_action, &current_on_full);

		sensor_node = new_node(current_sensor, 0);
		if(!sensor_node) break;
		action_set_args(full_action, sensor_node);

		sensor_add_action_on_data_full(current_sensor, full_action);

		should_transmit_current = TRUE;
		sync_wattage_sensors();
		return SUCCESS;
	}

	sensor_delete(&current_sensor);
	action_delete(&full_action);

	return FAILURE;
}


uint8_t create_internal_voltage_sensor(time_ref period, uint16_t size) {
	if(voltage_sensor) {
		// We already have one
		enable_internal_voltage_sensor();
		return SUCCESS;
	}

	action_ref full_action = 0;
	node_ref sensor_node = 0;

	for(;;) {

		time_clear(period);
		time_set_clock_time(period, VOL_CUR_PERIOD);

		voltage_sensor = new_sensor('V', VOLTAGE_ADC, period, VOL_CUR_SIZE);
		if(!voltage_sensor) break;

		// Create the transmit action
		full_action = new_action();
		if(!full_action) break;
		action_set_func(full_action, &voltage_on_full);

		sensor_node = new_node(voltage_sensor, 0);
		if(!sensor_node) break;
		action_set_args(full_action, sensor_node);

		sensor_add_action_on_data_full(voltage_sensor, full_action);

		should_transmit_voltage = TRUE;
		sync_wattage_sensors();
		return SUCCESS;
	}

	sensor_delete(&voltage_sensor);
	action_delete(&full_action);

	return FAILURE;
}



uint8_t create_internal_wattage_sensor(time_ref period, uint16_t size) {
	if(wattage_sensor) {
		// We already have one
		if((time_cmp(period, sensor_get_period(wattage_sensor)) == 0) && size == sensor_get_size(wattage_sensor)) {
			// If they are the same...do nothing
			enable_internal_wattage_sensor();
			return SUCCESS;
		} else {
			// They are different, so delete the old one
			delete_internal_wattage_sensor();
		}
	}

	action_ref transmit_action = 0;

	//Use a for loop so we can do a break statement for error checking
	for(;;) {

		// Create all the sensors
		wattage_sensor = new_sensor('W', 0, period, size);
		if(!wattage_sensor) break;


		// We give period to the internal sensors, because they need to use
		// the struct temporarily.  Could get around this by allocating a new
		// time and then freeing it
		if(!voltage_sensor)	{
			create_internal_voltage_sensor(period, size);
			should_transmit_voltage = FALSE;
		}
		if(!voltage_sensor) break;

		if(!current_sensor) {
			create_internal_current_sensor(period,size);
			should_transmit_current = FALSE;
		}
		if(!current_sensor) break;


		// Create the transmit action
		transmit_action = new_transmit_action(wattage_sensor);
		if(!transmit_action) break;
		sensor_add_action_on_data_full(wattage_sensor, transmit_action);

		should_transmit_wattage = TRUE;
		return SUCCESS;
	}


	//CLEAN UP
	delete_internal_wattage_sensor();

	action_delete(&transmit_action);

	return FAILURE;
}


// *********  ENABLE SENSORS

uint8_t enable_internal_current_sensor() {
	if(!current_sensor) return FAILURE;
	should_transmit_current = TRUE;
	enable_sensor(current_sensor);
	current_settling_counter = SETTLING_TIME;
	return SUCCESS;
}
uint8_t enable_internal_voltage_sensor() {
	if(!voltage_sensor) return FAILURE;
	should_transmit_voltage = TRUE;
	enable_sensor(voltage_sensor);
	voltage_settling_counter = SETTLING_TIME;
	return SUCCESS;
}
uint8_t enable_internal_wattage_sensor() {
	if(!should_transmit_current) {
		enable_internal_current_sensor();
		should_transmit_current = FALSE;
	}
	if(!should_transmit_voltage) {
		enable_internal_voltage_sensor();
		should_transmit_voltage = FALSE;
	}

	sync_wattage_sensors();

	should_transmit_wattage = TRUE;
	// dont need to enable wattage because it doesn't use a sensor directly

	return SUCCESS;
}


// ************** DISABLE SENSORS

uint8_t disable_internal_current_sensor() {
	if(!current_sensor) return FAILURE;
	should_transmit_current = FALSE;

	if(!should_transmit_wattage) {
		disable_sensor(current_sensor);
	}

	return SUCCESS;
}
uint8_t disable_internal_voltage_sensor() {
	if(!voltage_sensor) return FAILURE;
	should_transmit_voltage = FALSE;

	if(!should_transmit_wattage) {
		disable_sensor(voltage_sensor);
	}

	return SUCCESS;
}
uint8_t disable_internal_wattage_sensor() {
	should_transmit_wattage = FALSE;

	if(!should_transmit_current) disable_internal_current_sensor();
	if(!should_transmit_voltage) disable_internal_voltage_sensor();

	//don't need to disable wattage, because it was never enabled, it doesn't use a sensor
	return SUCCESS;
}


// ***************** DELETE SENSORS

uint8_t delete_internal_current_sensor() {
	if(!current_sensor) return FAILURE;
	should_transmit_current = FALSE;
	if(!should_transmit_wattage) sensor_delete(&current_sensor);
	return SUCCESS;
}
uint8_t delete_internal_voltage_sensor() {
	if(!voltage_sensor) return FAILURE;
	should_transmit_voltage = FALSE;
	if(!should_transmit_wattage) sensor_delete(&voltage_sensor);
	return SUCCESS;
}
uint8_t delete_internal_wattage_sensor() {
	should_transmit_wattage = FALSE;

	if(!should_transmit_current) delete_internal_current_sensor();
	if(!should_transmit_voltage) delete_internal_voltage_sensor();

	if(!wattage_sensor) return FAILURE;
	sensor_delete(&wattage_sensor);
	return SUCCESS;
}
