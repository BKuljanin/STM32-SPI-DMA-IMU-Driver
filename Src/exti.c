#include "exti.h"

#define GPIOCEN (1U<<2)
#define SYSCFGEN (1U<<14)

void pc13_exti_init(void)
{
	// Recommended that before we configure interrupt we disable global interrupt:
	__disable_irq();

	// Enable clock access for GPIOC
	RCC->AHB1ENR |= GPIOCEN; //GPIOC connected to AHB1 bus

	// Set PC13 as input
	GPIOC->MODER &=~ (1U<<26);
	GPIOC->MODER &=~ (1U<<27);

	// Enable clock access to SYSCFG (EXTI is part of SYSFCG module)
	RCC->APB2ENR |= SYSCFGEN; // syscfgen using apb1enr

	// Select PORTC for EXTI13
	SYSCFG->EXTICR[3] |= (1U<<5); // Reference manual p197 ch 8.2.6, table at the bottom contains EXTI13 for PC13.
	//Arrays similar to alternate function, we need register 4 which is index [3] because it starts from 0
	// p198 PC pin we need to set bit 5 (0010)

	// Unmask EXTI13, simply uncover, they are covered by default. We need to go to interrupt mask register
	EXTI->IMR |= (1U<<13); // Reference manual p244. 0: Interrupt request from line x is masked, 1: not masked

	// Interrupt triggered by rising or falling edge, now choosing rising edge trigger for MPU6500 data ready pin
	EXTI->RTSR |= (1U<<13);// Rising trigger selection register, reference manual p245 0: disabled rising trigger, 1 enabled

	// Enable EXTI13 line in NVIC
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	// Enable global interrupt
	__enable_irq();
}
