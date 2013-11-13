/*
 * main.c
 * Author: Nathan Abercrombie
 *
 * Smart Home Energy Monitoring Project
 * Main module
 *
 * There are three main threads:
 * main while(1) loop
 * 12kHz interrupt
 * "non_blocking_interrupt"
 *
 * The main loop handles all communications with WiFi and the Roving Module.
 * It transmits any data needed to be transmitted, and handles all changes in
 * settings and state.
 *
 * The 12kHz interrupt samples the sensors and keeps time.  It also spawns the
 * non_blocking_interrupt thread
 *
 * The non_blocking_interrupt handles sensors when their data array is full, it
 * may calculate wattage, look for transients, or simply put the data in the outbox
 * to be transmitted.
 *
 * I had to implement it as 3 threads because the 12kHz MUST always happen.  The calculations
 * in the non_blocking_interrupt thread can take longer than one period of 12kHz, and so it
 * cannot go into there.  Some sensors need to be calculated multiple times a second, but some
 * communication in the main loop takes longer than a second, so the calculations can not
 * go there.
 *
 * I have commented throughout to explain this program, however, the overall process is:
 * 1. Initialize variables
 * 2. Start the Roving Module
 * 3. Connect to the network, and then the server
 * 4. Get settings from the server for the sensors
 * 5. Create the sensor structs
 * 6. Sample them
 * 7. Calculate wattage, etc
 * 8. Transmit it back to the server
 *
 * You can just skip down to MAIN, everything above it is helper functions
 *
 * Note: there are //TODO statements where I think more work could be done
 */

#include <msp430.h>
#include <inttypes.h>
#include  "hal_pmm.h"

#include "events.h"
#include "sensors.h"
#include "time.h"
#include "roving.h"
#include "uart.h"
#include "leds.h"
#include "shempserver.h"

#include "wattage_sensor.h"
#include "temperature_sensor.h"
#include "audio_sensor.h"
#include "light_sensor.h"

uint8_t inside_non_blocking_interrupt;
uint8_t server_wants_header;
uint8_t okay_to_transmit;
time_ref last_connect_time;
time_ref check_in_period;
uint8_t setup_button_pressed;
time_ref setup_button_time_trigger;
time_ref setup_button_time_duration;

// This is for handling AUX ports
#define NUMBER_OF_AUX_PORTS 2
typedef uint8_t (*Function) (void);

sensor_ref aux_sensor[NUMBER_OF_AUX_PORTS];
uint8_t aux_sensor_enabled[NUMBER_OF_AUX_PORTS];

//TODO REMEMBER TO CHANGE HEAP SIZE IN LINKER OPTIONS TO SOMETHING LARGE
// Stack: 0x300 = 768 //default 160
// Heap: 0x1800 = 6144 //default 160

void init_clock() {
//TODO: Implement correct usage of watchdog
	WDTCTL = WDTPW+WDTHOLD;

	// Raise the internal Core voltage to enable higher clock rates
	SetVCore(PMMCOREV_3);

	__bis_SR_register(SCG0);                  // Disable the FLL control loop


	//UCSCTL0 = DCO0 * 20;					// 24mHz - 204 uart divider
	//UCSCTL0 = DCO0 * 19;					// 22.2 mHz - 193 uart divider
	//UCSCTL0 = DCO0 * 18;					// 20.6 mHz - 179 uart divider
	UCSCTL0 = DCO0 * 16;					// 18mHz - 156 uart divider
	//UCSCTL0 = DCO0 * 8;					// 11mHz - 96 uart divider
	UCSCTL1 = DCORSEL_7;					// Set it for the fastest

	__delay_cycles(100000);  				// So it can settle in, maybe unneeded
}


void init_timer() {
	TA0CTL = 0; //halt it
	TA0CCTL0 = 0; //reset it
	TA0CTL = TASSEL_1 + MC_2; //ACLK - 32khz
}

void init_pll() {
	P2IFG &= ~BIT3; //clear flags
	P2IE |= BIT3; //enable interrupts
	P2DIR &= ~BIT3; //set direction

	P2DIR |= BIT2; //set directions
}

#define RELAY_PDIR P8DIR
#define RELAY_POUT P8OUT
#define RELAY_PIN	BIT4
void init_relay() {
	RELAY_PDIR |= RELAY_PIN;
	RELAY_POUT &= RELAY_PIN; // Active low
}
#define ON 1
#define OFF 0
void set_relay(uint8_t state) {
	if(state) { // On
		RELAY_POUT &= ~RELAY_PIN;
	} else {
		RELAY_POUT |= RELAY_PIN;
	}
}

void toggle_relay() {
	RELAY_POUT ^= RELAY_PIN;
}

//TODO Better debounce
#define BUTTON_2_DOWN (!(P1IN & BIT7))
#define BUTTON_2_UP (P1IN & BIT7)
void button1_toggled() {
	toggle_relay();
}

void button2_toggled() {
	if(BUTTON_2_DOWN) {
		if(!setup_button_pressed) {
			time_set_to_sum(setup_button_time_trigger, setup_button_time_duration, global_time());
			setup_button_pressed = TRUE;
		}
	} else {
		setup_button_pressed = FALSE;
	}
}

void init_buttons() {
	//INIT TIP SWITCHES
	//TIP 1 - P1.0
	//TIP 2 - P1.1
	//SW1  - P1.6
	//SW2  - P1.7
	P1IE |= BIT0 + BIT1 + BIT6 + BIT7; //enable interrupts
	P1IFG &= ~BIT0 + BIT1 + BIT6 + BIT7; //clear flags
	P1DIR &= ~BIT0 + BIT1 + BIT6 + BIT7; //input direction
	P1IES |= BIT0 + BIT1 + BIT6 + BIT7;  // Rising edge?
	P1SEL &= ~(BIT0 + BIT1 + BIT6 + BIT7); // No other functions

	aux_sensor_enabled[0] = P1IN & BIT1; //port a
	aux_sensor_enabled[1] = P1IN & BIT0; //port b
	if(aux_sensor_enabled[0]) {
		P1IES ^= BIT1;
	}
	if(aux_sensor_enabled[1]) {
		P1IES ^= BIT0;
	}


	if(BUTTON_2_DOWN) {
		button2_toggled();// button is pressed
	}



}

uint8_t connect_and_send_header(node_ref args) {
	uint8_t ret = TRUE;
	if(!is_connected()) {
		if(connect()) {
			transmit_header();
			ret = SUCCESS;
		} else {
			ret = FAILURE;
		}
	}
	return ret;
}


uint8_t roving_call_back(uint8_t event) {
	// This is called when roving sends the disconnect event
	//TODO implement more call backs
	if(event == DISCONNECT_EVENT) {
		okay_to_transmit = FALSE;
		server_wants_header = FALSE;
		time_set_current(last_connect_time);
		add_time_to_time(last_connect_time, check_in_period);
	}
	return SUCCESS;
}



/*
 * ****************************************************************************
 *
 *
 * MAIN
 *
 * ****************************************************************************
 */

#define MAIN_MODE_INIT 0
#define MAIN_MODE_SETUP 1
#define MAIN_MODE_SAMPLE 2
uint8_t main_mode;

void main(void) {
	// Have to init the clock first
	init_clock();

	// These initialize variables
	// Mostly just zeroing them
	okay_to_transmit = FALSE;
	server_wants_header = FALSE;
	inside_non_blocking_interrupt = FALSE;
	last_connect_time = 0;
	check_in_period = 0;
	setup_button_pressed = FALSE;
	setup_button_time_trigger = 0;
	setup_button_time_duration = 0;
	main_mode = MAIN_MODE_INIT;
	init_internal_wattage_sensor();
	init_temperature_sensor();
	init_audio_sensor();
	init_light_sensor();
	uint8_t itor = 0;
	for (itor = 0; itor < NUMBER_OF_AUX_PORTS; itor++) {
		aux_sensor[itor] = 0;
	}

	// These initialize functions and interrupts
	init_timer();
	init_time();
	init_leds();
	init_relay();
	init_uart();
	init_transmits();
	init_buttons();
	init_roving(&roving_call_back);

	setup_button_time_trigger = new_time();
	setup_button_time_duration = new_time();
	time_set_seconds(setup_button_time_duration, 2);

	_enable_interrupts();

	// confirm roving works
	while (!enter_command_mode()) {
		reset_roving();
		wait(500);
	}
	init_adc();
	init_pll();

	// This is for the check in
	// If we are disconnected, we wait 5 seconds before trying to reconnect
	last_connect_time = new_time();
	check_in_period = new_time();
	time_set_seconds(check_in_period, 5);


	// And go!!!

	set_led_anim(led_start);

	// MAIN LOOP
	while(1) {
		handle_roving_input();

		// Check if it a long hold
		if(setup_button_pressed) {
			// If the button is pressed down
			if(time_cmp(global_time(), setup_button_time_trigger) >= 0) {
				// If enough time has passed
				if(in_setup_mode()) {
					leave_setup_mode();
				} else {
					start_setup_mode();
				}
				// Only want to check this once
				setup_button_pressed = FALSE;
			}
		}


		// MAIN BEHAVIOR
		// There is setup_mode and main_mode.
		// Setup mode is for configuring WiFi SSID and PASS
		// Main mode is for sampling
		if (in_setup_mode()) {
			if(main_mode != MAIN_MODE_SETUP) {
				stop_sampling();
				set_led_anim(led_setup_start);
				main_mode = MAIN_MODE_SETUP;
			}
			do_setup();

		} else if (in_main_mode()) { //regular mode
			if(main_mode != MAIN_MODE_SAMPLE) {
				set_led_anim(led_main_start);
				start_sampling();
				main_mode = MAIN_MODE_SAMPLE;
			}

			if(!is_associated()) {
				associate();
			} else if (!have_dhcp()) {
				get_dhcp();
			} else {
				if(is_connected()) set_led_anim(led_main_connected);
				else set_led_anim(led_main_assoc);

				if(!is_connected() && (time_cmp(global_time(), last_connect_time) >= 0) ) {
					// If we are not connected, and enough time has passed, connect
					connect();
					add_time_to_time(last_connect_time, check_in_period);
				}

				if(server_wants_header) {
					led_ping();
					exit_command_mode();
					transmit_header();
					wait(100);
				}

				if(okay_to_transmit && have_data_to_transmit()) {
					led_ping();
					exit_command_mode();
					transmit_data();
				}


			}
		} else {
			main_mode = MAIN_MODE_INIT;
			set_led_anim(led_error);
			stop_sampling();
		}
	}
}




void do_non_blocking_interrupt() {
	_enable_interrupts();
	// Because we have re-enabled interrupts in here, the 12kHz interrupt can happen again
	// But because we set the flag "inside_non_blocking_interrupt", this will not get called
	// again until this pops off the stack.
	// Essentially, this function gets called constantly, but only one will exist on the stack
	// at the same time.

	handle_full_sensors();

	_disable_interrupts();
}

// This is the 12kHz thread.
#pragma vector=PORT2_VECTOR
__interrupt void Port2GPIOHandler(void)
{
	static uint8_t pll_count;
	if (P2IFG & BIT3) {

		// PLL FEEDBACK
		P2IFG &= ~BIT3;
		pll_count++;
		if (pll_count == 100) {
			P2OUT |= BIT2;
		}
		if (pll_count >= 200) {
			P2OUT &= ~BIT2;
			pll_count = 0;
		}

		//TICK THE TIME
		time_tick();
		run_led_driver();

		if (ready_to_sample()) {
			sample_enabled_sensors();
		}
	}

	if(inside_non_blocking_interrupt == FALSE) {
		inside_non_blocking_interrupt = TRUE;
		do_non_blocking_interrupt();
		inside_non_blocking_interrupt = FALSE;
	}
}

// ADC10 interrupt service routine
// This is called when the ADC completes is sampleing
#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
	handle_adc_interrupt();
}

// This interrupt is for all the buttons / tip switches
#pragma vector=PORT1_VECTOR
__interrupt void Port1GPIOHandler(void)
{
	if (P1IFG & BIT0) {
		//TIP1
		P1IFG &= ~BIT0;

		//Toggle it
		aux_sensor_enabled[1] = !aux_sensor_enabled[1];
		if(aux_sensor_enabled[1]) {
			sensor_enable_this(aux_sensor[1]);
		} else {
			sensor_disable_this(aux_sensor[1]);
		}

		P1IES ^= BIT0; //toggle direction
		//PORTB
	}
	if(P1IFG & BIT1) {
		//TIP2
		P1IFG &= ~BIT1;

		// Toggle it
		aux_sensor_enabled[0] = !aux_sensor_enabled[0];
		if(aux_sensor_enabled[0]) {
			sensor_enable_this(aux_sensor[0]);
		} else {
			sensor_disable_this(aux_sensor[0]);
		}

		P1IES ^= BIT1; //toggle direction
		//PORTA
	}
	if(P1IFG & BIT6) {
		//SW1
		P1IFG &= ~BIT6;
		button1_toggled();
	}
	if(P1IFG & BIT7) {
		//SW2
		P1IFG &= ~BIT7;
		button2_toggled();
		P1IES ^= BIT7; //toggle direction
	}
}


/* When the roving module receives a packet that starts with @, it passes the rest
 * of the command to this function.
 */
uint8_t parse_shemp_command(uint8_t * string, uint16_t length) {
	// TODO Make this entire function much more modular

	uint16_t cur_index = 0;

	if(string[cur_index] == 'W') {
		// Wait
		cur_index++;
		uint16_t wait_ms = read_number_from_string(&string[cur_index], 4);
		wait(wait_ms);
		// And then clear output buffer
		reset_output_buffer();

	} else if(string[cur_index] == 'R') {
		// Relay
		cur_index++;
		if (string[cur_index] == 'N') {
			set_relay(ON);
		} else if (string[cur_index] == 'F') {
			set_relay(OFF);
		}
		cur_index++;
	} else if (string[cur_index] == 'H') {
		// Sync with header
		cur_index++;
		server_wants_header = TRUE;
		okay_to_transmit = FALSE;
		//transmit_header();
	} else if(string[cur_index] == 'K') {
		// Okay to transmit
		cur_index++;
		server_wants_header = FALSE;
		okay_to_transmit = TRUE;
	} else if(string[cur_index] == 'S') {
		cur_index++;
		// Sensor configuration

		uint8_t sensor_loc = 0;
		uint8_t sensor_type = 0;
		uint8_t sensor_state = 0;

		time_ref sensor_period = 0;
		uint16_t sensor_size = 0;

		uint8_t sensor_index = 0;

		#define ENABLED 'E'
		#define DISABLED 'D'

		while(cur_index < length) {
			if(string[cur_index] == 'L') {
				cur_index++;
				// Location
				sensor_loc = string[cur_index];
				cur_index++;
			} else if(string[cur_index] == 'T') {
				cur_index++;
				// Type
				sensor_type = string[cur_index];
			 	cur_index++;
			} else if(string[cur_index] == 's') {
				cur_index++;
				// State
				if(string[cur_index] == 'E') {
					cur_index++;
					sensor_state = ENABLED;
				} else if (string[cur_index] == 'D') {
					cur_index++;
					sensor_state = DISABLED;
				}
			} else if(string[cur_index] == 'P') {
				cur_index++;
				sensor_period = new_time_from_string(&string[cur_index]);
				cur_index += STRING_TO_TIME_LENGTH;
			} else 	if(string[cur_index] == 'C') {
				cur_index++;
				sensor_size = read_number_from_string(&string[cur_index], 5);
				cur_index += 5;
			}
		}

		// Command is parsed, now to determine how to do it
		if (sensor_loc == 'I') {
			// Internal, internal voltage, current, or wattage
			if (sensor_type == 'W') {
				// Wattage can be enabled and disabled and period changed
				if (sensor_state == DISABLED) disable_internal_wattage_sensor();
				else if (sensor_state == ENABLED) {
					// Else it should be enabled
					if (sensor_period && sensor_size) {
						// We have all the needed information
						create_internal_wattage_sensor(sensor_period, sensor_size);
						wait(1); //This ensures that all 3 sensors involved are in sync
						enable_internal_wattage_sensor();
					}
				}
			} else if (sensor_type == 'V') {
				// Handle internal voltage sensor
				if (sensor_state == DISABLED) disable_internal_voltage_sensor();
				else if (sensor_state == ENABLED) {
					// Else it should be enabled
					if (sensor_period && sensor_size) {
						// We have all the needed information
						create_internal_voltage_sensor(sensor_period, sensor_size);
						wait(1);
						enable_internal_voltage_sensor();
					}
				}
			} else if (sensor_type == 'I') {
				// Handle internal current sensor
				if (sensor_state == DISABLED) disable_internal_current_sensor();
				else if (sensor_state == ENABLED) {
					// Else it should be enabled
					if (sensor_period && sensor_size) {
						// We have all the needed information
						create_internal_current_sensor(sensor_period, sensor_size);
						wait(1);
						enable_internal_current_sensor();
					}
				}
			} else if (sensor_type == 'T') {
				// Handle internal temperature sensor
				// It can be enabled, disabled, period and size changed
				if (sensor_state == DISABLED) disable_internal_temperature_sensor();
				else if (sensor_state == ENABLED) {
					// Else it should be enabled
					if (sensor_period && sensor_size) {
						// We have all the needed information
						create_temperature_sensor('I',sensor_period, sensor_size);
						wait(1);
						enable_internal_temperature_sensor();
					}
				}
			}

		} else if (sensor_loc == 'A' || sensor_loc == 'B') {
			sensor_index = sensor_loc - 'A';

			if(sensor_state == DISABLED) {
				// Should disable that sensor if it matches
				if(sensor_type == sensor_get_type(aux_sensor[sensor_index])) {
					sensor_disable_this(aux_sensor[sensor_index]);
				}

			} else if (sensor_state == ENABLED) {
				if(sensor_period && sensor_size) {
					// We have everything we need, but maybe we are already set up, do we have to change anything?

					if(sensor_type != sensor_get_type(aux_sensor[sensor_index]) || time_cmp(sensor_period, sensor_get_period(aux_sensor[sensor_index])) || sensor_size != sensor_get_size(aux_sensor[sensor_index])) {
						// It is a different type
						// Or a different period
						// Or a different size
						sensor_delete_this(aux_sensor[sensor_index]);
						aux_sensor[sensor_index] = 0;
					}

					sensor_ref s;

					if(sensor_type == 'T') {
						// External Temperature Sensor
						s = create_temperature_sensor(sensor_loc, sensor_period, sensor_size);
					}
					else if(sensor_type == 'A') {
						// External Audio Sensor
						s = create_audio_sensor(sensor_loc, sensor_period, sensor_size);
					}
					else if(sensor_type == 'L') {
						// External Light Sensor
						s = create_light_sensor(sensor_loc, sensor_period, sensor_size);
					}

					if(s) {
						aux_sensor[sensor_index] = s;
						if(aux_sensor_enabled[sensor_index]) sensor_enable_this(aux_sensor[sensor_index]);
					}
				}
			}
		}
		time_delete(&sensor_period);
	}
	return SUCCESS;
}








