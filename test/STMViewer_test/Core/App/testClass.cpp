/*
 * testClass.cpp
 *
 *  Created on: Feb 4, 2023
 *      Author: klonyyy
 */

#include "testClass.hpp"
#include "math.h"


float TestClass::getSin(float x)
{
	return sinf(x);
}

float TestClass::getCos(float x)
{
	return cosf(x);
}

void TestClass::spin()
{
	x += 0.001f;

	if(x > 2.0f*M_PI)
	{
		x = 0.0f;

		if(tri > 0)
			tri = -1;
		else
			tri = 1;
	}

	triangle = (x/(2.0f*M_PI));
	a = sinf(x);
	b = sinf(x + (1.0f/3.0f)*2.0f*M_PI);
	c = sinf(x + (2.0f/3.0f)*2.0f*M_PI);
}
