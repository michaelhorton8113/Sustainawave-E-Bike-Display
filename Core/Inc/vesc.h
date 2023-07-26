/*
 * vesc.h
 *
 *  Created on: Jul 8, 2023
 *      Author: david
 */

#ifndef INC_VESC_H_
#define INC_VESC_H_

#include "stm32l4xx_hal.h"

typedef struct VESC_Status {
	uint32_t rpm;			// 4b rpm
	uint32_t current;		// 2b current * 10
	uint32_t duty;			// 2b duty * 1000
	uint32_t ah;			// 4b amp_hours * 10000
	uint32_t ah_cap;		// 4b amp_hours_charged * 10000
	uint32_t wh;			// 4b watt_hours * 10000
	uint32_t wh_cap;		// 4b watt_hours_charged * 10000
	uint32_t temp_fet;		// 2b temp_fet * 10
	uint32_t temp_motor;	// 2b temp_motor * 10
	uint32_t current_in; 	// 2b current_in * 10
	uint32_t pid_pos_now;	// 2b pid_pos_now * 50
	uint32_t tacho_value;	// 4b tacho_value
	uint32_t v_in;			// 2b v_in * 10
}VESC_Status;

extern volatile VESC_Status vesc_status;

void can_task(void* argumnet);
void sendVESCMessage(uint8_t TA);
void vesc_init(CAN_HandleTypeDef *can_handle);
void vesc_transmit(uint8_t command, uint8_t *data);
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
int get_vesc_data(uint32_t *result, int num);

#endif /* INC_VESC_H_ */
