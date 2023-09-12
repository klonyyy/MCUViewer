/*
 * main.cpp
 *
 *  Created on: Feb 4, 2023
 *      Author: klonyyy
 */

#include "maincpp.hpp"
#include "stm32g4xx_ll_gpio.h"
#include "testClass.hpp"

TestClass test;
volatile float sinTest = 0.0f;
volatile float cosTest = 0.0f;

void maincpp()
{
	float x = 0;

	while (1)
	{
		sinTest = test.getSin(x);
		cosTest = test.getCos(x);
		x += 0.001f;

		if (x > 6.28f)
			x = 0.0f;

		if(sinTest > 3.14f)
			LED_G_ON;
		else
			LED_G_OFF;

		test.spin();

		ITM->PORT[0].u8 = 0xaa;
		for(volatile uint32_t l=0;l<0xfff;l++)
		{
			__asm__ __volatile__("nop");
		}
		ITM->PORT[0].u8 = 0xbb;
	}
}
