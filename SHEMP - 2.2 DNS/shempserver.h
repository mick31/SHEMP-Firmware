/*
 * shempserver.h
 *
 *  Created on: Jul 25, 2012
 *      Author: nabercro
 */

#ifndef SHEMPSERVER_H_
#define SHEMPSERVER_H_

#include "roving.h"
#include "sensors.h"
#include "events.h"
#include "linked_list.h"
#include "uart.h"


#define OUTPUT_BUFFER_SIZE 6000

void init_transmits();
action_ref new_transmit_action(sensor_ref s);

uint8_t encode_data_for_transmit(node_ref args);
uint8_t encode_data_and_old_data_for_transmit(node_ref args);

void hold_transmits();
void continue_transmits();


void reset_output_buffer();
uint8_t have_data_to_transmit();

uint8_t transmit_header();

uint8_t transmit_data();





#endif /* SHEMPSERVER_H_ */
