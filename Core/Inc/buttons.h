/*
 * buttons.h
 *
 *  Created on: Jul 19, 2023
 *      Author: david
 */

#ifndef INC_BUTTONS_H_
#define INC_BUTTONS_H_

typedef enum ButtonState {
	ButtonNone,
	ButtonLeft,
	ButtonLeftLong,
	ButtonRight,
	ButtonRightLong,
	ButtonMiddle,
	ButtonMiddleLong,
}ButtonState;

#define LONG_PRESS (400)

extern uint32_t setting_selected;
extern int32_t duty_sent;

void button_task(void *argument);

ButtonState get_button_state(void);

uint8_t get_button_right(void);
uint8_t get_button_middle(void);
uint8_t get_button_left(void);

bool inc_assist(int inc_amount);
int get_assist(void);

#endif /* INC_BUTTONS_H_ */
