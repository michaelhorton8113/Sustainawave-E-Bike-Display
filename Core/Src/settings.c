/*
 * settings.c
 *
 *  Created on: Jul 19, 2023
 *      Author: david
 */

#include <stdbool.h>
#include <stdint.h>

#include "settings.h"

// Min, max, increment and default values
typedef struct {
  char label[11];
  const uint16_t min;          // Inclusive minimum value
  const uint16_t max;          // Inclusive maximum value
  const uint16_t increment;    // Standard increment
  const uint16_t defaultValue; // Default vaue after reset
} SettingsConstants;

static const SettingsConstants settings_constants[SETTINGS_LENGTH] =
{
	{ "Wheel size",		50, 	100, 	1, 		70 },	// Wheel size
	{ "VESC ID",		0,		255,	1,		23 },
	{ "Time H",			1,		24,		1,		12 },
	{ "Time M",			0,		60, 	1,		0 },
};

static uint16_t settings[SETTINGS_LENGTH];

// Approximate discharge curve for 36V battery
const int discharge_curve[61] =
	{ 0, 2,	4, 6, 8, 10, 12, 14, 16, 18,
		20, 22, 24, 26, 28, 30, 32,	34,	36, 38,
		40,	42, 44, 46, 48, 50, 52, 54, 56, 58,
		60, 62, 64, 66, 68, 70, 72, 74, 76, 78,
		80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
		90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
		100,
};
void init_settings(void)
{
	for(int i = 0; i < SETTINGS_LENGTH; i++)
	{
		settings[i] = settings_constants[i].defaultValue;
	}
}

bool set_setting(Settings setting, uint16_t new_value)
{
	// If new settings was within the range
	if(new_value <= settings_constants[(int)setting].max
			&& new_value >= settings_constants[(int)setting].min)
	{
		// Update to the new value
		settings[(int)setting] = new_value;

		return 0;
	}
	else return 1;
}

uint16_t get_setting(Settings setting)
{
	return settings[(int)setting];
}

bool inc_setting(Settings setting)
{
	if(settings[(int)setting] + settings_constants[(int)setting].increment <= settings_constants[(int)setting].max)
	{
		settings[(int)setting] += settings_constants[(int)setting].increment;
	}

	return 1;
}

bool dec_setting(Settings setting)
{
	if(settings[(int)setting] - settings_constants[(int)setting].increment >= settings_constants[(int)setting].min)
	{
		settings[(int)setting] -= settings_constants[(int)setting].increment;
	}

	return 1;
}

const char* get_label(Settings setting)
{
	return settings_constants[(int)setting].label;
}
