/*
 * roving.h
 *
 *  Created on: Jul 5, 2012
 *      Author: nabercro
 */

#ifndef ROVING_H_
#define ROVING_H_

#include <msp430.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "linked_list.h"
#include "html.h"
#include "leds.h"

extern uint8_t parse_shemp_command(uint8_t * string, uint16_t length);

/* DEFINES */
#define TRUE 1
#define FALSE 0
#define UNKNOWN 2
#define ACK_STRING "\x06"

#define DISCONNECT_EVENT 	0x1
#define CONNECT_EVENT		0x2

uint8_t init_roving(uint8_t (*call_back_function)(uint8_t event));
uint8_t confirm_roving();
void reset_roving();
uint8_t enter_command_mode();
uint8_t exit_command_mode();
uint8_t send_command(uint8_t * command);
uint8_t send_command_with_arg(uint8_t * command, uint8_t * arg);
uint8_t send_data(uint8_t * data);

uint8_t in_setup_mode();
uint8_t in_main_mode();
uint8_t start_setup_mode();
uint8_t leave_setup_mode();
uint8_t done_setup();

uint8_t get_number_of_slaves();
uint8_t do_setup();

uint8_t is_associated();
uint8_t associate();

uint8_t get_dhcp();
uint8_t have_dhcp();

uint8_t send_ip();

void reset_ack();
uint8_t have_ack();

uint8_t is_connected();
uint8_t connect();
uint8_t disconnect();

uint8_t * get_new_ssid();
uint8_t * get_new_pass();
uint8_t * get_new_host_ip();
uint8_t * get_new_host_name();

uint8_t add_new_ssid(uint8_t * ssid, uint8_t length);
uint8_t add_new_pass(uint8_t * pass, uint8_t length);
uint8_t add_new_host_ip(uint8_t * ip, uint8_t length);
uint8_t add_new_host_name(uint8_t * host_name, uint8_t length);

uint8_t have_new_ssid();
uint8_t have_new_pass();
uint8_t have_new_host_ip();
uint8_t have_new_host_name();

void complete_setup();


void handle_roving_input();

uint8_t set_ip_host(uint8_t * ip_addr);


#endif /* ROVING_H_ */
