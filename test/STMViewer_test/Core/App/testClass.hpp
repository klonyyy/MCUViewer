/*
 * testClass.hpp
 *
 *  Created on: Feb 4, 2023
 *      Author: klonyyy
 */

#ifndef APP_TESTCLASS_HPP_
#define APP_TESTCLASS_HPP_

#include "main.h"

class TestClass
{
public:
	TestClass() = default;
	~TestClass() = default;

	float getSin(float x);
	float getCos(float x);
	void spin();

	struct __attribute__((packed, aligned(1))) StructA
	{
		uint8_t a = 12;
		uint8_t b = 155;
		int8_t c = -85;
		uint8_t d = 105;
		uint8_t e = 131;
		uint16_t f = 55234;
		int16_t g = -5234;
	}structA;

private:
	volatile float triangle = 0.0f;
	volatile float a,b,c;
	volatile float x;
	volatile int8_t tri = 0;
	volatile uint8_t ua = 250;
	volatile uint16_t ub = 65000;
	volatile uint32_t uc = 4290000000;
	volatile int8_t	ia = -120;
	volatile int16_t ib =  -32000;
	volatile int32_t ic = -2000000000;
	volatile float square = 21.37f;
};

#endif /* APP_TESTCLASS_HPP_ */
