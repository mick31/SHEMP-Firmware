/*
 * audio_sensor.c
 * Author: Nathan Abercrombie
 * abercrombie.nathan@gmail.com
 *
 * Audio Sensor Module
 *
 * To understand how this sensor works, read through the comments.
 */

#include "audio_sensor.h"

/*
 * We use two 'sensors', both sampling the same ADC channel
 * audio_sensor_lofi is a lower sampling rate: 2400 samples/sec
 * audio_sensor_hifi is a higher sampling rate: 12000 samples/sec
 */
sensor_ref audio_sensor_lofi;
sensor_ref audio_sensor_hifi;

#define LOFI_PERIOD	(12000/2400) // 2400 hz
#define HIFI_PERIOD	(1) // 12000 hz
#define LOFI_SIZE	(240) // tenth of a second
#define HIFI_SIZE	(1200) // tenth of a second

/*
 * The idea is that the lofi sensor will detect when there is something
 * interesting to record, and then the hifi sensor will turn on, sample
 * quickly for a tenth of a second, and then turn off.
 *
 * audio_sensor_is_hifi is just a flag so that this module knows which
 * setting it is in
 */
uint8_t audio_sensor_is_hifi;

/*
 * In order to prevent it constantly sending these waveforms, there is
 * a cool-down time.
 */
#define AUDIO_COOL_DOWN_TIME 10
uint8_t audio_cool_down_counter;

// Prototypes
uint8_t detect_audio_use(node_ref args);
uint8_t swap_enabled_audio_sensor();

/*
 * init_audio_sensor()
 * Initializes some variables for this module
 */
void init_audio_sensor() {
	audio_sensor_lofi = 0;
	audio_sensor_hifi = 0;
	audio_sensor_is_hifi = FALSE;
	audio_cool_down_counter = AUDIO_COOL_DOWN_TIME;
}

/*
 * These are the standard enables/disable/delete functions
 */
uint8_t enable_audio_sensor() {
	audio_sensor_is_hifi = FALSE;
	disable_sensor(audio_sensor_hifi);
	enable_sensor(audio_sensor_lofi);
	return SUCCESS;
}
uint8_t disable_audio_sensor() {
	disable_sensor(audio_sensor_lofi);
	disable_sensor(audio_sensor_hifi);
	return SUCCESS;
}
uint8_t delete_audio_sensor() {
	sensor_delete(&audio_sensor_lofi);
	sensor_delete(&audio_sensor_hifi);
	return SUCCESS;
}

/*
 * This swaps the two sensors
 */
uint8_t swap_enabled_audio_sensor() {
	if(audio_sensor_is_hifi) {
		disable_sensor(audio_sensor_hifi);
		enable_sensor(audio_sensor_lofi);
		audio_sensor_is_hifi = FALSE;
	} else {
		disable_sensor(audio_sensor_lofi);
		enable_sensor(audio_sensor_hifi);
		audio_sensor_is_hifi = TRUE;
	}
	return SUCCESS;
}


/*
 * create_audio_sensor();
 * Creates both audio sensors
 */
sensor_ref create_audio_sensor(uint8_t loc, time_ref period, uint16_t size) {
	if(audio_sensor_lofi) {
		// We already have one
		if(loc != sensor_get_loc(audio_sensor_lofi)) {
			// This is a second audio sensor... we cannot have 2, failure
			// We cannot have two because the RAM needed to support two sets of 12khz sampling
			// is more than this microcontroller cna handle
			return FAILURE;
		}
	}

	uint8_t channel = loc_to_channel(loc); //just parse the location into a channel

	// Init variables
	action_ref detect_use_action = 0;
	action_ref swap_action = 0;
	node_ref sensor_node = 0;
	time_ref tmp_time = 0;

	// We use a for(;;) so we can use break
	for(;;) {
		// Create a temporary time that we will use to pass the periods to the new_sensor() function
		tmp_time = new_time();
		if(!tmp_time) break;
		time_set_clock_time(tmp_time, LOFI_PERIOD);

		// Create the lofi sensor
		audio_sensor_lofi = new_sensor('A', channel, tmp_time, LOFI_SIZE);
		if(!audio_sensor_lofi) break;

		// Set the delete/enable/disable functions
		sensor_set_delete_func(audio_sensor_lofi, &delete_audio_sensor);
		sensor_set_enable_func(audio_sensor_lofi, &enable_audio_sensor);
		sensor_set_disable_func(audio_sensor_lofi, &disable_audio_sensor);

		// Change the tmp_time
		time_set_clock_time(tmp_time, HIFI_PERIOD);

		// Create the hifi sensor
		audio_sensor_hifi = new_one_time_sensor('A', channel, tmp_time, HIFI_SIZE);
		if(!audio_sensor_hifi) break;

		// Delete tmp_time, as we are done with it
		time_delete(&tmp_time);

		// When the lofi sensor is full, it should trigger an action to detect if there is audio use
		detect_use_action = new_action();
		if(!detect_use_action) break;
		// Set the action's function to detect audio use
		action_set_func(detect_use_action, &detect_audio_use);
		// And set it to the hifi sensor
		sensor_node = new_node(audio_sensor_hifi,0);
		if(!sensor_node) break;
		action_set_args(detect_use_action, sensor_node);
		sensor_add_action_on_data_full(audio_sensor_lofi, detect_use_action);

		// When the hifi sensor is full, it should disable itself and enable the lofi sensor
		swap_action = new_action();
		if(!swap_action) break;
		action_set_func(swap_action, &swap_enabled_audio_sensor);
		sensor_add_action_on_data_full(audio_sensor_hifi, swap_action);

		return audio_sensor_lofi; //the lofi sensor is passed back, because it has the delete/enable/disable functions above
	}

	// If we got here, something failed, so delete everything
	sensor_delete(&audio_sensor_lofi);
	sensor_delete(&audio_sensor_hifi);

	action_delete(&detect_use_action);
	action_delete(&swap_action);

	node_delete(&sensor_node);

	time_delete(&tmp_time);

	return FAILURE;
}

/*
 * This is run every time the lofi sensor is full
 */
uint8_t detect_audio_use(node_ref args) {
	if(audio_cool_down_counter > 0) {
		audio_cool_down_counter--;
		return SUCCESS;
	}

	// Get the data
	int16_t * array = sensor_get_data_array(audio_sensor_lofi);

	uint16_t itor = 0;
	uint16_t audio_reading = 0;
	int16_t audio_diff = 0;
	int32_t signal_volume = 0;
	int32_t audio_reference = 0;
	int32_t audio_sum = 0;

	// Find the average value, in order to get the reference
	for(itor = 0; itor < LOFI_SIZE; itor++) {
		audio_sum += array[itor];
	}
	audio_reference = audio_sum / LOFI_SIZE;


	// Subtract the reference and keep track of the sum of the magnitude
	for(itor = 0; itor < LOFI_SIZE; itor++) {
		audio_reading = array[itor];
		audio_diff = audio_reading - audio_reference;
		if(audio_diff > 0) signal_volume = signal_volume + audio_diff;
		else signal_volume = signal_volume - audio_diff;
	}

	// If it is loud enough, swap sensors
	if(signal_volume > 40000) {
		swap_enabled_audio_sensor();
		audio_cool_down_counter = AUDIO_COOL_DOWN_TIME;
		encode_data_for_transmit(args);
	}

	return SUCCESS;
}


