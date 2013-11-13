/*
 * custom_sensors.h
 *
 *  Created on: Jul 23, 2012
 *      Author: nabercro
 */

#ifndef WATTAGE_SENSORS_H_
#define WATTAGE_SENSORS_H_

#include "sensors.h"
#include "events.h"
#include <inttypes.h>
#include "bool.h"
#include "shempserver.h"

void init_internal_wattage_sensor();

uint8_t create_internal_wattage_sensor(time_ref period, uint16_t size);
uint8_t enable_internal_wattage_sensor();
uint8_t disable_internal_wattage_sensor();
uint8_t delete_internal_wattage_sensor();


uint8_t create_internal_voltage_sensor(time_ref period, uint16_t size);
uint8_t enable_internal_voltage_sensor();
uint8_t disable_internal_voltage_sensor();
uint8_t delete_internal_voltage_sensor();

uint8_t create_internal_current_sensor(time_ref period, uint16_t size);
uint8_t enable_internal_current_sensor();
uint8_t disable_internal_current_sensor();
uint8_t delete_internal_current_sensor();

uint8_t create_internal_reference_sensor(time_ref period, uint16_t size);
uint8_t enable_internal_reference_sensor();
uint8_t disable_internal_reference_sensor();
uint8_t delete_internal_reference_sensor();


#endif /* CUSTOM_SENSORS_H_ */
