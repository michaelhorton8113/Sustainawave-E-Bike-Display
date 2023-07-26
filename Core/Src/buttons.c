/*
 * buttons.c
 *
 *  Created on: Jul 19, 2023
 *      Author: david
 */

#include <stdbool.h>

#include "FreeRTOS.h"
#include "cmsis_os.h"

#include "buttons.h"

#include "stm32_lcd.h"
#include "key_io.h"
#include "app_display.h"
#include "settings.h"
#include "main.h"
#include "vesc.h"

uint32_t setting_selected = 0;
int32_t duty_sent = 0;

static TickType_t last_button_time = 0;
static TickType_t last_state_change = 0;
static uint8_t last_state = 0;		// Last state initialized as no button pushed
static bool long_press = 0;

static int assist_level = 3;


static void handleButtonClick(ButtonState button);

void button_task(void *argument)
{
	for(;;)
	{
		TickType_t start = xTaskGetTickCount();

		ButtonState button = get_button_state();

		if(button != ButtonNone)
			handleButtonClick(button);

		// Run task every 16ms (~60Hz)
		vTaskDelayUntil(&start, 16);
	}
}


void handleButtonClick(ButtonState button) {
	ScreenType screen = get_screen();

	switch(button) {
	case ButtonMiddle:
		switch(screen)
		{
		case ScreenSettings:
			screen = ScreenSettingsEditing;
			break;
		case ScreenSettingsEditing:
			screen = ScreenSettings;
			break;
		default:
			// Increment the current screen
			screen++;

			// If the current screen exceeds the maximum screen, wrap back to the first screen
			if (screen > ScreenBattery) {
				screen = ScreenSpeed;
			}
			break;
		}
		update_screen(screen);

	break;
	case ButtonMiddleLong:
		// Toggle settings screen
		if(screen != ScreenSettings && screen != ScreenSettingsEditing)
		{
			screen = ScreenSettings;
			set_selection(0);

			update_screen(screen);
		}
		else
		{
			screen = ScreenSpeed;
			update_screen(screen);
		}

		while(get_button_state() == ButtonMiddleLong) vTaskDelay(16);
		break;
	case ButtonRightLong:
		vesc_status.v_in = 360;
		while(get_button_state() == ButtonRightLong)
		{
			// Increase duty cycle by 1%
			osSemaphoreAcquire(duty_cycleHandle, portMAX_DELAY);
			duty_sent ++;
			osSemaphoreRelease(duty_cycleHandle);

			vesc_status.v_in ++;

			vTaskDelay(100);
		}

		osSemaphoreAcquire(duty_cycleHandle, portMAX_DELAY);
		duty_sent = 0;
		osSemaphoreRelease(duty_cycleHandle);

		break;
	case ButtonLeftLong:
		while(get_button_state() == ButtonLeftLong)
		{
			// Increase duty cycle by 1%
			osSemaphoreAcquire(duty_cycleHandle, portMAX_DELAY);
			duty_sent --;
			osSemaphoreRelease(duty_cycleHandle);

			vTaskDelay(100);
		}

		osSemaphoreAcquire(duty_cycleHandle, portMAX_DELAY);
		duty_sent = 0;
		osSemaphoreRelease(duty_cycleHandle);

		break;
	case ButtonLeft:
		if(screen == ScreenSettingsEditing)
		{
			dec_setting((Settings)setting_selected);
		}
		else if(screen == ScreenSettings)
		{
			int current_selection = get_selection();
			set_selection(current_selection + (int)(current_selection < SETTINGS_LENGTH));
		}
		else
		{
			inc_assist(-1);
		}
		break;
	case ButtonRight:
		if(screen == ScreenSettingsEditing)
		{
			inc_setting((Settings)get_selection());
		}
		else if(screen == ScreenSettings)
		{
			int current_selection = get_selection();
			set_selection(current_selection - (int)(current_selection > 0));
		}
		else
		{
			inc_assist(1);
		}
		break;
	default:

		break;
	}
}
ButtonState get_button_state(void)
{
	TickType_t timeout = LONG_PRESS;

	// Which buttons are pressed
	uint8_t button_state = 0;

	// Get state of buttons
	button_state |= get_button_left() << 2;			// Bit 2: Left button
	button_state |= get_button_right() << 1;		// Bit 1: Right button
	button_state |= get_button_middle();			// Bit 0: Middle button

	// If anything was pressed, update timestamp
	if(button_state)
		last_button_time = xTaskGetTickCount();

	// If the same button is still pressed
	if(button_state == last_state)
	{
		if(button_state == 0)
			return ButtonNone;
		if((xTaskGetTickCount() - last_state_change) >= timeout)
		{
			long_press = true;
			// Long press
			switch(button_state)
			{
			case 0x01:
				return ButtonMiddleLong;
				break;
			case 0x02:
				return ButtonRightLong;
				break;
			case 0x04:
				return ButtonLeftLong;
				break;
			default:
				return ButtonNone;
				break;
			}
		}
	}
	else
	{
		// Button state has changed and a button is pushed
		ButtonState state = ButtonNone;

		// If a button is pushed
		if(button_state)
			last_state |= button_state;
		// If all buttons are released
		else
		{
			// Do not send key after long press
			if(!long_press)
			{
				switch (last_state) {
				case 0x01:
					state = ButtonMiddle;
					break;
				case 0x02:
					state = ButtonRight;
					break;
				case 0x04:
					state = ButtonLeft;
					break;
				}
			}

			last_state = 0;
			long_press = false;
		}

		// Update last change of state
		last_state_change = xTaskGetTickCount();
		return state;
	}

	// No buttons pushed
	return ButtonNone;
}

bool inc_assist(int inc_amount)
{
	osSemaphoreAcquire(assist_levelHandle, portMAX_DELAY);

	assist_level += inc_amount;

	if(assist_level > 5) assist_level = 5;
	if(assist_level < 0) assist_level = 0;

	osSemaphoreRelease(assist_levelHandle);

	return 0;
}

int get_assist(void)
{
	osSemaphoreAcquire(assist_levelHandle, portMAX_DELAY);
	int assist = assist_level;
	osSemaphoreRelease(assist_levelHandle);

	return assist;
}

uint8_t get_button_left(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(GPIOC, (1) << 10) == GPIO_PIN_RESET;
}

uint8_t get_button_middle(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(GPIOC, (1) << 11) == GPIO_PIN_RESET;
}

uint8_t get_button_right(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(GPIOC, (1) << 12) == GPIO_PIN_RESET;
}
