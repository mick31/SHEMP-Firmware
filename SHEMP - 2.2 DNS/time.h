/*
 * the_time.h
 *
 *  Created on: Jul 13, 2012
 *      Author: nabercro
 */

#ifndef THE_TIME_H_
#define THE_TIME_H_

#include "events.h"
#include <msp430.h>
#include <inttypes.h>
#include "auxlib.h"


#define time_ref struct timestamp *

struct timestamp {
	uint16_t clock_time; //12 khz
	uint16_t milliseconds;
	uint16_t seconds;
	uint16_t minutes;
	uint16_t hours;
	uint16_t days;
};

void init_time();
void time_tick();
time_ref global_time();
uint16_t get_current_ms();


uint8_t add_time_to_time(time_ref result_time, time_ref add_time);
uint8_t time_set_to_sum(time_ref dest, time_ref a, time_ref b);

uint8_t time_set_clock_time(time_ref t, uint16_t clock_time);
uint8_t time_set_milliseconds(time_ref t, uint16_t milliseconds);
uint8_t time_set_seconds(time_ref t, uint8_t seconds);
uint8_t time_set_minutes(time_ref t, uint8_t minutes);
uint8_t time_set_hours(time_ref t, uint8_t hours);
uint8_t time_set_days(time_ref t, uint16_t days);

time_ref new_time();
uint8_t time_delete(time_ref * time_ref_ptr);

uint8_t check_time_args(node_ref args);
uint8_t check_time(time_ref period, time_ref trigger);
uint8_t time_set_current(time_ref t);

uint16_t get_current_ms();

uint8_t time_clear(time_ref time);

uint16_t time_in_ms(time_ref time);

time_ref time_copy(time_ref original);

int16_t time_cmp(time_ref a, time_ref b); // a - b

uint8_t fix_time(time_ref t);

void wait(uint16_t wait_ms);
uint8_t wait_for(uint8_t (*break_func)(), uint16_t wait_ms);

event_ref new_timer_event(uint8_t timer, uint16_t period, uint8_t repeat, uint16_t repeat_count);
#define CLOCK_TIME 0
#define MILLISECONDS 1
#define SECONDS 2
#define MINUTES 3
#define HOURS 4
#define DAYS 5

#define STRING_TO_TIME_LENGTH 14
uint8_t write_time_to_string(uint8_t * string, time_ref time);
time_ref new_time_from_string(uint8_t * string);

#endif /* THE_TIME_H_ */
