/*
 * leds.h
 * Author: Nathan Abercrombie
 *
 * This module is to animate the LEDs
 * The pins can be set here
 *
 * simply do
 * >> set_led_anim(led_main_start);
 * and call run_led_driver(); to update the LEDs
 * This is designed to have run_led_driver() called 12000 times/sec
 */

#ifndef LEDS_H_
#define LEDS_H_

#include <inttypes.h>
#include <msp430.h>
#include "time.h"

enum led_anim {
	led_off,
	led_start,

	led_setup_start,
	led_setup_slave_assoc,
	led_setup_slave_found_master,
	led_setup_master_assoc,

	led_main_start,
	led_main_assoc,
	led_main_connected,

	led_error,
};

void led_ping();

#define LEDDIR P8DIR
#define LEDOUT P8OUT
#define LED0	BIT0
#define LED1	BIT1
#define LED2	BIT2
#define LED3	BIT3

#define REDLED	LED0
#define YELLOWLED	LED1
#define GREENLED1	LED2
#define GREENLED2	LED3

void init_leds();
//void set_led(uint8_t led);
//void clear_led(uint8_t led);
//void toggle_led(uint8_t led);
void set_led_anim(enum led_anim anim);
void run_led_driver();



#endif /* LEDS_H_ */
