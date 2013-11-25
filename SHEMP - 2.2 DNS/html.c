/*
 * html.c
 *
 *  Created on: Sep 10, 2012
 *      Author: nabercro
 */
#include "html.h"

// This is so the preprocesser can calculate strlens.  It must be used on uint8_t[], not uint8_t *
#define STRLEN(s) ((sizeof(s)/sizeof(s[0])) - 1)

// This is the header of the HTTP response.  The Content-Length must be changed to the sum of strlen for all that follows, see html_send()
const uint8_t http_header[] = "HTTP/1.1 200 OK\nContent-Length: 000000\nConnection: close\nContent-Type: text/html\n\n";
#define CONTENT_LENGTH_POS 32

// This is the header of the HTML page.  It provides structure and styling.
// Had to break it into parts, so that i wouldn't get the warning
// "Integer conversion resulted in truncation"
const uint8_t html_header_01[] = "<html><title>SHEMP Configuration</title><body style='text-align:center; font-family:Verdana; color: #444; padding:30px;'>";
const uint8_t html_header_02[] = "<form><table style='margin:auto; background-color: #f5f5f5; padding: 10px; border-style: solid; border-width: 5px; border-color: #eee;'>";
const uint8_t html_header_03[] = "<tr><td colspan=2 style='text-align:center; font-size:1.1em'>Smart Home Energy Monitoring Plug<td></tr>";

// These is the instructions, they appear at the top of the page depending on what state it is in
const uint8_t html_instructions_01[] = "<tr><td colspan=2 style='text-align:center; font-size:0.9em'>Enter your WiFi Info and press 'Submit'</td></tr>";
const uint8_t html_instructions_02[] = "<tr><td colspan=2 style='text-align:center; font-size:0.9em'>Please wait while the devices configure</td></tr>";

// Number of slaves
const uint8_t html_number_of_slaves_prefix[] = "<tr><td colspan=2 style='text-align:center; font-size: 0.9em'>Number of devices to configure: ";
uint8_t html_number_of_slaves_field[] = "000000";
const uint8_t html_number_of_slaves_suffix[] = "</td></tr>";


// These are the form rows, must put in all 3 for each item
// The _form entry is removed and replaced with the new value for confirmation
const uint8_t html_ssid_prefix[] = "<tr><td style='text-align:right'>WiFi SSID:</td><td>";
const uint8_t html_ssid_form[] = "<input name=SSID type=text>";
const uint8_t html_ssid_suffix[] = "</td></tr>";

const uint8_t html_pass_prefix[] = "<tr><td style='text-align:right'>WiFi Password:</td><td>";
const uint8_t html_pass_form[] = "<input name=PASS type=text>";
const uint8_t html_pass_suffix[] = "</td></tr>";

//Removed so traffic only goes through shempserver.com
const uint8_t html_host_ip_prefix[] = "<tr><td style='text-align:right'>Server:</td><td>";
const uint8_t html_host_ip_form[] = "<input name=HOST type=text>";
const uint8_t html_host_ip_suffix[] = "</td></tr>";


// Error, if needed
const uint8_t html_input_error[] = "<br>Invalid Input";



// This is the submit button
const uint8_t html_submit_button[] = "<tr><td colspan=2 style='text-align:right'><input type=submit value='Submit'><input style='float:left' type=submit name=reset value='Reset'></td></tr>";



// And lastly, the footer
const uint8_t html_footer[] = "</table></form></body></html>";

// All of this is written to the HTML buffer, so make sure it is large enough
#define HTML_BUFFER_SIZE 1500
uint8_t html_buffer[HTML_BUFFER_SIZE];
uint8_t * html_write_ptr;
uint16_t html_page_length;
uint16_t http_header_length;

#define MAX_VARCHAR_LENGTH 32
uint8_t temp_varchar[MAX_VARCHAR_LENGTH];

void html_reset() {
	html_write_ptr = html_buffer;
	html_page_length = 0;
	http_header_length = 0;
}

void http_write(uint8_t * string, uint8_t string_length) {
	// Make sure it does not overflow
	if(http_header_length + html_page_length + string_length >= HTML_BUFFER_SIZE) {
		return;
	}

	// Add it to the end
	memcpy(html_write_ptr, string, string_length);

	// Update the end
	html_write_ptr = &html_write_ptr[string_length];
	http_header_length += string_length;

}


void html_write(uint8_t * string, uint8_t string_length) {
	// Make sure it does not overflow
	if(http_header_length + html_page_length + string_length >= HTML_BUFFER_SIZE) {
		return;
	}

	// Add it to the end
	memcpy(html_write_ptr, string, string_length);

	// Update the end
	html_write_ptr = &html_write_ptr[string_length];
	html_page_length += string_length;
}

void html_send() {
	// fix the content length ptr
	write_int_to_string(&html_buffer[CONTENT_LENGTH_POS],html_page_length);

	// and add a null plug to the end
	html_write_ptr[0] = 0;

	// Then transmit it
	send_data((uint8_t * )html_buffer);
}

// This function parses the src_string for the key
// To simplify this function, key has to have the = sign in it
// for example:  html_GET("SSID=");
uint8_t html_GET(uint8_t * destination, uint8_t * src, uint8_t * key) {
	uint8_t * value = 0;
	if(!src || !key) return FAILURE;

	uint8_t * cur_ptr = src;

	// Here we search for the existence of the key in the string
	while(*cur_ptr != 0) {
		if(string_starts_with(key,cur_ptr)) {
			value = &cur_ptr[strlen(key)];
			break;
		}
		cur_ptr = &cur_ptr[1];
	}
	if(!value) return FAILURE;

	// With the key found, we must terminate the string
	cur_ptr = value;
	uint8_t temp_character = 0;
	while(cur_ptr) {
		if(*cur_ptr == '?') break;
		if(*cur_ptr == 0) break;
		if(*cur_ptr == '&') break;
		if(*cur_ptr == ' ') break;
		cur_ptr++;
	}

	temp_character = *cur_ptr;
	*cur_ptr = 0;

	if(strlen(value) == 0) return FAILURE;


	if(destination) {
		if(strlen(value)+1 < MAX_VARCHAR_LENGTH) memcpy(destination, value, strlen(value)+1); //+1 for null plug
		else return FAILURE;
	}

	*cur_ptr = temp_character;

	return SUCCESS;
}



void parse_http_request(uint8_t * request) {
	uint8_t ssid_error = FALSE;
	uint8_t pass_error = FALSE;
	uint8_t host_ip_error = FALSE;

	// First, have to parse it
	if(html_GET(temp_varchar, request, "SSID=")) {
		if(is_valid_string(temp_varchar)) {
			add_new_ssid(temp_varchar, strlen(temp_varchar));
		} else {
			ssid_error = TRUE;
		}
	}
	if(html_GET(temp_varchar, request, "PASS="))  {
		if(is_valid_string(temp_varchar)) {
			add_new_pass(temp_varchar, strlen(temp_varchar));
		} else {
			pass_error = TRUE;
		}
	}
	if(html_GET(temp_varchar, request, "HOST="))  {
		if(is_valid_string(temp_varchar)) {
			if(number_of_characters_in_string(temp_varchar, '.') == 3) {
				// If the number of '.' in the host is equal to 3
				add_new_host_ip(temp_varchar, strlen(temp_varchar));
			} else {
				add_new_host_name(temp_varchar, strlen(temp_varchar));
			}

		} else {
			host_ip_error = TRUE;
		}
	}

	if(html_GET(temp_varchar, request, "reset=")) {
		reset_setup_info();
	}



	html_reset();

	// We use HTTP write instead of HTML write, because this is the HTTP header
	http_write((uint8_t *)http_header, STRLEN(http_header));

	// Had to be broken into multiple parts
	html_write((uint8_t *)html_header_01, STRLEN(html_header_01));
	html_write((uint8_t *)html_header_02, STRLEN(html_header_02));
	html_write((uint8_t *)html_header_03, STRLEN(html_header_03));


	if(have_new_ssid() && have_new_pass() && have_new_host_ip()) {
		html_write((uint8_t *)html_instructions_02, STRLEN(html_instructions_02));
	} else {
		html_write((uint8_t *)html_instructions_01, STRLEN(html_instructions_01));
	}

	if(get_number_of_slaves()) {
		html_write((uint8_t *)html_number_of_slaves_prefix, STRLEN(html_number_of_slaves_prefix));
		write_int_to_string(html_number_of_slaves_field,get_number_of_slaves()+1);
		html_write((uint8_t *)html_number_of_slaves_field, strlen(html_number_of_slaves_field));
		html_write((uint8_t *)html_number_of_slaves_suffix, STRLEN(html_number_of_slaves_suffix));
	}


	// SSID Row
	html_write((uint8_t *)html_ssid_prefix, STRLEN(html_ssid_prefix));
	if(have_new_ssid()) {
		html_write((uint8_t *)get_new_ssid(), strlen(get_new_ssid()));
	} else {
		html_write((uint8_t *)html_ssid_form, STRLEN(html_ssid_form));
		if(ssid_error) html_write((uint8_t *)html_input_error, STRLEN(html_input_error));
	}
	html_write((uint8_t *)html_ssid_suffix, STRLEN(html_ssid_suffix));



	// PASS Row
	html_write((uint8_t *)html_pass_prefix, STRLEN(html_pass_prefix));
	if(have_new_pass()) {
		html_write((uint8_t *)get_new_pass(), strlen(get_new_pass()));
	} else {
		html_write((uint8_t *)html_pass_form, STRLEN(html_pass_form));
		if(pass_error) html_write((uint8_t *)html_input_error, STRLEN(html_input_error));
	}
	html_write((uint8_t *)html_pass_suffix, STRLEN(html_pass_suffix));


	// PASS Row
	html_write((uint8_t *)html_host_ip_prefix, STRLEN(html_host_ip_prefix));
	if(have_new_host_ip()) {
		html_write((uint8_t *)get_new_host_ip(), strlen(get_new_host_ip()));
	} else if (have_new_host_name()) {
		html_write((uint8_t *)get_new_host_name(), strlen(get_new_host_name()));
	} else {
		html_write((uint8_t *)html_host_ip_form, STRLEN(html_host_ip_form));
		if(host_ip_error) html_write((uint8_t *)html_input_error, STRLEN(html_input_error));
	}
	html_write((uint8_t *)html_host_ip_suffix, STRLEN(html_host_ip_suffix));

	if(have_new_ssid() && have_new_pass() && (have_new_host_ip() || have_new_host_name())) {
		complete_setup();
	} else {
		html_write((uint8_t *)html_submit_button, STRLEN(html_submit_button));
	}





	html_write((uint8_t *)html_footer, STRLEN(html_footer));

	html_send();

	return;
}
