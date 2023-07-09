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
#include "string.h"
#include "stdbool.h"
#include "stm32_lcd.h"
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

typedef enum {
    SPEED_SCREEN,
    ASSIST_SCREEN,
    BATTERY_SCREEN,
    POWER_SCREEN
} ScreenType;
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
uint8_t __IO TransferAllowed = 0;
static uint8_t CacheBuffer[BUFFER_CACHE_COUNT][(320 * 2 * BUFFER_CACHE_LINES)];
//static block_t memory_blocks[BUFFER_CACHE_COUNT];
//static uint16_t posy0 = 0;
static uint16_t posx = 0;
static uint16_t posy = 0;
static uint8_t key = 1;
//static uint8_t image_id = 0;
static uint32_t LCD_Width = 0;
static uint32_t LCD_Height = 0;
static uint32_t LCD_Orientation = 0;
//static uint8_t orientation_id = 0;
static uint8_t speed = 0;
static uint8_t assist_level = 0;
static uint8_t battery_level = 0;
static uint8_t available_power = 0;
static char buffer[16];

//static int drawing_block_idx = 0;
//static int display_block_idx = 0;
static __IO uint16_t tearing_effect_counter = 0;

//static const orientation_t orientations[] = { { LCD_ORIENTATION_PORTRAIT,
	//	KEY_ORIENTATION_PORTRAIT }, { LCD_ORIENTATION_LANDSCAPE,
		//KEY_ORIENTATION_LANDSCAPE }, { LCD_ORIENTATION_PORTRAIT_ROT180,
		//KEY_ORIENTATION_PORTRAIT_ROT180 }, { LCD_ORIENTATION_LANDSCAPE_ROT180,
		//KEY_ORIENTATION_LANDSCAPE_ROT180 } };

ScreenType currentScreen = SPEED_SCREEN;
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

static void handleButtonClick(void);

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

void handleButtonClick() {
    // Increment the current screen
    currentScreen++;

    // If the current screen exceeds the maximum screen, wrap back to the first screen
    if (currentScreen > POWER_SCREEN) {
        currentScreen = SPEED_SCREEN;
    }
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
		if (BSP_KEY_GetState(0, &key) == BSP_ERROR_NONE) {
			if (key == BSP_KEY_CENTER) {
				handleButtonClick();
				UTIL_LCD_Clear(UTIL_LCD_COLOR_TRANSPARENT);
			}
		}

		// Update and display the current screen
		switch (currentScreen) {
		case SPEED_SCREEN:
			UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_TRANSPARENT);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);

			UTIL_LCD_SetFont(&Font24);
			UTIL_LCD_DisplayStringAt(posx, posy, COMPANY_NAME, CENTER_MODE);

			//snprintf(buffer, sizeof(buffer), "%d", vescMessage[8]);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
			UTIL_LCD_SetFont(&Font20);
			UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "SPEED", LEFT_MODE);
			UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "KM/H", RIGHT_MODE);
			UTIL_LCD_SetFont(&Font48);
			UTIL_LCD_DisplayStringAt(0, 140, (uint8_t*) buffer, CENTER_MODE);
			memset(buffer, 0, sizeof(buffer));
			UTIL_LCD_FillRect(280, 10, 30, 15, 0x07E0);
			break;
		case ASSIST_SCREEN:
			UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_TRANSPARENT);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);

			UTIL_LCD_SetFont(&Font24);
			UTIL_LCD_DisplayStringAt(posx, posy, COMPANY_NAME, CENTER_MODE);

			snprintf(buffer, sizeof(buffer), "%d", assist_level);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
			UTIL_LCD_SetFont(&Font20);
			UTIL_LCD_DisplayStringAt(0, 80, (uint8_t*) "ASSIST LEVEL", CENTER_MODE);
			UTIL_LCD_SetFont(&Font48);
			UTIL_LCD_DisplayStringAt(0, 140, (uint8_t*) buffer, CENTER_MODE);
			memset(buffer, 0, sizeof(buffer));
			break;
		case BATTERY_SCREEN:
			UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_TRANSPARENT);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);

			UTIL_LCD_SetFont(&Font24);
			UTIL_LCD_DisplayStringAt(posx, posy, COMPANY_NAME, CENTER_MODE);

			snprintf(buffer, sizeof(buffer), "%d", battery_level);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
			UTIL_LCD_SetFont(&Font20);
			UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "BATTERY LEVEL", LEFT_MODE);
			UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "%", RIGHT_MODE);
			UTIL_LCD_SetFont(&Font48);
			UTIL_LCD_DisplayStringAt(0, 140, (uint8_t*) buffer, CENTER_MODE);
			memset(buffer, 0, sizeof(buffer));
			break;
		case POWER_SCREEN:
			UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_TRANSPARENT);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);

			UTIL_LCD_SetFont(&Font24);
			UTIL_LCD_DisplayStringAt(posx, posy, COMPANY_NAME, CENTER_MODE);

			snprintf(buffer, sizeof(buffer), "%d", available_power);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
			UTIL_LCD_SetFont(&Font20);
			UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "AVAILABLE POWER", LEFT_MODE);
			UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "W", RIGHT_MODE);
			UTIL_LCD_SetFont(&Font48);
			UTIL_LCD_DisplayStringAt(0, 140, (uint8_t*) buffer, CENTER_MODE);
			memset(buffer, 0, sizeof(buffer));
			break;
		default:
			UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_TRANSPARENT);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_YELLOW);

			UTIL_LCD_SetFont(&Font24);
			UTIL_LCD_DisplayStringAt(posx, posy, COMPANY_NAME, CENTER_MODE);

			snprintf(buffer, sizeof(buffer), "%d", speed);
			UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
			UTIL_LCD_SetFont(&Font20);
			UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "SPEED", LEFT_MODE);
			UTIL_LCD_DisplayStringAt(15, 80, (uint8_t*) "KM/H", RIGHT_MODE);
			UTIL_LCD_SetFont(&Font48);
			UTIL_LCD_DisplayStringAt(0, 140, (uint8_t*) buffer, CENTER_MODE);
			memset(buffer, 0, sizeof(buffer));
			break;
		}
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

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
