/*
 * linked_list.c
 *
 *  Created on: Jul 23, 2012
 *      Author: nabercro
 */

#include "linked_list.h"

node_ref new_node(void * val, uint8_t (*delete_func)(void **)) {
	node_ref result = malloc(sizeof(struct node));
	if(result) {

		result->value = val;
		result->next_node = 0;
		result->delete_func = delete_func;

		return result;
	}
	return FAILURE;
}


uint8_t node_delete(node_ref * node_ref_ptr) {
	if (!node_ref_ptr) return FAILURE; //pointer is null
	node_ref n = *node_ref_ptr;
	if (!(n)) return SUCCESS; //pointer is to a null pointer, event is already deleted?

	if(n->delete_func) {
		// If there is a delete function
		n->delete_func(n->value);
	}

	(*node_ref_ptr) = n->next_node;
	free(n);

	return SUCCESS;
}


uint8_t node_delete_list(node_ref * a_ptr) {
	if (!a_ptr) return FAILURE; //pointer is null

	while(*a_ptr) { //while it still exists
		node_delete(a_ptr);
	}

	return SUCCESS;
}


void * node_get_val(node_ref a) {
	return a->value;
}


uint8_t node_set_val(node_ref a, void * val) {
	if(!a) return FAILURE;

	a->value = val;

	return SUCCESS;
}

node_ref node_get_next(node_ref a) {
	if(!a) return FAILURE;
	return a->next_node;
}

uint8_t node_append(node_ref a1, node_ref a2) {
	if(!a1) return FAILURE;
	if(!a2) return FAILURE;
	node_ref a_cur = a1;
	while(a_cur->next_node) {
		a_cur = a_cur->next_node;
	}
	a_cur->next_node = a2;
	return SUCCESS;
}

uint8_t node_delete_next(node_ref a) {
	if (!a) return FAILURE;

	node_ref a_tmp = a->next_node;
	if(!a_tmp) { //there is no next
		return FAILURE;
	} else {
		a->next_node = a_tmp->next_node;
	}
	node_delete(&a_tmp);

	return SUCCESS;
}

uint8_t node_remove_next(node_ref a) {
	if (!a) return FAILURE;

	node_ref a_tmp = a->next_node;
	if(!a_tmp) { //there is no next
		return FAILURE;
	} else {
		a->next_node = a_tmp->next_node;
	}

	return SUCCESS;
}


node_ref node_copy(node_ref a) {
	if(!a) return FAILURE;

	node_ref return_node = new_node(a->value, a->delete_func);

	if(return_node) return return_node;
	return FAILURE;
}


node_ref node_copy_list(node_ref a) {
	if(!a) return FAILURE;

	node_ref cur_node = a;
	node_ref result_nodes = 0;

	while(cur_node) {
		if(result_nodes == 0) {
			//First one, assign it now
			result_nodes = node_copy(cur_node);
			if(!result_nodes) return FAILURE;
			cur_node = cur_node->next_node;
			continue;
		}

		node_append(result_nodes, node_copy(cur_node));
		cur_node = cur_node->next_node;
	}

	return result_nodes;
}
