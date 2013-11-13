/*
 * events.c
 * Author: Nathan Abercrombie
 *
 *
 */


#include "events.h"
#include <msp430.h>

/* STRUCTS */


/* FUNCTIONS */

uint8_t check_actions(action_ref a) {
	action_ref a_cur = a;

	while(a_cur) {
		if (a_cur->action_func(a_cur->arguments)) return TRUE;
		a_cur = a_cur->next_action;
	}
	return FALSE;
}

uint8_t do_actions(action_ref a) {
	action_ref a_cur = a;
	while(a_cur) {
		a_cur->action_func(a_cur->arguments);
		a_cur = a_cur->next_action;
	}
	return SUCCESS;
}

uint8_t check_events(event_ref * e_ptr) {
	event_ref e_cur = *e_ptr;
	event_ref e_prev = 0;
	uint8_t triggered_events = 0;

	while(e_cur) { // For each event
		if (check_actions(e_cur->check_list)) { //Check the event
			do_actions(e_cur->actions_list);
			triggered_events++;
			if(e_cur->repeat_mode == REPEAT_FOREVER) {
				e_prev = e_cur;
				e_cur = e_cur->next_event;
				continue;
			}
			if(e_cur->repeat_mode == REPEAT_LIMITED) {
				e_cur->repeat_count++;
				if (e_cur->repeat_count >= e_cur->max_repetitions) {
					//repeated too many times, have to delete it
					e_cur = e_cur->next_event; //first move to the next
					if (e_prev) {
						//It was not the first event, go back one to get the pointer
						event_delete(&e_prev->next_event);
					} else {
						//This was the first event, so just use the pointer we were given.
						event_delete(e_ptr);
					}
					continue;
				}
			}
			if(e_cur->repeat_mode == NO_REPEAT) {
				//have to delete it
				e_cur = e_cur->next_event; //first move to the next
				if (e_prev) {
					//It was not the first event, go back one to get the pointer
					event_delete(&e_prev->next_event);
				} else {
					//This was the first event, so just use the pointer we were given.
					event_delete(e_ptr);
				}
				continue;
			}
		}
		e_prev = e_cur;
		e_cur = e_cur->next_event;
	}
	return triggered_events;
}


event_ref new_event() {
	event_ref e = malloc(sizeof(struct event));
	if (e) {
		e->check_list = 0;
		e->repeat_count = 0;
		e->repeat_mode = 0;
		e->actions_list = 0;
		e->next_event = 0;
		e->max_repetitions = 0;
		return e;
	}
	return FAILURE;
}

uint8_t event_delete(event_ref * e_ptr) {
	if (!e_ptr) return FAILURE; //pointer is null
	event_ref e = *e_ptr;
	if (!(e)) return SUCCESS; //pointer is to a null pointer, event is already deleted?

	action_delete_list(&e->check_list);
	action_delete_list(&e->actions_list);
	(*e_ptr) = e->next_event;

	free(e);
	return SUCCESS;
}

uint8_t event_delete_list(event_ref * e_ptr) {
	if (!e_ptr) return FAILURE; //pointer is null
	event_ref e = *e_ptr;

	while(e) { //while an event still exists
		event_delete(&e);
	}

	return SUCCESS;
}

uint8_t event_set_check(event_ref e, action_ref a) {
	if (!e) return FAILURE; //event is null
	if (!a) return FAILURE; //action is null

	if(e->check_list) action_delete_list(&e->check_list);

	e->check_list = a;
	return SUCCESS;
}

uint8_t event_set_repeat(event_ref e, uint8_t mode, uint32_t repetitions) {
	if (!e) return FAILURE; //event is null

	e->repeat_mode=mode;
	e->max_repetitions=repetitions;

	return SUCCESS;
}

uint8_t event_set_actions(event_ref e, action_ref a ){
	if (!e) return FAILURE; //event is null
	if (!a) return FAILURE; //action is null

	if(e->actions_list) action_delete_list(&e->actions_list);

	e->actions_list = a;
	return SUCCESS;
}

uint8_t event_append_event(event_ref e1, event_ref e2) {
	if (!e1) return FAILURE; //event 1 is null
	if (!e2) return FAILURE; //event 2 is null

	event_ref e_cur = e1;
	while(e_cur->next_event) {
		e_cur = e_cur->next_event;
	}
	e_cur->next_event = e2;

	return SUCCESS;
}

uint8_t event_append_action(event_ref e, action_ref a) {
	if (!e) return FAILURE; //event is null
	if (!a) return FAILURE; //action is null

	action_ref a_cur = e->actions_list;
	if (!a_cur) {
		//It has no actions yet
		e->actions_list = a;
		return SUCCESS;
	}

	while(a_cur->next_action) {
		a_cur = a_cur->next_action;
	}
	a_cur->next_action = a;

	return SUCCESS;
}

uint8_t event_append_check(event_ref e, action_ref a) {
	if (!e) return FAILURE; //event is null
	if (!a) return FAILURE; //action is null

	action_ref a_cur = e->check_list;
	if (!a_cur) {
		//It has no actions yet
		e->check_list = a;
		return SUCCESS;
	}

	while(a_cur->next_action) {
		a_cur = a_cur->next_action;
	}
	a_cur->next_action = a;

	return SUCCESS;
}




action_ref new_action() {
	action_ref a = malloc(sizeof(struct action));
	if(a) {
		a->action_func = 0;
		a->arguments = 0;
		a->next_action = 0;
		return a;
	}
	return FAILURE;
}

uint8_t action_delete(action_ref * a_ptr) {
	if (!a_ptr) return FAILURE; //pointer is null
	action_ref a = *a_ptr;
	if (!(a)) return SUCCESS; //pointer is to a null pointer, event is already deleted?


	node_delete_list(&a->arguments);

	(*a_ptr) = a->next_action;
	free(a);

	return SUCCESS;
}

uint8_t action_delete_list(action_ref * a_ptr) {
	if (!a_ptr) return FAILURE; //pointer is null
	action_ref a = *a_ptr;

	while(a) { //while an event still exists
		action_delete(&a);
	}

	return SUCCESS;
}



uint8_t action_set_func(action_ref a, uint8_t (*func)(node_ref)) {
	if (!a) return FAILURE;
	if (!func) return FAILURE;

	a->action_func = func;

	return TRUE;
}

uint8_t action_set_args(action_ref a, node_ref args) {
	if (!a) return FAILURE;
	if (!args) return FAILURE;

	a->arguments = args;

	return TRUE;
}



uint8_t action_append_action(action_ref a1, action_ref a2) {
	if (!a1) return FAILURE; //event 1 is null
	if (!a2) return FAILURE; //event 2 is null

	action_ref a_cur = a1;
	while(a_cur->next_action) {
		a_cur = a_cur->next_action;
	}
	a_cur->next_action = a2;

	return SUCCESS;
}


action_ref action_copy (action_ref action_proto) {
	if(!action_proto) return FAILURE;
	action_ref result = new_action();
	if(result) {
		result->action_func = action_proto->action_func;
		result->arguments = node_copy_list(action_proto->arguments);
		return result;
	}
	return FAILURE;
}

action_ref action_copy_list(action_ref actions) {
	if (!actions) return FAILURE; //no actions to copy

	action_ref cur_action = actions;
	action_ref result_actions = 0;

	while(cur_action) {
		if(result_actions == 0) {
			//First one, assign it now
			result_actions = action_copy(cur_action);
			if(!result_actions) return FAILURE;
			cur_action = cur_action->next_action;
			continue;
		}

		action_append_action(result_actions, action_copy(cur_action));
		cur_action = cur_action->next_action;
	}

	return result_actions;
}






