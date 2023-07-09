/*
 * vesc.c
 *
 *  Created on: Jul 8, 2023
 *      Author: david
 */

#include "vesc.h"

uint8_t RxData[8];
uint8_t vesc_id = 0x17;

// Globally visible status
// TODO Should probably deal with this better
VESC_Status vesc_status;

static CAN_HandleTypeDef *vesc_can;

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
  filter.FilterIdLow = vesc_id;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;

  HAL_CAN_ConfigFilter(vesc_can, &filter);
  HAL_CAN_Start(vesc_can);
  HAL_CAN_ActivateNotification(vesc_can, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void sendVESCMessage(uint8_t TA)
{
  CAN_TxHeaderTypeDef txHeader;
  uint8_t txData[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08} ;
  uint32_t txMailbox;

  //txData[2] = TA;

  txHeader.StdId = 38; // Replace with the actual CAN ID of the VESC
  txHeader.ExtId = 0x01;
  txHeader.RTR = CAN_RTR_DATA;
  txHeader.IDE = CAN_ID_STD;
  txHeader.DLC = 2;
  //txHeader.TransmitGlobalTime = DISABLE;

  if (HAL_CAN_AddTxMessage(vesc_can, &txHeader, txData, &txMailbox) != HAL_OK)
  {
  }
}

void vesc_transmit(void)
{
  CAN_TxHeaderTypeDef header;
  header.StdId = 41;
  header.ExtId = 41;
  header.IDE = CAN_ID_STD;
  header.RTR = CAN_RTR_DATA;
  header.DLC = 1;
  header.TransmitGlobalTime = DISABLE;

  uint32_t mailbox;
  uint8_t can_data = 0x69;
  HAL_CAN_ActivateNotification(vesc_can, CAN_IT_TX_MAILBOX_EMPTY);

  // Transmit one message
  if(HAL_CAN_AddTxMessage(vesc_can, &header, &can_data, &mailbox) != HAL_OK) {
	  __asm__("nop");
  }
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
		vesc_status.rpm = *(uint32_t*)&data[0];
		vesc_status.current = (data[4] << 8) | data[5];
		vesc_status.duty = (data[6] << 8) | data[7];
		break;
	case 0x0E00:
		vesc_status.ah = *(uint32_t*)&data[0];
		vesc_status.ah_cap = *(uint32_t*)&data[4];
		break;
	case 0x0F00:
		vesc_status.wh = *(uint32_t*)&data[0];
		vesc_status.wh_cap = *(uint32_t*)&data[4];
		break;
	case 0x1000:
		vesc_status.temp_fet = (data[0] << 8) | data[1];
		vesc_status.temp_motor = (data[2] << 8) | data[3];
		vesc_status.current_in = (data[4] << 8) | data[5];
		vesc_status.pid_pos_now = (data[6] << 8) | data[7];
		break;
	case 0x1b00:
		vesc_status.tacho_value = *(uint32_t*)&data[0];
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
