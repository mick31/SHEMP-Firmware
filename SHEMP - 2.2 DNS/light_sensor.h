/*
 * light_sensor.h
 * Author: Nathan Abercrombie
 *
 * This is the module for the light sensor.  It is the simpliest of the sensors
 */

#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_


#include "sensors.h"
#include "events.h"
#include <inttypes.h>
#include "bool.h"
#include "shempserver.h"


void init_light_sensor();
sensor_ref create_light_sensor(uint8_t loc, time_ref period, uint16_t size);




#endif /* LIGHT_SENSOR_H_ */
