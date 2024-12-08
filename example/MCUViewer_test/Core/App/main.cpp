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
#include <cmath>

TestClass test;
TemplateTest<uint32_t, bool> templateTest;
volatile float sinTest = 0.0f;
volatile float cosTest = 0.0f;
volatile float LissajousX = 0.0f;
volatile float LissajousY1 = 0.0f;
volatile float LissajousY2 = 0.0f;
volatile float LissajousY3 = 0.0f;

volatile float roseX = 0.0f;
volatile float roseY = 0.0f;

volatile float smallRoseX = 0.0f;
volatile float smallRoseY = 0.0f;

volatile float petalsNumber = 8.0f;

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
		LissajousX = test.getLissajousX(x);
		LissajousY1 = test.getLissajousY1(x);
		LissajousY2 = test.getLissajousY2(x);
		LissajousY3 = test.getLissajousY3(x);

		float r = cosf(petalsNumber / 2.0f * x);
		roseX = r * cosf(x);
		roseY = r * sinf(x);
		smallRoseY = 0.4f * r * sinf(x);

		x += 0.01f;

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
