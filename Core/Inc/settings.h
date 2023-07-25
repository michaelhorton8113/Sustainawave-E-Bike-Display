/*
 * settings.h
 *
 *  Created on: Jul 19, 2023
 *      Author: david
 */

#ifndef INC_SETTINGS_H_
#define INC_SETTINGS_H_

#include <stdbool.h>

#define SETTINGS_LENGTH (2)
typedef enum Settings {
	WheelSize 			= 0,
	VescID				= 1,
	TimeH				= 2,
	TimeM				= 3,
} Settings;

extern const int discharge_curve[61];

const char* get_label(Settings setting);
uint16_t get_setting(Settings setting);
void init_settings(void);
bool dec_setting(Settings setting);
bool inc_setting(Settings setting);

#endif /* INC_SETTINGS_H_ */
