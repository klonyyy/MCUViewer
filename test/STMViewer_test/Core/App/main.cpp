/*
 * main.cpp
 *
 *  Created on: Feb 4, 2023
 *      Author: klonyyy
 */

#include <main.hpp>
#include "stm32g4xx_ll_gpio.h"
#include "testClass.hpp"
#include "testClass2.hpp"

TestClass test;
TemplateTest<uint32_t, bool> templateTest;
volatile float sinTest = 0.0f;
volatile float cosTest = 0.0f;

void maincpp()
{
	LL_TIM_ClearFlag_UPDATE(TIM17);
	LL_TIM_EnableIT_UPDATE(TIM17);
	LL_TIM_EnableCounter(TIM17);

	LL_TIM_ClearFlag_UPDATE(TIM7);
	LL_TIM_EnableIT_UPDATE(TIM7);
	LL_TIM_EnableCounter(TIM7);

	LL_TIM_ClearFlag_UPDATE(TIM6);
	LL_TIM_EnableIT_UPDATE(TIM6);
	LL_TIM_EnableCounter(TIM6);

	float x = 0;

	while (1)
	{
		sinTest = test.getSin(x);
		cosTest = test.getCos(x);
		x += 0.001f;

		if (x > 6.28f)
		{
			x = 0.0f;
			templateTest.test = true;
		}
		else
			templateTest.test = false;

		test.spin();

		for(volatile uint32_t l=0;l<0xfff;l++)
			__asm__ __volatile__("nop");
	}
}
