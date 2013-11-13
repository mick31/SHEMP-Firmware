/*
 * the_time.c
 *
 *  Created on: Jul 13, 2012
 *      Author: nabercro
 */


#include "time.h"

struct timestamp the_time;
struct timestamp the_time_frozen;

#define CLK_TO_MS 12

time_ref new_time() {
	time_ref result = malloc(sizeof(struct timestamp));
	if(!result) return FAILURE;

	result->clock_time = 0;
	result->milliseconds = 0;
	result->seconds = 0;
	result->minutes = 0;
	result->hours = 0;
	result->days = 0;

	return result;
}

uint8_t time_set_clock_time(time_ref t, uint16_t clock_time) {
	if(!t) return FAILURE;
	t->clock_time = clock_time;
	return SUCCESS;
}
uint8_t time_set_milliseconds(time_ref t, uint16_t milliseconds) {
	if(!t) return FAILURE;
	t->milliseconds = milliseconds;
	return SUCCESS;
}
uint8_t time_set_seconds(time_ref t, uint8_t seconds) {
	if(!t) return FAILURE;
	t->seconds = seconds;
	return SUCCESS;
}
uint8_t time_set_minutes(time_ref t, uint8_t minutes) {
	if(!t) return FAILURE;
	t->minutes = minutes;
	return SUCCESS;
}
uint8_t time_set_hours(time_ref t, uint8_t hours) {
	if(!t) return FAILURE;
	t->hours = hours;
	return SUCCESS;
}
uint8_t time_set_days(time_ref t, uint16_t days) {
	if(!t) return FAILURE;
	t->days = days;
	return SUCCESS;
}
uint8_t time_clear(time_ref t) {
	if(!t) return FAILURE;
	t->clock_time = 0;
	t->days = 0;
	t->hours = 0;
	t->milliseconds = 0;
	t->minutes = 0;
	t->seconds = 0;
	return SUCCESS;
}


uint8_t time_set_current(time_ref t) {
	if(!t) return FAILURE;
	t->clock_time = the_time.clock_time;
	t->milliseconds = the_time.milliseconds;
	t->seconds = the_time.seconds;
	t->minutes = the_time.minutes;
	t->hours = the_time.hours;
	t->days = the_time.days;

	return SUCCESS;
}


time_ref time_copy(time_ref original) {
	time_ref result = new_time();
	if(!result) return FAILURE;

	result->clock_time = original->clock_time;
	result->milliseconds = original->milliseconds;
	result->seconds = original->seconds;
	result->minutes = original->minutes;
	result->hours = original->hours;
	result->days = original->days;

	return result;
}


uint8_t time_delete(time_ref * time_ref_ptr) {
	if(!time_ref_ptr) return FAILURE;
	if(!*time_ref_ptr) return FAILURE;

	free(*time_ref_ptr);
	time_ref_ptr = 0;

	return SUCCESS;
}


void init_time() {
	the_time.clock_time = 0;
	the_time.days = 0;
	the_time.hours = 0;
	the_time.milliseconds = 0;
	the_time.minutes = 0;
	the_time.seconds = 0;
}

void time_tick() {
	the_time.clock_time++;

	if (the_time.clock_time >= CLK_TO_MS) {
		the_time.clock_time = 0;

		the_time.milliseconds++;
		if (the_time.milliseconds == 1000) {
			the_time.milliseconds = 0;

			the_time.seconds++;
			if(the_time.seconds == 60) {
				the_time.seconds = 0;

				the_time.minutes++;
				if(the_time.minutes == 60) {
					the_time.minutes = 0;

					the_time.hours++;
					if(the_time.hours == 24) {
						the_time.hours = 0;

						the_time.days++;
					}
				}
			}
		}
	}
}

time_ref global_time() {
	the_time_frozen.days = the_time.days;
	the_time_frozen.hours = the_time.hours;
	the_time_frozen.minutes = the_time.minutes;
	the_time_frozen.seconds = the_time.seconds;
	the_time_frozen.milliseconds = the_time.milliseconds;
	the_time_frozen.clock_time = the_time.clock_time;
	return &the_time_frozen;
}

uint16_t get_current_ms() {
	return the_time.milliseconds;
}



uint8_t add_clock_time_to_time(time_ref timestamp, uint16_t fast_time){
	if(!timestamp) return FAILURE;
	if(!fast_time) return FAILURE;

	timestamp->clock_time += fast_time;

	fix_time(timestamp);

	return SUCCESS;
}

uint8_t add_time_to_time(time_ref result_time, time_ref add_time) {
	if(!result_time) return FAILURE;
	if(!add_time) return FAILURE;

	result_time->clock_time += add_time->clock_time;
	result_time->milliseconds += add_time->milliseconds;
	result_time->seconds += add_time->seconds;
	result_time->minutes += add_time->minutes;
	result_time->hours += add_time->hours;
	result_time->days += add_time->days;

	fix_time(result_time);

	return SUCCESS;
}

uint8_t time_set_to_sum(time_ref dest, time_ref a, time_ref b) {
	if(!dest) return FAILURE;
	if(!a) return FAILURE;
	if(!b) return FAILURE;

	dest->clock_time = a->clock_time + b->clock_time;
	dest->milliseconds = a->milliseconds + b->milliseconds;
	dest->seconds = a->seconds + b->seconds;
	dest->minutes = a->minutes + b->minutes;
	dest->hours = a->hours + b->hours;
	dest->days = a->days + b->days;

	fix_time(dest);

	return SUCCESS;
}


int16_t time_cmp(time_ref a, time_ref b) {
	if(!a || !b) return FAILURE;
	int16_t diff = 0;

	diff = a->days - b->days;
	if(diff != 0) return diff;
	diff = a->hours - b->hours;
	if(diff != 0) return diff;
	diff = a->minutes - b->minutes;
	if(diff != 0) return diff;
	diff = a->seconds - b->seconds;
	if(diff != 0) return diff;
	diff = a->milliseconds - b->milliseconds;
	if(diff != 0) return diff;
	diff = a->clock_time - b->clock_time;
	return diff;
}


uint8_t fix_time(time_ref time) {
	if(!time) return FAILURE;

	while(time->clock_time >= CLK_TO_MS) {
		time->clock_time -= CLK_TO_MS;
		time->milliseconds++;
	}

	while(time->milliseconds >= 1000) {
		time->milliseconds -= 1000;
		time->seconds++;
	}

	while(time->seconds >= 60) {
		time->seconds -= 60;
		time->minutes++;
	}

	while(time->minutes >= 60) {
		time->minutes -= 60;
		time->minutes++;
	}

	while(time->hours >= 24) {
		time->hours -= 24;
		time->hours++;
	}

	return SUCCESS;
}


uint8_t check_time_args(node_ref args) {
	// the arguments are first PERIOD then TRIGGER_TIME
	if(!args) return FAILURE; //there are no thresholds, so it didn't trigger
	time_ref period = (time_ref)node_get_val(args);
	args = node_get_next(args);
	if(!args) return FAILURE;
	time_ref trigger = (time_ref)node_get_val(args);

	return check_time(period, trigger);
}


uint8_t check_time(time_ref period, time_ref trigger) {
	if(!period || !trigger) return FAILURE;

	if(time_cmp(global_time(), trigger) >= 0) {
		// if the_time > trigger
		// trigger = the_time + period
		time_set_to_sum(trigger, period, global_time());
		//add_time_to_time(trigger, period);
		return TRUE;
	}
	return FALSE;
}

event_ref new_timer_event(uint8_t timer, uint16_t period, uint8_t repeat, uint16_t repeat_count) {
	event_ref e = 0;
	action_ref check_action = 0;

	node_ref period_arg = 0;
	node_ref trigger_arg = 0;

	time_ref time_trigger = 0;
	time_ref time_period = 0;


	for(;;) {

		// Create a new time period
		time_period = new_time();
		if(!time_period) break;
		// Set up the period
		if(timer == CLOCK_TIME) {
			time_period->clock_time = period;
		} else if (timer == MILLISECONDS) {
			time_period->milliseconds = period;
		} else if (timer == SECONDS) {
			time_period->seconds = period;
		} else if (timer == MINUTES) {
			time_period->minutes = period;
		} else if (timer == HOURS) {
			time_period->hours = period;
		} else if (timer == DAYS) {
			time_period->days = period;
		}

		// Create a new time trigger
		time_trigger = new_time();
		if(!time_trigger) break;
		// Set up the trigger = global + period
		add_time_to_time(time_trigger, global_time());
		add_time_to_time(time_trigger, time_period);

		// Create a new event
		e = new_event();
		if(!e) break;

		// Create a new action
		check_action = new_action();
		if(!check_action) break;
		// Set its function
		action_set_func(check_action, &check_time_args);

		// Create a trigger arg
		trigger_arg = new_node(time_trigger, &time_delete);
		if(!trigger_arg) break;

		// Create the period arg
		period_arg = new_node(time_period, &time_delete);
		if(!period_arg) break;
		node_append(period_arg, trigger_arg);

		action_set_args(check_action, period_arg);
		event_set_check(e, check_action);
		event_set_repeat(e, repeat, repeat_count);

		return e;
	}

	event_delete(&e);

	node_delete(&period_arg);
	node_delete(&trigger_arg);

	action_delete(&check_action);

	time_delete(&time_period);
	time_delete(&time_trigger);

	return FAILURE;
}

uint8_t write_time_to_string(uint8_t * string, time_ref time) {
	if(!string || !time) return FAILURE;

	uint16_t cur_ptr = 0;

	string[cur_ptr++] = '0'+(uint8_t)(time->days / 100);
	string[cur_ptr++] = '0'+(uint8_t)((time->days % 100)/10);
	string[cur_ptr++] = '0'+(uint8_t)(time->days % 10);

	string[cur_ptr++] = '0'+(uint8_t)(time->hours / 10);
	string[cur_ptr++] = '0'+(uint8_t)(time->hours % 10);

	string[cur_ptr++] = '0'+(uint8_t)(time->minutes / 10);
	string[cur_ptr++] = '0'+(uint8_t)(time->minutes % 10);

	string[cur_ptr++] = '0'+(uint8_t)(time->seconds / 10);
	string[cur_ptr++] = '0'+(uint8_t)(time->seconds % 10);

	string[cur_ptr++] = '0'+(uint8_t)(time->milliseconds / 100);
	string[cur_ptr++] = '0'+(uint8_t)(time->milliseconds % 100)/10;
	string[cur_ptr++] = '0'+(uint8_t)(time->milliseconds % 10);

	string[cur_ptr++] = '0'+(uint8_t)(time->clock_time / 10);
	string[cur_ptr++] = '0'+(uint8_t)(time->clock_time % 10);

	return SUCCESS;
}

time_ref new_time_from_string(uint8_t * string) {
	time_ref result = 0;

	result = new_time();
	if(!result) return FAILURE;

	uint16_t cur_ptr = 0;

	result->days = read_number_from_string(&string[cur_ptr], 3);
	cur_ptr += 3;
	result->hours = read_number_from_string(&string[cur_ptr], 2);
	cur_ptr += 2;
	result->minutes = read_number_from_string(&string[cur_ptr], 2);
	cur_ptr += 2;
	result->seconds = read_number_from_string(&string[cur_ptr], 2);
	cur_ptr += 2;
	result->milliseconds = read_number_from_string(&string[cur_ptr], 3);
	cur_ptr += 3;
	result->clock_time = read_number_from_string(&string[cur_ptr], 2);
	cur_ptr += 2;

	return result;
}


void wait(uint16_t wait_ms) {
	uint16_t wait_ticks = wait_ms*32; // because it is sourced from a 32kHz clock
	uint16_t start_time = TA0R;
	uint16_t cur_time;

	while(1) {
		cur_time = TA0R;
		if ((cur_time - start_time) > wait_ticks) break;
	}

}
uint8_t wait_for(uint8_t (*break_func)(), uint16_t wait_ms) {
	uint16_t wait_ticks = wait_ms*32;
	uint16_t start_time = TA0R;
	uint16_t cur_time;

	while(1) {
		cur_time = TA0R;
		if ((cur_time - start_time) > wait_ticks) return FALSE;

		if(break_func()) return TRUE;
	}

}
