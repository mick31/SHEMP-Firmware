/*
 * audio_sensor.h
 * Author: Nathan Abercrombie
 * abercrombie.nathan@gmail.com
 *
 * See audio_sensor.c for implementation details.
 */



#ifndef AUDIO_SENSOR_H_
#define AUDIO_SENSOR_H_

#include <inttypes.h>
#include "sensors.h"
#include "events.h"
#include "linked_list.h"
#include "shempserver.h"
#include "time.h"

void init_audio_sensor();
sensor_ref create_audio_sensor(uint8_t loc, time_ref period, uint16_t size);

uint8_t disable_audio_sensor();
uint8_t enable_audio_sensor();
uint8_t delete_audio_sensor();

#endif /* AUDIO_SENSOR_H_ */
