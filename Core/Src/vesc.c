/*
 * vesc.c
 *
 *  Created on: Jul 8, 2023
 *      Author: david
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "cmsis_os.h"

#include "main.h"
#include "vesc.h"

#include "settings.h"
#include "buttons.h"

uint8_t RxData[8];

// Globally visible status
// TODO Should probably deal with this better
volatile VESC_Status vesc_status;
static volatile uint32_t conv_value = 0;

static CAN_HandleTypeDef *vesc_can;

static void buffer_append_int32(uint8_t* buffer, int32_t number, int32_t *index);

void can_task(void* argumnet)
{
	for(;;)
	{
		TickType_t start_time = xTaskGetTickCount();

		// Get duty cycle according to buttons
		osSemaphoreAcquire(duty_cycleHandle, portMAX_DELAY);
		uint32_t num = duty_sent;
		osSemaphoreRelease(duty_cycleHandle);

		// Convert to duty cycle that will be accepted by VESC
		num *= 1000;

		// Put number in buffer
		uint8_t data[4];
		int32_t index = 0;
		buffer_append_int32(data, num, &index);

		// Transmit duty cycle
		vesc_transmit(0, data);

		// Trigger again every 1/10 of a second
		vTaskDelayUntil(&start_time, 100);
	}
}

void vesc_init(CAN_HandleTypeDef *can_handle)
{
	/* Get pointer to the CAN handle */
	vesc_can = can_handle;
  /* CAN filter to receive all messages */
  CAN_FilterTypeDef filter;
  filter.FilterActivation = CAN_FILTER_ENABLE;
  filter.FilterBank = 0;
  filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  filter.FilterMaskIdHigh = 0;
  filter.FilterMaskIdLow = 0;
  filter.FilterIdHigh = 0;
  filter.FilterIdLow = get_setting(VescID);
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;

  HAL_CAN_ConfigFilter(vesc_can, &filter);
  HAL_CAN_Start(vesc_can);
  HAL_CAN_ActivateNotification(vesc_can, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void vesc_transmit(uint8_t command, uint8_t * data)
{
  CAN_TxHeaderTypeDef header;
  header.StdId = 0;
  header.ExtId = get_setting(VescID) | (command << 8);
  header.IDE = CAN_ID_EXT;
  header.RTR = CAN_RTR_DATA;
  header.DLC = 1;
  header.TransmitGlobalTime = DISABLE;

  uint32_t mailbox;
  HAL_CAN_ActivateNotification(vesc_can, CAN_IT_TX_MAILBOX_EMPTY);

  // Transmit one message
  if(HAL_CAN_AddTxMessage(vesc_can, &header, data, &mailbox) != HAL_OK) {
	  __asm__("nop");
  }
}

static void buffer_append_int32(uint8_t* buffer, int32_t number, int32_t *index) {
	buffer[(*index)++] = number >> 24;
	buffer[(*index)++] = number >> 16;
	buffer[(*index)++] = number >> 8;
	buffer[(*index)++] = number;
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
	__asm__("nop");
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	CAN_RxHeaderTypeDef rxHeader;

	uint8_t data[8];
	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, data) != HAL_OK)
	{
		return;
	}

	switch(rxHeader.ExtId & 0xFF00)
	{
	case 0x0900:
		vesc_status.rpm = data[3] | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
		vesc_status.rpm /= 35;
		vesc_status.current = (data[4] << 8) | data[5];
		vesc_status.duty = (data[6] << 8) | data[7];
		break;
	case 0x0E00:
		vesc_status.ah = data[3] | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
		vesc_status.ah_cap = data[7] | (data[6] << 8) | (data[5] << 16) | (data[4] << 24);
		break;
	case 0x0F00:
		vesc_status.wh = data[3] | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
		vesc_status.wh_cap = data[7] | (data[6] << 8) | (data[5] << 16) | (data[4] << 24);
		break;
	case 0x1000:
		vesc_status.temp_fet = (data[0] << 8) | data[1];
		vesc_status.temp_motor = (data[2] << 8) | data[3];
		vesc_status.current_in = (data[4] << 8) | data[5];
		vesc_status.pid_pos_now = (data[6] << 8) | data[7];
		break;
	case 0x1b00:
		vesc_status.tacho_value = data[3] | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
		vesc_status.v_in = (data[4] << 8) | data[5];
		break;
	default:
		return;
		break;
	}

	return;
}

int get_vesc_data(uint32_t *result, int num)
{
	*result = *((uint32_t*)&vesc_status)+num;
	return 0;
}
