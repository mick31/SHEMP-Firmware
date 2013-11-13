/*
 * html.h
 * Author: Nathan Abercrombie
 *
 * This module is simply for the purpose of parseing HTTP requests and responding
 * with a HTML page.
 */

#ifndef HTML_H_
#define HTML_H_

#include <inttypes.h>

#include "auxlib.h"
#include "uart.h"
#include "roving.h"

void parse_http_request(uint8_t * request);

#endif /* HTML_H_ */
