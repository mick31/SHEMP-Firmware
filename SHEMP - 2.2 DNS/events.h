/*
 * events.h
 * Author: Nathan Abercrombie
 *
 * Event module.  Can create events, check for events, etc.
 * This ended up not being used except for the actions
 *
 * Use it like this:
 * action_ref a = new_action();
 * action_set_func(a, &foo);
 * node_ref n = new_node(bar, &delete_function);
 * action_set_args(a, n);
 *
 * Where foo is the function declared as
 * uint8_t foo(node_ref args);
 * And bar is any variable you'd like to use as an argument.
 * The node is a linked list, so you can chain multiple arguments.
 * delete_function is an optional parameter, it is the function
 * that deletes the value in the node, if it should be deleted when
 * you do delete_node(n);
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#include <inttypes.h>
#include <stdlib.h>
#include "linked_list.h"

#include "bool.h"

typedef struct action action;
#define action_ref action *

typedef struct event event;
#define event_ref event *

//TODO Move this to events.c, it was put here for debugging purposes
struct event{
	action_ref check_list;
	uint8_t repeat_mode;
	uint32_t max_repetitions;
	uint32_t repeat_count;
	action_ref actions_list;
	event_ref next_event;
};

struct action {
	uint8_t (*action_func)(node_ref args);
	node_ref arguments;
	action_ref next_action;
};



/*
 * uint8_t check_events(event_ref * e);
 *
 * This is the main function of this event scheduler.
 * Check for all events in the event list e.  If any event's check action
 * returns true, this will carry out any actions in the actions list.
 * Must pass in a pointer to the pointer to the first event, as it may
 * be deleted.
 *
 * Returns the number of events that triggered.
 */
uint8_t check_events(event_ref * e);


/*
 * uint8_t check_actions(action_ref a);
 *
 * This should not be called from a client except for debugging/testing purposes.
 * This is used by check_events() to check each action.
 *
 * Returns TRUE if all actions returned TRUE, else returns FALSE.
 */
uint8_t check_actions(action_ref a);

/*
 * uint8_t do_actions(action_ref a);
 *
 * This should be called from a client except for debugging/testing purposes.
 * This is used by check_events() to perform each action if check_actions
 * returns true.
 *
 * Returns SUCCESS on success, or FAILURE on failure.
 */
uint8_t do_actions(action_ref a);


/*
 * event_ref new_event();
 *
 * Creates a new event struct, leaves all fields empty
 *
 * Returns the pointer to the new event, 0 for failure.
 */
event_ref new_event();

/*
 * uint8_t event_delete(event_ref * event_ref_ptr);
 *
 * Frees all memory associated with the event, except for the next_event field.
 * Must pass in a pointer to the event_ref, for reassignment.  The pointer is
 * set to equal the next_event, essentially removing the targeted event from
 * a list.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t event_delete(event_ref * event_ref_ptr);

/*
 * uint8_t event_delete(event_ref * event_ref_ptr);
 *
 * Similar to event_delete() except that it deletes all of the following
 * events also.
 * Must pass in a pointer to the event_ref, for reassignment.  The pointer is
 * set to equal null
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t event_delete_list(event_ref * event_ref_ptr);

/*
 * uint8_t event_set_check(event_ref e, action_ref a);
 *
 * Sets the check_func of the event.  The action must return a 1 for true
 * or 0 for false.  If a list of actions is set, all must be true to trigger
 * the event.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t event_set_check(event_ref e, action_ref a);

/*
 * uint8_t event_set_repeat(uint8_t mode, uint32_t repetitions);
 *
 * Sets the repeat mode for the event.  See #defines for modes.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t event_set_repeat(event_ref e, uint8_t mode, uint32_t repetitions);
#define NO_REPEAT 0
#define REPEAT_LIMITED 1
#define REPEAT_FOREVER 2

/*
 * uint8_t event_set_actions(event_ref e, action_ref a);
 *
 * Sets the actions that the event should take when the check action triggers.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t event_set_actions(event_ref e, action_ref a);

/*
 * uint8_t event_append_event(event_ref e1, event_ref e2);
 *
 * Appends e2 to the end of e1.  (Ie: e1.next_event = e2)
 * If e1 already has a next_event, then e2 will go at the end of the list.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t event_append_event(event_ref e1, event_ref e2);

/*
 * uint8_t event_append_action(event_ref e, action_ref a);
 *
 * Appends action a to the end of the event's action list.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t event_append_action(event_ref e, action_ref a);

/*
 * uint8_t event_append_check(event_ref e, action_ref a);
 *
 * Appends action a to the end of the event's check list.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t event_append_check(event_ref e, action_ref a);






/*
 * action_ref new_action();
 *
 * Creates a new action struct.
 *
 * Returns the pointer to the new action, 0 for failure.
 */
action_ref new_action();

/*
 * uint8_t delete_action(action_ref * a_ptr);
 *
 * Frees all memory associated with the action, except for the next_action field.
 * Must pass in a pointer to the action_ref, for reassignment.  The pointer is
 * set to equal the next_action, essentially removing the targeted action from
 * a list.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t action_delete(action_ref * a_ptr);





/*
 * uint8_t action_delete_list(action_ref * a_ptr);
 *
 * Similar to action_delete() except that it deletes all of the following
 * actions also.
 * Must pass in a pointer to the action_ref, for reassignment.  The pointer is
 * set to equal null
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t action_delete_list(action_ref * a_ptr);

/*
 * uint8_t action_set_func(action_ref a, uint8_t (*func)(node_ref));
 *
 * Sets the function of the action.
 *
 * Returns SUCCESS or FAILURE
 */
uint8_t action_set_func(action_ref a, uint8_t (*func)(node_ref));

/*
 * uint8_t action_set_args(action_ref a, node_ref args);
 *
 * Sets the args of the action.  If an argument is already there, deletes that one.
 *
 * Returns SUCCESS or FAILURE
 */
uint8_t action_set_args(action_ref a, node_ref args);

/*
 * uint8_t action_append_action(action_ref a1, action_ref a2);
 *
 * Appends a2 to the end of a1.  (Ie: a1.next_action = a2)
 * If a1 already has a next_action, then a2 will go at the end of the list.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t action_append_action(action_ref a1, action_ref a2);


/*
 *
 *
 *
 */
action_ref action_copy (action_ref action);
action_ref action_copy_list(action_ref actions);




#endif /* EVENTS_H_ */
