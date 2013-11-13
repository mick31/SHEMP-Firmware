/*
 * sensors.h
 *
 *  Created on: Jul 13, 2012
 *      Author: nabercro
 */

#ifndef SENSORS_H_
#define SENSORS_H_

#include "events.h"
#include <msp430.h>

#include "time.h"
#include "auxlib.h"

struct sensor;
#define sensor_ref struct sensor *


// These are needed by each sensor to know which ADC it is
// See init sensors
#define CURRENT_ADC 0
#define VOLTAGE_ADC 1
#define TEMPERATURE_ADC 2
#define PORT_A_ADC 3
#define PORT_B_ADC 4




//TODO Move this to sensors.c
struct sensor{
	uint8_t type;
	uint8_t adc_channel;

	time_ref trigger_time;
	time_ref period;
	uint8_t enabled;

	uint16_t size;
	uint16_t count;

	uint8_t data_ready;
	time_ref end_time;

	int16_t * write_array;
	int16_t * data_array;
	int16_t * old_array;

	action_ref on_full;

	uint8_t (*delete_this)();
	uint8_t (*enable_this)();
	uint8_t (*disable_this)();
};



uint8_t init_adc();
uint8_t sample_enabled_sensors();
uint8_t handle_adc_interrupt();
uint8_t handle_full_sensors();

void start_sampling();
void stop_sampling();
uint8_t ready_to_sample();

sensor_ref new_sensor(uint8_t type, uint8_t channel, time_ref period, uint16_t data_array_size);
sensor_ref new_one_time_sensor(uint8_t type, uint8_t channel, time_ref period, uint16_t data_array_size);

uint8_t sensor_delete(sensor_ref * sensor_ptr_ptr);
uint8_t sensor_delete_list(sensor_ref * sensor_ptr_ptr);

uint8_t store_data_point(sensor_ref s, uint16_t measurement);

uint8_t sensor_add_action_on_data_full(sensor_ref s, action_ref a);

int16_t * sensor_get_data_array(sensor_ref s);
int16_t * sensor_get_old_array(sensor_ref s);

uint8_t enable_sensor_args(node_ref sensors);
uint8_t disable_sensor_args(node_ref sensors);

uint8_t enable_sensor(sensor_ref sensor);
uint8_t disable_sensor(sensor_ref sensor);


uint8_t sensor_clear_state(sensor_ref s);

uint8_t sensor_get_type(sensor_ref s);
uint16_t sensor_get_size(sensor_ref s);
time_ref sensor_get_period(sensor_ref s);
time_ref sensor_get_end_time(sensor_ref s);
time_ref sensor_get_trigger_time(sensor_ref s);
uint8_t sensor_get_loc(sensor_ref s);


uint8_t sensor_set_delete_func(sensor_ref s, uint8_t (*delete_func)());
uint8_t sensor_set_disable_func(sensor_ref s, uint8_t (*disable_func)());
uint8_t sensor_set_enable_func(sensor_ref s, uint8_t (*enable_func)());

uint8_t sensor_disable_this(sensor_ref s);
uint8_t sensor_enable_this(sensor_ref s);
uint8_t sensor_delete_this(sensor_ref s);


uint8_t loc_to_channel(uint8_t loc);
uint8_t channel_to_loc(uint8_t channel);



#endif /* SENSORS_H_ */
