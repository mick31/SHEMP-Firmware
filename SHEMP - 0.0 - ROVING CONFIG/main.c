/*
 * main.c
 */

#include "uart.h"
#include "hal_pmm.h"

void init_clock() {
	WDTCTL = WDTPW+WDTHOLD;

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




void wait(uint16_t wait_ms) {
	uint16_t wait_ticks = wait_ms*32; // because it is sourced from a 32kHz clock
	uint16_t start_time = TA0R;

	while(1) {
		uint16_t cur_time = TA0R;
		if ((cur_time - start_time) > wait_ticks) break;
	}
}

uint16_t write_index;

uint8_t have_carrot;


uint8_t send_command(uint8_t * string) {
	while(!have_carrot);
	have_carrot = 0;

	uart_send_string(string);
	uart_send_string("\n\r");
}


void main(void) {


	write_index = 0;

	// Have to init the clock first
	init_clock();
	init_uart_9600();
	init_timer();


	_enable_interrupts();
	// Init roving
	P9DIR |= BIT6;//Reset pin direction
	P9OUT &= ~BIT6;
	wait(100);
	write_index = 0;
	P9OUT |= BIT6;

	wait(2000);
	//TODO add factory reset also
	uart_send_string("$$$");
	wait(1000);
	have_carrot = 1;
	send_command("");

	send_command("set sys printlevel 1");
	send_command("set ip protocol 2");
	send_command("set uart mode 0x01");
	send_command("set uart baud 115200");
	send_command("set comm remote 0");
	send_command("set ip tcp-mode 0x3");

	send_command("set wlan ssid shempconfig");
	send_command("set wlan pass 0");
	send_command("set ip netmask 255.255.0.0");
	send_command("set ip remoteport 80");
	send_command("set ip listenport 80");

	send_command("set ip host 169.254.1.1");
	send_command("set ip dhcp 2");
	send_command("set wlan join 1");
	send_command("set opt deviceid setup_slave");
	send_command("save setup_slave");

	send_command("set ip dhcp 0");
	send_command("set ip address 169.254.1.1");
	send_command("set wlan join 4");
	send_command("set wlan channel 1");
	send_command("set opt deviceid setup_master");
	send_command("save setup_master");


	send_command("set ip dhcp 1");
	send_command("set ip host 192.168.1.105");
	send_command("set ip remote 9000");
	send_command("set ip listenport 9000");
	send_command("set wlan join 1");
	send_command("set wlan channel 0");
	send_command("set wlan ssid SHEMP_NETWORK");
	send_command("set wlan pass teammantey");
	send_command("set options deviceid main_config");
	send_command("save main_config");

	send_command("load setup_slave");
	send_command("save");
	send_command("reboot");

	// SET BREAKPOINT HERE!
	while(1);
}

uint8_t input_buffer[2000];
void store_uart_input(uint8_t input) {
	if(write_index >= 2000) write_index = 0;
	input_buffer[write_index] = input;
	write_index++;
	if(input == '<') have_carrot = 1;
}
