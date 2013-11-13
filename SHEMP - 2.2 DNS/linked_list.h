/*
 * linked_list.h
 * Author: Nathan Abercrombie
 *
 * Pretty self explanatory
 */

#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include "bool.h"
#include <stdlib.h>
#include <inttypes.h>


typedef struct node node;
#define node_ref node *


//TODO Move this to linked_list.c, it was here for debugging purposes

struct node {
	void * value;
	uint8_t (*delete_func)(void **);
	node_ref next_node;

};




uint8_t node_delete_next(node_ref n);
/*
 * node_ref new_node(void * a1);
 *
 * Creates a new node struct with a1 as the value.
 *
 * Returns the pointer to the new struct, 0 for failure.
 *
 */
node_ref new_node(void * val, uint8_t (*del_func)(void **));


/*
 * uint8_t node_delete(node_ref * a);
 *
 * Frees all memory associated with the node, except for the next_node field.
 * Must pass in a pointer to the node_ref, for reassignment.  The pointer is
 * set to equal the next_node, essentially removing the targeted action from
 * a list.
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t node_delete(node_ref * a);


/*
 * uint8_t node_delete_list(node_ref * a_ptr);
 *
 * Similar to node_delete() except that it deletes all of the following
 * nodes, also.
 * Must pass in a pointer to the node_ref, for reassignment.  The pointer is
 * set to equal null
 *
 * Returns 1 for success, 0 for failure.
 */
uint8_t node_delete_list(node_ref * a_ptr);


/*
 * uint32_t node_get_val(node_ref a);
 *
 * Returns the value of the node struct as an int32
 */
void * node_get_val(node_ref a);

/*
 * uint8_t node_set_val(node_ref a, uint32_t val);
 *
 * Sets the value of the node struct
 *
 * Returns SUCCESS or FAILURE
 */
uint8_t node_set_val(node_ref a, void * val);

/*
 * node_ref node_get_next(node_ref a);
 *
 * Returns the pointer to the next node
 */
node_ref node_get_next(node_ref a);


/*
 * uint8_t node_append(node_ref a1, node_ref a2);
 *
 * Appends node2 to the end of node1's list.
 *
 * Returns SUCCESS or FAILURE
 */
uint8_t node_append(node_ref a1, node_ref a2);


/*
 * uint8_t node_remove_next(node_ref a);
 *
 * Removes the next_node from the linked list
 *
 * Returns SUCCESS or FAILURE
 */
uint8_t node_remove_next(node_ref a);

node_ref node_copy(node_ref a);
node_ref node_copy_list(node_ref a);

uint8_t create_and_add_node(node_ref * list_head, uint32_t value);



#endif /* LINKED_LIST_H_ */
