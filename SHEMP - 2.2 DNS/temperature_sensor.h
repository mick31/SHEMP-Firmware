/*
 * temperature_sensor.h
 *
 *  Created on: Jul 25, 2012
 *      Author: nabercro
 */

#ifndef TEMPERATURE_SENSOR_H_
#define TEMPERATURE_SENSOR_H_


#include "sensors.h"
#include "events.h"
#include <inttypes.h>
#include "bool.h"
#include "shempserver.h"


void init_temperature_sensor();
sensor_ref create_temperature_sensor(uint8_t loc, time_ref period, uint16_t size);
uint8_t enable_internal_temperature_sensor();
uint8_t disable_internal_temperature_sensor();
uint8_t delete_internal_temperature_sensor();
uint16_t get_celcius(uint16_t temperature_reading);













#endif /* TEMPERATURE_SENSOR_H_ */
