/*
 * main.hpp
 *
 *  Created on: Feb 4, 2023
 *      Author: klonyyy
 */

#ifndef APP_MAIN_HPP_
#define APP_MAIN_HPP_

#define LED_R_ON  LED_R_GPIO_Port->BSRR = LED_R_Pin
#define LED_R_OFF LED_R_GPIO_Port->BRR = LED_R_Pin
#define LED_R_TOG LL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin)

#define LED_G_ON  LED_G_GPIO_Port->BSRR = LED_G_Pin
#define LED_G_OFF LED_G_GPIO_Port->BRR = LED_G_Pin
#define LED_G_TOG LL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin)

#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

void maincpp(void);

#ifdef __cplusplus
}
#endif


#endif /* APP_MAIN_HPP_ */
