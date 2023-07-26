/**
  ******************************************************************************
  * File Name          : app_display.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#include <stdbool.h>

typedef enum {
    ScreenSpeed,
    ScreenAssist,
    ScreenBattery,
    ScreenPower,
	ScreenSettings,
	ScreenSettingsEditing,
} ScreenType;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void MX_DISPLAY_PreOSInit(void);
void MX_DISPLAY_Init(void);
void DISPLAY_Task(void *argument);

/* USER CODE BEGIN EFP */
ScreenType get_screen(void);
bool update_screen(ScreenType screen);
int get_selection(void);
bool set_selection(int selection);

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif
#endif /* APP_DISPLAY_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
