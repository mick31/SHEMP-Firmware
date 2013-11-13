/*
 * roving.c
 *
 *  Created on: Jul 5, 2012
 *      Author: nabercro
 */

#include "roving.h"

extern void uart_send_string(uint8_t * string);
extern void wait(uint16_t ms);

uint8_t host_setup_mode();

/* DATAS */
#define BUFFER_SIZE 1024
uint8_t input_buffer[BUFFER_SIZE];

uint16_t write_index;
uint16_t read_index;
uint8_t newline;

/* STATES */
uint8_t command_mode;
uint8_t socket_open;

enum config_mode {unknown, main_config, setup_slave, setup_master} config_mode;

uint8_t assoc;
uint8_t dhcp;

uint8_t ip[16];
uint8_t host_ip[16];
uint8_t new_host_ip[16];
uint8_t host_name[60];
uint8_t new_host_name[60];
uint8_t new_host_name_flag;
uint8_t new_host_ip_flag;
uint8_t never_was_a_slave;

uint8_t ssid[35];
uint8_t pass[35];
uint8_t new_ssid[35];
uint8_t new_pass[35];
uint8_t new_ssid_flag;
uint8_t new_pass_flag;
uint8_t setup_is_complete;
uint8_t setup_found_master;

node_ref slave_list;
uint8_t number_of_slaves;
uint8_t setup_join_attempts;
uint8_t setup_connect_attempts;
#define MAX_SETUP_JOIN_ATTEMPTS 5

uint8_t have_ack_flag;
uint8_t has_prompt;

uint8_t (*call_back)(uint8_t event);

uint8_t init_roving(uint8_t (*call_back_function)(uint8_t event)) {
	call_back = call_back_function;

	write_index = 0;
	read_index = 0;
	newline = FALSE;
	has_prompt = FALSE;

	number_of_slaves = 0;
	never_was_a_slave = TRUE;
	new_ssid_flag = FALSE;
	new_pass_flag = FALSE;
	new_host_ip_flag = FALSE;
	new_host_name_flag = FALSE;
	slave_list = 0;
	setup_is_complete = FALSE;
	setup_found_master = FALSE;
	setup_join_attempts = 0;
	setup_connect_attempts = 0;
	P9DIR |= BIT6;//Reset pin direction

	reset_roving();

	return TRUE;
}

void reset_roving() {
	P9OUT &= ~BIT6;

	config_mode = unknown;

	have_ack_flag = FALSE;

	command_mode = FALSE;
	socket_open = FALSE;

	assoc = FALSE;
	dhcp = FALSE;

	wait(100);
	P9OUT |= BIT6;
	wait(200);
}


void reset_ack() {
	have_ack_flag = FALSE;
}
uint8_t have_ack() {
	if(have_ack_flag) {
		return TRUE;
	} else {
		handle_roving_input();
		if(have_ack_flag) return TRUE;
		return FALSE;
	}
}

uint8_t does_has_prompt() {
	if(has_prompt) return TRUE;
	else {
		handle_roving_input();
		if(has_prompt) return TRUE;
		return FALSE;
	}

}

uint8_t send_command_with_arg(uint8_t * command, uint8_t * arg) {
	if (enter_command_mode()) {
		wait_for(&does_has_prompt, 500);
		uart_send_string(command);
		uart_send_string(" ");
		uart_send_string(arg);
		uart_send_string("\n\r");
		has_prompt = FALSE;
	}
	return TRUE;
}



uint8_t send_command(uint8_t * command){
	if (enter_command_mode()) {
		wait_for(&does_has_prompt, 500);
		uart_send_string(command);
		uart_send_string("\n\r");
		has_prompt = FALSE;
	}
	return TRUE;
}

uint8_t send_data(uint8_t * data) {
	if(!data) return FAILURE;
	if(exit_command_mode()) {
		uart_send_string(data);
	}
	return SUCCESS;
}


uint8_t in_command_mode() {
	if(command_mode) return TRUE;
	else {
		handle_roving_input();
		if(command_mode) return TRUE;
		return FALSE;
	}
}

uint8_t not_in_command_mode() {
	if(!command_mode) return TRUE;
	else {
		handle_roving_input();
		if(!command_mode) return TRUE;
		return FALSE;
	}
}

uint8_t enter_command_mode() {
	uint8_t attempts = 0;
	while(command_mode == FALSE) {
		uart_send_string((uint8_t *)"$$$");
		if(wait_for(&in_command_mode, 1000)) {
			has_prompt = TRUE;
			break;
		}
		attempts++;
		if(attempts > 5) {
			return FALSE;
		}
	}
	return TRUE;
}


uint8_t exit_command_mode() {
	uint8_t attempts = 0;
	while(command_mode == TRUE) {
		uart_send_string((uint8_t *)"exit\n\r");
		wait_for(&not_in_command_mode, 500);
		attempts++;
		if(attempts > 5) {
			return FALSE;
		}
	}
	return TRUE;
}



uint8_t is_associated() {
	if(assoc) return TRUE;
	else {
		send_command("show n");
		wait(10);
		handle_roving_input();
		if(assoc) return TRUE;
		return FALSE;
	}
}

uint8_t associate() {
	if(!is_associated) {
		send_command("join");
		wait_for(&is_associated, 500);
	}
	return is_associated();
}


uint8_t get_dhcp() {
	if(dhcp == FALSE) {
		send_command("show n");
		wait(10);
		handle_roving_input();
	}
	return 0;
}
uint8_t have_dhcp() {
	if(config_mode == setup_master) return TRUE;
	return dhcp;
}

uint8_t reset_setup_info() {
	new_ssid_flag = FALSE;
	new_pass_flag = FALSE;
	new_host_ip_flag = FALSE;
	new_host_name_flag = FALSE;
	setup_is_complete = FALSE;
	return TRUE;
}


uint8_t add_new_ssid(uint8_t * ssid, uint8_t length) {
	memcpy(new_ssid,ssid,length);
	new_ssid[length] = 0; //null plug!
	new_ssid_flag = TRUE;

	return SUCCESS;
}
uint8_t add_new_pass(uint8_t * pass, uint8_t length) {
	memcpy(new_pass,pass,length);
	new_pass[length] = 0; //null plug!
	new_pass_flag = TRUE;

	return SUCCESS;
}
uint8_t add_new_host_ip(uint8_t * ip, uint8_t length) {
	memcpy(new_host_ip,ip,length);
	new_host_ip[length] = 0; //null plug!
	new_host_ip_flag = TRUE;

	return SUCCESS;
}

uint8_t add_new_host_name(uint8_t * host_name, uint8_t length) {
	memcpy(new_host_name, host_name, length);
	new_host_name[length] = 0;
	new_host_name_flag = TRUE;

	return SUCCESS;
}

uint8_t * get_new_ssid() {
	return new_ssid;
}

uint8_t * get_new_pass() {
	return new_pass;
}
uint8_t * get_new_host_ip() {
	return new_host_ip;
}
uint8_t * get_new_host_name() {
	return new_host_name;
}


uint8_t have_new_ssid() {
	return new_ssid_flag;
}

uint8_t have_new_pass() {
	return new_pass_flag;
}
uint8_t have_new_host_ip() {
	return new_host_ip_flag;
}

uint8_t have_new_host_name() {
	return new_host_name_flag;
}

void complete_setup() {
	setup_is_complete = TRUE;
}






uint8_t send_ip() {
	send_data("SLAVE_IP=");
	send_data(ip);
	send_data("\n\r");
	return FALSE;
}



uint8_t in_setup_mode() {
	uint8_t attempts = 0;

	while(config_mode == unknown) {
		send_command("get o");
		wait(10);
		handle_roving_input();
		attempts++;
		if(attempts > 10) return FALSE;
	}

	if(config_mode == setup_slave) return TRUE;
	if(config_mode == setup_master) return TRUE;
	if(config_mode == main_config) return FALSE;

	return FALSE;
}

uint8_t in_main_mode() {
	uint8_t attempts = 0;

	while(config_mode == unknown) {
		send_command("get o");
		wait(10);
		handle_roving_input();
		attempts++;
		if(attempts > 10) return FALSE;
	}

	if(config_mode == setup_slave) return FALSE;
	if(config_mode == setup_master) return FALSE;
	if(config_mode == main_config) return TRUE;

	return FALSE;
}


uint8_t in_setup_slave_mode() {
	if(config_mode == setup_slave) return TRUE;
	else {
		if(in_setup_mode()) if(config_mode == setup_slave) return TRUE;
		return FALSE;
	}

}


uint8_t start_setup_mode() {
	uint8_t attempts = 0;

	while(1) {
		send_command("load setup_slave");
		setup_found_master = FALSE;
		config_mode = unknown;

		if(wait_for(&in_setup_slave_mode, 100)) break;

		attempts++;
		if (attempts > 5) {
			return FALSE;
		}
	}

	setup_join_attempts = 0;
	setup_connect_attempts = 0;

	send_command("save");
	wait(100);
	reset_roving();

	return TRUE;
}

uint8_t in_setup_master_mode() {
	if(config_mode == setup_master) return TRUE;
	else {
		if(in_setup_mode()) if(config_mode == setup_master) return TRUE;
		return FALSE;
	}
}

uint8_t host_setup_mode() {
	uint8_t attempts = 0;

	while(1) {
		send_command("load setup_master");
		config_mode = unknown;

		wait(100);

		if(wait_for(&in_setup_master_mode, 100)) break;

		attempts++;
		if (attempts > 10) {
			return FALSE;
		}
	}

	send_command("save");
	wait(100);
	reset_roving();

	return TRUE;
}


uint8_t done_setup() {
	return setup_is_complete && !socket_open;
}

uint8_t get_number_of_slaves() {
	return number_of_slaves;
}


uint8_t add_to_slaves(uint8_t * slave_ip_string) {
	number_of_slaves++;
	uint8_t ip_len = strlen(slave_ip_string) + 1;//+1 for null plug

	uint8_t * ip_copy = malloc(sizeof(uint8_t) * ip_len);
	if(!ip_copy) return FALSE;
	memcpy(ip_copy,slave_ip_string,ip_len);

	node_ref new_slave = new_node(ip_copy, &free);
	if(!new_slave) return FALSE;

	if(!slave_list) slave_list = new_slave;
	else node_append(slave_list, new_slave);

	return TRUE;
}


uint8_t alert_slaves() {
	if(!slave_list) return TRUE;

	node_ref cur_slave_node = slave_list;
	uint8_t * cur_slave_ip = node_get_val(cur_slave_node);

	while(cur_slave_node) {
		send_command_with_arg("set ip host",cur_slave_ip);
		wait(1000);

		while(1) {
			uint8_t counter = 0;

			disconnect();
			if(connect()) {
				wait(100);
				// we are connected, pass the new ssid and password to them
				send_data("\n\r");

				if(new_ssid_flag) {
					send_data("SSID=");
					send_data(new_ssid);
					send_data("\n");
				}
				if(new_pass_flag) {
					send_data("PASS=");
					send_data(new_pass);
					send_data("\n");
				}
				if(new_host_ip_flag) {
					send_data("HOST_IP=");
					send_data(new_host_ip);
					send_data("\n");
				}
				if(new_host_name_flag) {
					send_data("HOST_NAME=");
					send_data(new_host_name);
					send_data("\n");
				}


				// then tell them to connect to the new network
				send_data("DONE SETUP");
				send_data("\n");

				// and close the connection
				wait(100);
				disconnect();
				break;
			} else {
				counter++;
				if(counter > 10) break;
			}
		}

		cur_slave_node = node_get_next(cur_slave_node);
	}

	// Dome with these slaves
	node_delete_list(&slave_list);
	number_of_slaves = 0;

	return TRUE;
}


uint8_t leave_setup_mode() {
	uint8_t attempts = 0;

	while(1) {
		send_command("load main_config");
		config_mode = unknown;

		if(wait_for(&in_main_mode, 100)) break;

		attempts++;
		if(attempts > 5) {
			return FALSE;
		}

	}

	if(new_ssid_flag) {
		memcpy(ssid,new_ssid,strlen(new_ssid)+1);
		send_command_with_arg("set w s",ssid);
	}
	if(new_pass_flag) {
		memcpy(pass,new_pass,strlen(new_pass)+1);
		send_command_with_arg("set w p",pass);
	}
	if(new_host_ip_flag) {
		memcpy(host_ip,new_host_ip,strlen(new_host_ip)+1);
		send_command_with_arg("set i h",host_ip);
	}
	if(new_host_name_flag) {
		memcpy(host_name,new_host_name,strlen(new_host_name)+1);
		send_command("set i h 0"); //have to clear the
		send_command_with_arg("set d n", host_name);
	}

	send_command("save main_config");
	//wait(1000);
	send_command("save");
	wait(1000);
	reset_roving();

	return TRUE;
}

uint8_t do_setup() {

	if(config_mode == setup_slave) {
		never_was_a_slave = FALSE;

		if (!is_associated()) {
			setup_join_attempts++;
			associate();
			if(setup_join_attempts > MAX_SETUP_JOIN_ATTEMPTS) {
				host_setup_mode();
			}
		} else if (!have_dhcp()) {
			get_dhcp();
		} else {
			set_led_anim(led_setup_slave_assoc);
			if(!setup_found_master) {
				setup_connect_attempts++;
				if(setup_connect_attempts > MAX_SETUP_JOIN_ATTEMPTS) {
					host_setup_mode();
				}
				// has not found the master yet
				if(connect()) {
					setup_found_master = TRUE;
					wait(50);
					send_ip();
					wait(50);
					reset_setup_info();
					disconnect();
				}


			} else {
				set_led_anim(led_setup_slave_found_master);

				if(done_setup()) {
					leave_setup_mode();
				}
			}
		}
	} else if (config_mode == setup_master) {
		// This case it taken care of by handling input
		// Once it gets the done command, it will finish here

		if(never_was_a_slave) {
			// We don't want multiple masters on the network
			start_setup_mode();
			return TRUE;
		}

		set_led_anim(led_setup_master_assoc);

		if(done_setup()) {
			wait(1000);
			alert_slaves();
			leave_setup_mode();
			reset_setup_info();
		}

	} else {
		if(!in_setup_mode()) {
			// this case shouldn't exist!!!

		}
	}

	return TRUE;
}



uint8_t is_connected() {
	return socket_open;
}


uint8_t connect() {
	handle_roving_input();
	if(!socket_open) {
		send_command("open");
		wait(200);
		handle_roving_input();
	}
	if(socket_open)	{
		return TRUE;
	} else {
		return FALSE;
	}
}


uint8_t disconnect() {
	uint8_t attempts = 0;

	while(socket_open) {
		send_command("close");
		wait(100);
		handle_roving_input();
		attempts++;
		if(attempts > 10) {
			return FALSE;
		}
	}

	return TRUE;
}


uint8_t is_delimiter(uint8_t character) {
	if (character == '\n') return TRUE;
	if (character == '\r') return TRUE;
	if (character == '*') return TRUE;
	if (character == 0xD) return TRUE; //carriage return
	if (character == '<') return TRUE;
	if (character == '>') return TRUE;
	if (character == 0) return TRUE;
	return FALSE;
}

uint8_t is_prompt(uint8_t character) {
	if (character == '>') return TRUE;
}

void convert_string_to_ip_in_hex(uint8_t * string, uint8_t * ip_addr) {
	uint8_t byte;
	for (byte = 0; byte < 4; byte++) {
		uint8_t val = 0;
		while ((*string != '.') && (*string != 0) && (*string != ':')) {
			val = val*10;
			val = val + (*string-'0');
			string++;
		}
		string++;
		ip_addr[byte] = val;
	}
}


void parse_string(uint8_t * string) {
	uint16_t length = strlen(string); //null plug
	uint8_t * end;
	uint8_t * equals_sign;
	uint8_t str_length;


	if (!command_mode) if(length == 3) if (strcmp("CMD",string) == 0) {
		command_mode = TRUE;
		has_prompt = TRUE;
		return;
	}


	if (command_mode) {

		equals_sign = strchr(string, '=');

		if (equals_sign) {
			equals_sign[0] = 0; //null plug it
			if(strcmp(string,"IP") == 0) {
				end = strchr(&equals_sign[1], ':');
				*end = 0; //null plug
				length = end-equals_sign;
				memcpy(ip,&equals_sign[1],length);
				ip[str_length] = 0;
				return;
			}
			if(strcmp(string,"HOST") == 0) {
				end = strchr(&equals_sign[1], ':');
				*end = 0;
				length = end-equals_sign;
				memcpy(host_ip,&equals_sign[1],length);
				host_ip[str_length] = 0;
				return;
			}
			if(strcmp(string,"SSID") == 0) {
				str_length = length - 4;
				memcpy(ssid,&equals_sign[1],str_length);
				ssid[str_length] = 0;
				return;
			}
			if(strcmp(string,"Assoc") == 0) {
				if (equals_sign[1] == 'O' && equals_sign[2] == 'K') {
					assoc = TRUE;
					return;
				}
				if (equals_sign[1] == 'F' && equals_sign[2] == 'A') {
					assoc = FALSE;
					return;
				}
			}
			if(strcmp(string,"DHCP") == 0) {
				if (equals_sign[1] == 'O' && equals_sign[2] == 'K') {
					dhcp = TRUE;
					return;
				}
				if (equals_sign[1] == 'F' && equals_sign[2] == 'A') {
					dhcp = FALSE;
					return;
				}
				if (equals_sign[1] == 'A' && equals_sign[2] == 'U') {
					// auto ip...
					// Doesn't help
					return;
				}
			}
			if(strcmp(string,"DeviceId") == 0) {
				// we use DeviceId on the WiFly module to act as the name of the mode
				if (strcmp(&equals_sign[1], "main_config") == 0) {
					config_mode = main_config;
					return;
				}
				if (strcmp(&equals_sign[1], "setup_slave") == 0) {
					config_mode = setup_slave;
					return;
				}
				if (strcmp(&equals_sign[1], "setup_master") == 0) {
					config_mode = setup_master;
					return;
				}


			}
			equals_sign[0] = '='; //revert it
		}

		if(strcmp("ERR:Connected!", string) == 0) {
			wait(4000);
			//TODO Handle this case better
			// Wait for the timeout
			return;
		}

		if(strcmp("Connect FAILED", string) == 0) {
			wait(100);
			//TODO Handle this case better
			return;
		}

		if(strcmp("EXIT",string) == 0) {
			command_mode = FALSE;
			return;
		}

		if(strcmp("Associated!",string) == 0) {
			assoc = TRUE;
			return;
		}
	}

	if (socket_open) {
			if(string[0] == '@') {
				//This is a command to the host module
				parse_shemp_command(&string[1], length-1);
				return;
			}


			if(string_starts_with("GET /",string)) {
				// HTTP Setup request!
				parse_http_request(string);
				return;
			}


			if (strcmp("CLOS",string) == 0) {
				socket_open = FALSE;
				call_back(DISCONNECT_EVENT);
				return;
			}
			if (strcmp(ACK_STRING,string) == 0) {
				have_ack_flag = TRUE;
				return;
			}
			if (strcmp("DONE SETUP",string) == 0) {
				complete_setup();
				return;
			}


			equals_sign = strchr(string, '=');
			if (equals_sign) {
				equals_sign[0] = 0; //null plug it
				end = &string[length];
				length = end-&equals_sign[1];

				if(strcmp(string,"SSID") == 0) {
					add_new_ssid(&equals_sign[1],length);
					return;
				}
				if(strcmp(string,"PASS") == 0) {
					add_new_pass(&equals_sign[1],length);
					return;
				}
				if(strcmp(string,"HOST_IP") == 0) {
					add_new_host_ip(&equals_sign[1],length);
					return;
				}
				if(strcmp(string,"HOST_NAME") == 0) {
					add_new_host_name(&equals_sign[1],length);
					return;
				}
				if(strcmp(string,"SLAVE_IP") == 0) {
					add_to_slaves(&equals_sign[1]);
					return;
				}
				equals_sign[0] = '='; //revert it
			}
		}





	if(string_starts_with("Disconn",string)) {
		assoc = FALSE;
		dhcp = FALSE;
		return;
	}


	if (strcmp("OPEN",string) == 0) {
		command_mode = FALSE;
		socket_open = TRUE;
		call_back(CONNECT_EVENT);
		return;
	}



}

void handle_roving_input() {
	if (newline == FALSE) {
		return;
	}
	newline = FALSE;

	uint16_t temp_read_index = read_index; // we use temp_read_index because it fixes recursion issues
	uint16_t cur_index = read_index; //start where we left off
	uint16_t char_count = 0; //want to keep track of how many characters in a line

	// While there is more to read
	while(cur_index != write_index) {

		if(cur_index >= BUFFER_SIZE) {
			cur_index = 0;
			read_index = 0;
		}


		// If we hit a delimiter (new line, etc), then parse it
		if (is_delimiter(input_buffer[cur_index])) {
			input_buffer[cur_index] = 0; // null plug
			if (char_count > 0) {
				// Parse the line

				if(read_index+char_count >= BUFFER_SIZE) { //over the end...
					// If it overflowed, just start over...
					char_count = 0;
					cur_index = 0;
					read_index = cur_index;
					continue;
				}

				if (cur_index >= (BUFFER_SIZE-256)) {
					// This is in the overflow buffer area, reset stuff
					temp_read_index = read_index;
					cur_index = 0;
					read_index = cur_index;
					parse_string(&input_buffer[temp_read_index]);
					char_count = 0;

				} else {
					// This is a good one, just read it
					temp_read_index = read_index;
					cur_index++;
					read_index = cur_index;
					parse_string(&input_buffer[temp_read_index]);
					char_count = 0;

				}

			} else {
				// Empty line, move on
				char_count = 0;
				cur_index++;
				read_index = cur_index;
			}

			if(cur_index > (BUFFER_SIZE-256)) {
				cur_index = 0;
				read_index = 0;
				char_count = 0;
			}

		} else {
			// Not done yet, keep incrementing
			char_count++;
			cur_index++;
		}
	}



}


void store_uart_input(uint8_t input) {
	if (write_index >= BUFFER_SIZE) write_index = 0;  //TODO Shouldn't need this, but it is here because of a glitch
	if (read_index >= BUFFER_SIZE) read_index = 0;

	input_buffer[write_index] = input;  	// store input
	write_index++;

	if (is_prompt(input)) {
		has_prompt = TRUE;
	}

	if (is_delimiter(input)) {
		newline = TRUE;	// something new to parse
		if(write_index > (BUFFER_SIZE-256)) write_index=0;
	}

	if (write_index >= BUFFER_SIZE) write_index = 0;  //increment and wrap
	//if (write_index == read_index) read_index++; //push it away, just in case
}



