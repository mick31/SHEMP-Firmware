/*
 * light_sensor.c
 * Author: Nathan Abercrombie
 *
 */


#include "light_sensor.h"


// ENABLE
uint8_t enable_light_sensor_A();
uint8_t enable_light_sensor_B();

// DISABLE
uint8_t disable_light_sensor_A();
uint8_t disable_light_sensor_B();

// DELETE
uint8_t delete_light_sensor_A();
uint8_t delete_light_sensor_B();


sensor_ref light_sensor_A;
sensor_ref light_sensor_B;

void init_light_sensor() {
	light_sensor_A = 0;
	light_sensor_B = 0;
}


sensor_ref create_light_sensor(uint8_t loc, time_ref period, uint16_t size) {
	sensor_ref * light_sensor;
	uint8_t (*delete_func)();
	uint8_t (*enable_func)();
	uint8_t (*disable_func)();
	uint8_t channel;

	// Based on the location, change the functions
	if (loc == 'A') {
		light_sensor = &light_sensor_A;
		delete_func = &delete_light_sensor_A;
		enable_func = &enable_light_sensor_A;
		disable_func = &disable_light_sensor_A;
		channel = PORT_A_ADC;
	} else if (loc == 'B') {
		light_sensor = &light_sensor_B;
		delete_func = &delete_light_sensor_B;
		enable_func = &enable_light_sensor_B;
		disable_func = &disable_light_sensor_B;
		channel = PORT_B_ADC;
	} else {
		return FAILURE;
	}

	// Check to see if we already have a light sensor there
	if(*light_sensor) {
		// We already have one there
		if((time_cmp(period, sensor_get_period(*light_sensor)) == 0) && size == sensor_get_size(*light_sensor)) {
			// If they are the same...do nothing
			return *light_sensor;
		} else {
			// They are different, so delete the old one
			delete_func();
		}
	}


	action_ref transmit_action = 0;

	// Create the sensor, the transmit action, and set the functions
	for(;;) {

		*light_sensor = new_sensor('L', channel, period, size);
		if(!*light_sensor) break;

		// Create the transmit action
		transmit_action = new_transmit_action(*light_sensor);
		if(!transmit_action) break;
		sensor_add_action_on_data_full(*light_sensor, transmit_action);

		sensor_set_delete_func(*light_sensor, delete_func);
		sensor_set_enable_func(*light_sensor, enable_func);
		sensor_set_disable_func(*light_sensor, disable_func);

		return *light_sensor;
	}

	delete_func();
	action_delete(&transmit_action);

	return FAILURE;

}




// ENABLE LIGHT
uint8_t enable_light_sensor_A() {
	return enable_sensor(light_sensor_A);
}
uint8_t enable_light_sensor_B() {
	return enable_sensor(light_sensor_B);
}

// DISABLE LIGHT
uint8_t disable_light_sensor_A() {
	return disable_sensor(light_sensor_A);
}
uint8_t disable_light_sensor_B() {
	return disable_sensor(light_sensor_B);
}

// DELETE LIGHT
uint8_t delete_light_sensor_A() {
	return sensor_delete(&light_sensor_A);
}
uint8_t delete_light_sensor_B() {
	return sensor_delete(&light_sensor_B);
}

