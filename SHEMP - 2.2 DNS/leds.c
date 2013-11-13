/*
 * leds.c
 *
 *  Created on: Aug 20, 2012
 *      Author: nabercro
 */
#include "leds.h"

enum led_anim led_state;
uint16_t cur_time;
uint16_t ping_time;

void init_leds() {
	led_state = led_off;
	cur_time = 0;
	ping_time = 0;

	LEDDIR |= LED0 + LED1 + LED2 + LED3;
	LEDOUT &= ~(LED0 + LED1 + LED2 + LED3);
}

void set_led_anim(enum led_anim anim) {
	led_state = anim;
}

void led_ping() {
	// This just pings the last LED, signifying a transmit
	// It doesn't work that well

	ping_time = cur_time + 1200; //0.1 sec
	if(ping_time >= 12000) ping_time = ping_time - 12000;

	LEDOUT |= GREENLED2;
}

// Holds onto the current LEDOUT port state
uint8_t LEDOUT_temp;


void turn_on(uint8_t led) {
	LEDOUT_temp |= led;
}

void blink_fast(uint8_t led) {
	if(cur_time < 2000) LEDOUT_temp |= led;
	else if (cur_time < 4000);
	else if (cur_time < 6000) LEDOUT_temp |= led;
	else if (cur_time < 8000);
	else if (cur_time < 10000) LEDOUT_temp |= led;
}

void blink_medium(uint8_t led) {
	if(cur_time < 3000) LEDOUT_temp |= led;
	else if (cur_time < 6000);
	else if (cur_time < 9000) LEDOUT_temp |= led;
}

void blink_slow(uint8_t led) {
	if(cur_time < 6000) LEDOUT_temp |= led;
}

void blink_slow_sliding(uint8_t led) {
	if(cur_time > 3000 && cur_time < 9000) LEDOUT_temp |= led;
}
void blink_slow_alternating(uint8_t led) {
	if(cur_time > 6000) LEDOUT_temp |= led;
}


// This handles the setup and keeping track of time, each time run_led_driver is called
void prep_leds() {
	LEDOUT_temp = LEDOUT;
	LEDOUT_temp &= ~(REDLED + YELLOWLED + GREENLED1);

	if(cur_time >= 12000) cur_time = 0;
	if(cur_time == ping_time) LEDOUT_temp &= ~(GREENLED2);
	cur_time++;
}

// This finishes the run_led_driver
void update_leds() {
	LEDOUT = LEDOUT_temp;
}

void run_led_driver() {
	prep_leds();

	switch(led_state) {

	case led_off:
		break;
	case led_start:
		turn_on(REDLED);
		break;

	case led_setup_start:
		turn_on(REDLED);
		blink_slow(YELLOWLED);
		blink_slow(GREENLED1);
		blink_slow(GREENLED2);
		break;
	case led_setup_slave_assoc:
		turn_on(REDLED);
		blink_slow(YELLOWLED);
		turn_on(GREENLED1);
		blink_slow(GREENLED2);
		break;
	case led_setup_slave_found_master:
		turn_on(REDLED);
		blink_slow(YELLOWLED);
		turn_on(GREENLED1);
		turn_on(GREENLED2);
		break;
	case led_setup_master_assoc:
		turn_on(REDLED);
		blink_fast(YELLOWLED);
		turn_on(GREENLED1);
		turn_on(GREENLED2);
		break;

	case led_main_start:
		turn_on(REDLED);
		blink_slow(YELLOWLED);
		blink_slow_sliding(GREENLED1);
		blink_slow_alternating(GREENLED2);
		break;
	case led_main_assoc:
		turn_on(REDLED);
		turn_on(YELLOWLED);
		blink_slow_alternating(GREENLED1);
		break;
	case led_main_connected:
		turn_on(REDLED);
		turn_on(YELLOWLED);
		turn_on(GREENLED1);
		blink_slow(GREENLED2);
		break;

	case led_error:
		blink_slow(REDLED + YELLOWLED + GREENLED1);
		break;
	}

	update_leds();
}
