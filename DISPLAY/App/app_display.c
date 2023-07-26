/**
  ******************************************************************************
  * File Name          : app_display.c
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "app_display.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
#include "key_io.h"
#include "mem_io.h"
#include "lcd_io.h"
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "stm32_lcd.h"
#include "cmsis_os.h"

#include "settings.h"
#include "buttons.h"
#include "vesc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct image_s {
	uint32_t Width;
	uint32_t Height;
	uint8_t bpp;
	uint8_t *Data;
} image_t;

typedef struct orientation_s {
	uint32_t lcd;
	uint32_t key;
} orientation_t;

typedef enum block_state_e {
	EMPTY = 'E', ALLOCATED = 'A', DRAWN = 'D', SENDING = 'S'
} block_state_t;

typedef struct block_s {
	block_state_t state;
	uint16_t bottom_line;
	uint8_t *memory;
} block_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STEP_LINES                  1
#define BUTTON_USER_PRESSED_STATE   GPIO_PIN_RESET
#define BUFFER_CACHE_COUNT          3
#if defined(UTIL_LCD_DEFAULT_FONT)
#define MAIN_BOARD_NAME             (uint8_t *)"NUCLEO-G071RB"
#define COMPANY_NAME				(uint8_t *)"SUSTAINAWAVE"
#define EXPANSION_BOARD_NAME        (uint8_t *)"X-NUCLEO-GFX01M2"
#endif /* UTIL_LCD_DEFAULT_FONT */
#define BATTERY_X (175)
#define BATTERY_Y (40)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#if defined(VSYNC_FREQ_Pin)
#define LCD_VSYNC_FREQ_LOW()                       WRITE_REG(VSYNC_FREQ_GPIO_Port->BRR, VSYNC_FREQ_Pin)
#define LCD_VSYNC_FREQ_HIGH()                      WRITE_REG(VSYNC_FREQ_GPIO_Port->BSRR, VSYNC_FREQ_Pin)
#endif /* VSYNC_FREQ_Pin */

#if defined(RENDER_TIME_Pin)
#define LCD_RENDER_TIME_LOW()                       WRITE_REG(RENDER_TIME_GPIO_Port->BRR, RENDER_TIME_Pin)
#define LCD_RENDER_TIME_HIGH()                      WRITE_REG(RENDER_TIME_GPIO_Port->BSRR, RENDER_TIME_Pin)
#endif /* RENDER_TIME_Pin */

#if defined(FRAME_RATE_Pin)
#define LCD_FRAME_RATE_LOW()                       WRITE_REG(FRAME_RATE_GPIO_Port->BRR, FRAME_RATE_Pin)
#define LCD_FRAME_RATE_HIGH()                      WRITE_REG(FRAME_RATE_GPIO_Port->BSRR, FRAME_RATE_Pin)
#endif /* FRAME_RATE_Pin */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static uint8_t CacheBuffer[BUFFER_CACHE_COUNT][(320 * 2 * BUFFER_CACHE_LINES)];
static uint16_t posx = 0;
static uint16_t posy = 0;
static uint32_t LCD_Width = 0;
static uint32_t LCD_Height = 0;
static uint32_t LCD_Orientation = 0;

static uint8_t company_name[13] = "SUSTAINAWAVE";

static __IO uint16_t tearing_effect_counter = 0;

// Screen to be displayed; protected by mutex
ScreenType current_screen = ScreenSpeed;

ScreenType screen_old = ScreenSpeed;
static int battery_old = -1;
static int speed_old = -1;
static int setting_old = -1;
static int setting_selected_old = -1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void BSP_LCD_Clear(uint32_t Instance, uint32_t Xpos, uint32_t Ypos,
		uint32_t Width, uint32_t Height);

static int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos,
		uint32_t Width, uint32_t Height, uint32_t Color);

static int32_t BSP_LCD_FillRGBRect2(uint32_t Instance, uint32_t Xpos,
		uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height);

static int32_t BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat);

static bool setup_display(ScreenType screen);
static bool display_speed(void);
static bool display_assist(void);
static bool display_power(void);
static bool display_battery(void);
static bool display_settings(bool editing);

static bool show_battery(int xpos, int ypos);

static const LCD_UTILS_Drv_t LCD_Driver = {
NULL, /* DrawBitmap   */
BSP_LCD_FillRGBRect2, /* FillRGBRect  */
NULL, /* DrawHLine    */
NULL, /* DrawVLine    */
BSP_LCD_FillRect, /* FillRect     */
NULL, /* GetPixel     */
NULL, /* SetPixel     */
BSP_LCD_GetXSize, /* GetXSize     */
BSP_LCD_GetYSize, /* GetYSize     */
NULL, /* SetLayer     */
BSP_LCD_GetPixelFormat /* GetFormat    */
};
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static int32_t BSP_LCD_FillRGBRect2(uint32_t Instance, uint32_t Xpos,
		uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height) {
	while (1) {
		uint16_t current_display_line = (
				tearing_effect_counter > 0 ?
						0xFFFF : ((uint16_t) hLCDTIM.Instance->CNT));
		if ((Ypos + Height) < current_display_line) {
			return BSP_LCD_FillRGBRect(Instance, 0, pData, Xpos, Ypos, Width,
					Height);
		}
	}
}

/**
  * @brief  Draws a full rectangle in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Width Rectangle width
  * @param  Height Rectangle height
  */
static int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  uint32_t size;
  uint32_t CacheLinesCnt, CacheLinesSz;
  uint32_t offset = 0, line_cnt = Ypos;

  size = (2*Width*Height);
  CacheLinesCnt = (Height > BUFFER_CACHE_LINES ? BUFFER_CACHE_LINES : Height);
  CacheLinesSz = (2*Width*CacheLinesCnt);
  memset(CacheBuffer[0], Color, CacheLinesSz);

  while(line_cnt < (Ypos + Height))
  {
    uint16_t current_display_line = (tearing_effect_counter > 0 ? 0xFFFF : ((uint16_t)hLCDTIM.Instance->CNT));
    if((line_cnt + CacheLinesCnt) < current_display_line)
    {
      if(BSP_LCD_FillRGBRect(Instance, 0, CacheBuffer[0], Xpos, line_cnt, Width, CacheLinesCnt) == BSP_ERROR_NONE)
      {
        line_cnt += CacheLinesCnt;
        offset += CacheLinesSz;
      }
    }
    /* Check remaining data size */
    if(offset == size)
    {
      /* last block transfer was done, so exit */
      break;
    }
    else if((offset + CacheLinesSz) > size)
    {
      /* Transfer last block and exit */
      CacheLinesCnt = ((size - offset)/ (2*Width));
      current_display_line = (tearing_effect_counter > 0 ? 0xFFFF : ((uint16_t)hLCDTIM.Instance->CNT));
      if((line_cnt + CacheLinesCnt) < current_display_line)
      {
        if(BSP_LCD_FillRGBRect(Instance, 0, CacheBuffer[0], Xpos, line_cnt, Width, CacheLinesCnt) == BSP_ERROR_NONE)
        {
          line_cnt += CacheLinesCnt;
        }
      }
    }
  }
  return BSP_ERROR_NONE;
}

static int32_t BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat) {
	int32_t ret = BSP_ERROR_NONE;

	if (Instance >= LCD_INSTANCES_NBR) {
		ret = BSP_ERROR_WRONG_PARAM;
	} else {
		/* Only RGB565 format is supported */
		*PixelFormat = LCD_PIXEL_FORMAT_RGB565;
	}

	return ret;
}

static void BSP_LCD_Clear(uint32_t Instance, uint32_t Xpos, uint32_t Ypos,
		uint32_t Width, uint32_t Height) {
	BSP_LCD_FillRect(Instance, Xpos, Ypos, Width, Height, 0);
}

/* USER CODE END 0 */

/**
 * PreOS Initialization function
 */
void MX_DISPLAY_PreOSInit(void)
{
  /* USER CODE BEGIN MX_DISPLAY_PreOSInit 0 */

  /* USER CODE END MX_DISPLAY_PreOSInit 0 */

  /* USER CODE BEGIN MX_DISPLAY_PreOSInit 1 */

  /* USER CODE END MX_DISPLAY_PreOSInit 1 */
}

/**
 * Initialize DISPLAY application
 */
void MX_DISPLAY_Init(void)
{
  /* USER CODE BEGIN MX_DISPLAY_Init 0 */
  init_settings();
  /* USER CODE END MX_DISPLAY_Init 0 */
  if(BSP_LCD_Init(0, LCD_ORIENTATION_PORTRAIT) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  if(BSP_MEM_Init(0) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  if(BSP_KEY_Init(0, KEY_ORIENTATION_PORTRAIT) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN MX_DISPLAY_Init 1 */
	if ((BSP_LCD_GetXSize(0, &LCD_Width) != BSP_ERROR_NONE)
			|| (BSP_LCD_GetYSize(0, &LCD_Height) != BSP_ERROR_NONE)
			|| (BSP_LCD_GetOrientation(0, &LCD_Orientation) != BSP_ERROR_NONE)) {
		Error_Handler();
	}
	BSP_LCD_Clear(0, 0, 0, LCD_Width, LCD_Height);
	if (BSP_LCD_DisplayOn(0) != BSP_ERROR_NONE) {
		Error_Handler();
	}

	UTIL_LCD_SetFuncDriver(&LCD_Driver);

	setup_display(ScreenSpeed);
  /* USER CODE END MX_DISPLAY_Init 1 */
}

/**
 * DISPLAY application task
 */
void DISPLAY_Task(void *argument)
{
  /* USER CODE BEGIN DISPLAY_Task */
	for(;;)
	{
		ScreenType screen = get_screen();
		if(screen != screen_old)
		{
			// If switching between settings editing and select modes
			if((screen_old == ScreenSettings && screen == ScreenSettingsEditing)
					|| (screen_old == ScreenSettingsEditing && screen == ScreenSettings))
			{
				// Update the display
				screen_old = screen;
				// Set old value so number does not flash
				setting_old = get_setting(get_selection());
			}
			// Otherwise, setup a new display
			else
			{
				// Update current display
				screen_old = screen;
				// Setup new display
				setup_display(screen);
			}

		}

		// Update and display the current screen
		switch (screen) {
		case ScreenSpeed:
			display_speed();
			break;
		case ScreenAssist:
			display_assist();
			break;
		case ScreenBattery:
			display_battery();
			break;
		case ScreenPower:
			display_power();
			break;
		case ScreenSettings:
			display_settings(0);
			break;
		case ScreenSettingsEditing:
			display_settings(1);
			break;
		default:
			display_speed();
			break;
		}

		vTaskDelay(250);
	}
  /* USER CODE END DISPLAY_Task */
}

void BSP_LCD_SignalTearingEffectEvent(uint32_t Instance, uint8_t state, uint16_t Line)
{
  if(Instance == 0)
  {
    /* USER CODE BEGIN BSP_LCD_SignalTearingEffectEvent */
    if(state)
    {
      /* Line '0' is the Vsync event */
      if(Line == 0)
      {
        /* TE event is received : allow display refresh */
      }
    }
    else
    {
      /* TE event is done : de-allow display refresh */
    }
    /* USER CODE END BSP_LCD_SignalTearingEffectEvent */
  }
}

/* USER CODE BEGIN 1 */

ScreenType get_screen(void)
{
	osSemaphoreAcquire(screen_updateHandle, portMAX_DELAY);
	ScreenType screen = current_screen;
	osSemaphoreRelease(screen_updateHandle);

	return screen;
}
bool update_screen(ScreenType screen)
{
	osSemaphoreAcquire(screen_updateHandle, portMAX_DELAY);
	current_screen = screen;
	osSemaphoreRelease(screen_updateHandle);

	return 0;
}

int get_selection(void)
{
	osSemaphoreAcquire(settings_selectionHandle, portMAX_DELAY);
	int selection = setting_selected;
	osSemaphoreRelease(settings_selectionHandle);

	return selection;
}

bool set_selection(int selection)
{
	osSemaphoreAcquire(settings_selectionHandle, portMAX_DELAY);
	setting_selected = selection;
	osSemaphoreRelease(settings_selectionHandle);

	return 0;
}

static bool setup_display(ScreenType screen)
{
	// Clear screen so next data can be displayed
	UTIL_LCD_Clear(UTIL_LCD_COLOR_BLACK);

	// Depending on which screen
	switch(screen)
	{
	case ScreenSpeed:
		/* General text */
		UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_TRANSPARENT);
		UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);

		UTIL_LCD_SetFont(&Font24);
		UTIL_LCD_DisplayStringAt(posx, posy, company_name, CENTER_MODE);
		UTIL_LCD_SetFont(&Font20);
		UTIL_LCD_DisplayStringAt(155, 144, (uint8_t*) "KM/", LEFT_MODE);
		UTIL_LCD_DisplayStringAt(155, 168, (uint8_t*) "H", LEFT_MODE);

		// Force battery to be updated
		battery_old = -1;
		// Force speed to be updated
		speed_old = -1;
		show_battery(BATTERY_X, BATTERY_Y);

		/* Assist */
		UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);
		UTIL_LCD_SetFont(&Font20);
		UTIL_LCD_DisplayStringAt(10, 260, (uint8_t*)"ASSIST", LEFT_MODE);

		break;
	case ScreenAssist:
		UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_TRANSPARENT);
		UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);

		UTIL_LCD_SetFont(&Font24);
		UTIL_LCD_DisplayStringAt(posx, posy, COMPANY_NAME, CENTER_MODE);
		break;
	case ScreenPower:
		UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_TRANSPARENT);
		UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);

		UTIL_LCD_SetFont(&Font24);
		UTIL_LCD_DisplayStringAt(posx, posy, COMPANY_NAME, CENTER_MODE);
		break;
	case ScreenBattery:
		UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_TRANSPARENT);
		UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);

		UTIL_LCD_SetFont(&Font24);
		UTIL_LCD_DisplayStringAt(posx, posy, COMPANY_NAME, CENTER_MODE);
		break;
	case ScreenSettings:
		UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);
		UTIL_LCD_SetFont(&Font24);
		UTIL_LCD_DisplayStringAt(posx, posy, (uint8_t*)"Settings", CENTER_MODE);
		UTIL_LCD_FillRect(5, 50, 4, 270, UTIL_LCD_COLOR_TRANSPARENT);
		break;
	case ScreenSettingsEditing:
		UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);
		UTIL_LCD_SetFont(&Font24);
		UTIL_LCD_FillRect(235, 50, 4, 270, UTIL_LCD_COLOR_TRANSPARENT);
		UTIL_LCD_DisplayStringAt(posx, posy, (uint8_t*)"Settings", CENTER_MODE);
		break;
	}

	return 0;
}

static bool display_speed(void)
{
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
	UTIL_LCD_SetFont(&Font48);
	uint32_t speed_updated = (vesc_status.rpm * 60 * (get_setting(WheelSize) * M_PI)) / 100000;

	if(speed_updated != speed_old && speed_updated < 100)
	{
		UTIL_LCD_FillRect(80, 140, 75, 50, UTIL_LCD_COLOR_TRANSPARENT);
		speed_old = speed_updated;
		char buffer[16];
		snprintf(buffer, 16, "%lu", speed_updated);
		UTIL_LCD_DisplayStringAt(100, 140, (uint8_t*)buffer, RIGHT_MODE);
	}

	/* Battery */
	show_battery(BATTERY_X, BATTERY_Y);
	/* Assist */
	int assist = get_assist();
	for(int i = 4; i >= assist; i--)
	{
		UTIL_LCD_FillRect(10 + (i * 44), 290, 40, 20, UTIL_LCD_COLOR_TRANSPARENT);
	}
	for(int i = 0; i < assist; i++)
	{
		UTIL_LCD_FillRect(10 + (i * 44), 290, 40, 20, UTIL_LCD_COLOR_WHITE);
	}

	return 0;
}
static bool display_assist(void)
{
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
	UTIL_LCD_SetFont(&Font20);
	UTIL_LCD_DisplayStringAt(0, 80, (uint8_t*) "ASSIST LEVEL", CENTER_MODE);
	UTIL_LCD_SetFont(&Font48);

	return 0;
}
static bool display_power(void)
{
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
	UTIL_LCD_SetFont(&Font20);
	UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "AVAILABLE POWER", LEFT_MODE);
	UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "W", RIGHT_MODE);
	UTIL_LCD_SetFont(&Font48);

	return 0;
}

static bool display_battery(void)
{
	show_battery(BATTERY_X, BATTERY_Y);

	return 0;
}

static bool display_settings(bool editing)
{
	// If more than 10 settings, just print 10
	int num_printed = (SETTINGS_LENGTH > 10) ? 10 : SETTINGS_LENGTH;

	int selection = get_selection();
	if(selection != setting_selected_old)
	{
		UTIL_LCD_FillRect(235, 20, 4, 300, UTIL_LCD_COLOR_TRANSPARENT);
		UTIL_LCD_FillRect(5, 20, 4, 300, UTIL_LCD_COLOR_TRANSPARENT);
		setting_selected_old = selection;
	}

	if(!editing)
	{
		UTIL_LCD_FillRect(5, 56 + (selection * 20), 4, 4, UTIL_LCD_COLOR_WHITE);
		// Clear the other side
		UTIL_LCD_FillRect(235, 20, 4, 300, UTIL_LCD_COLOR_TRANSPARENT);
	}
	else
	{
		// Clear the other side
		UTIL_LCD_FillRect(5, 20, 4, 300, UTIL_LCD_COLOR_TRANSPARENT);
		// If the setting has changed
		if(setting_old != get_setting((Settings)selection))
		{
			// Clear the relevant setting
			UTIL_LCD_FillRect(160, 50 + (selection) * 20, 80, 16, UTIL_LCD_COLOR_TRANSPARENT);
			setting_old = get_setting((Settings)selection);
		}

		// Display selection indicator
		UTIL_LCD_FillRect(235, 56 + (selection * 20), 4, 4, UTIL_LCD_COLOR_WHITE);
	}

	// Set font and text color for setting display
	UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
	UTIL_LCD_SetFont(&Font16);

	for(int i = 0; i < num_printed; i++)
	{
		// Display the setting label
		UTIL_LCD_DisplayStringAt(20, 50 + i * 20, (uint8_t*)get_label((Settings)i), LEFT_MODE);
		// Display setting value
		uint8_t setting_value[5];
		snprintf((char*)setting_value, sizeof(setting_value), "%d", get_setting((Settings)i));
		UTIL_LCD_DisplayStringAt(10, 50 + i * 20, setting_value, RIGHT_MODE);
	}

	return 0;
}

static bool show_battery(int xpos, int ypos)
{
	int battery = 0;
	int ret = 0;
	if(vesc_status.v_in >= 360 && vesc_status.v_in <= 420)
		battery = discharge_curve[vesc_status.v_in - 360];
	else
	{
		battery = 0;
		ret = 1;
	}
	if(battery_old != battery)
	{
		// Display battery icon
		UTIL_LCD_FillRect(xpos-2, ypos+6, 2, 10, UTIL_LCD_COLOR_WHITE);
		UTIL_LCD_FillRect(xpos, ypos, 52, 22, UTIL_LCD_COLOR_WHITE);
		UTIL_LCD_FillRect(xpos+1 + (50 - (battery/2)), ypos+1, battery/2, 20, UTIL_LCD_COLOR_BLACK);

		// Display percentage
		char battery_str[5];
		snprintf(battery_str, sizeof(battery_str), "%d%%", battery);
		UTIL_LCD_FillRect(xpos - 70, ypos, 60, 20, UTIL_LCD_COLOR_TRANSPARENT);
		UTIL_LCD_SetFont(&Font16);
		UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
		UTIL_LCD_DisplayStringAt((240 - xpos + 5), ypos + 5, (uint8_t*)battery_str, RIGHT_MODE);
		// Update old battery level
		battery_old = battery;
	}

	return ret;
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
