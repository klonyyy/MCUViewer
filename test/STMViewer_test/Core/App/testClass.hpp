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

private:
	volatile uint8_t ua = 250;
	volatile uint16_t ub = 65000;
	volatile uint32_t uc = 4290000000;
	volatile int8_t	ia = -120;
	volatile int16_t ib =  -32000;
	volatile int32_t ic = -2000000000;
};

#endif /* APP_TESTCLASS_HPP_ */
