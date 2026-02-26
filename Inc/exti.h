/*
 * exti.h
 *
 *  Created on: Dec 2, 2025
 *      Author: Bogdan Kuljanin
 */

#ifndef EXTI_H_
#define EXTI_H_

#include "stm32f4xx.h"
void pc13_exti_init(void);

#define  LINE13		(1U<<13) // line 13 is bit 13 in pending register, reference manual

#endif /* EXTI_H_ */
