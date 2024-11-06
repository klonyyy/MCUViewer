#include <gtest/gtest.h>

#include <array>
#include <iostream>

#include "Variable.hpp"

TEST(VariableTest, testSignedFracPositive)
{
	Variable var{"test"};

	var.setType(Variable::Type::I16);
	var.setHighLevelType(Variable::HighLevelType::SIGNEDFRAC);
	var.setFractional({15, 1.0});
	var.setRawValueAndTransform(32767);
	double value = var.getValue();

	ASSERT_NEAR(value, 1.0, 10e-3);
}

TEST(VariableTest, testSignedFracNegative)
{
	Variable var{"test"};

	var.setType(Variable::Type::I16);
	var.setHighLevelType(Variable::HighLevelType::SIGNEDFRAC);
	var.setFractional({15, 1.0});
	var.setRawValueAndTransform(-32768);
	double value = var.getValue();

	ASSERT_NEAR(value, -1.0, 10e-6);
}

TEST(VariableTest, testSignedFracUnsignedVarPositive)
{
	Variable var{"test"};

	var.setType(Variable::Type::U16);
	var.setHighLevelType(Variable::HighLevelType::SIGNEDFRAC);
	var.setFractional({15, 1.0});
	var.setRawValueAndTransform(32767);
	double value = var.getValue();

	ASSERT_NEAR(value, 1.0, 10e-3);
}

TEST(VariableTest, testSignedFracUnsignedVarNegative)
{
	Variable var{"test"};

	var.setType(Variable::Type::U16);
	var.setHighLevelType(Variable::HighLevelType::SIGNEDFRAC);
	var.setFractional({15, 1.0});
	var.setRawValueAndTransform(-32768);
	double value = var.getValue();

	ASSERT_NEAR(value, -1.0, 10e-6);
}

/** Unsigned Fractional ***/

TEST(VariableTest, testUnsignedFracPositive)
{
	Variable var{"test"};

	var.setType(Variable::Type::I16);
	var.setHighLevelType(Variable::HighLevelType::UNSIGNEDFRAC);
	var.setFractional({15, 1.0});
	var.setRawValueAndTransform(32767);
	double value = var.getValue();

	ASSERT_NEAR(value, 1.0, 10e-3);
}

TEST(VariableTest, testUnsignedFracNegative)
{
	Variable var{"test"};

	var.setType(Variable::Type::I16);
	var.setHighLevelType(Variable::HighLevelType::UNSIGNEDFRAC);
	var.setFractional({15, 1.0});
	var.setRawValueAndTransform(65534);
	double value = var.getValue();

	ASSERT_NEAR(value, 2.0, 10e-3);
}

TEST(VariableTest, testUnsignedFracUnsignedVarPositive)
{
	Variable var{"test"};

	var.setType(Variable::Type::U16);
	var.setHighLevelType(Variable::HighLevelType::UNSIGNEDFRAC);
	var.setFractional({16, 1.0});
	var.setRawValueAndTransform(32767);
	double value = var.getValue();

	ASSERT_NEAR(value, 0.5, 10e-3);
}

TEST(VariableTest, testUnsignedFracUnsignedVarNegative)
{
	Variable var{"test"};

	var.setType(Variable::Type::U16);
	var.setHighLevelType(Variable::HighLevelType::UNSIGNEDFRAC);
	var.setFractional({16, 1.0});
	var.setRawValueAndTransform(65534);
	double value = var.getValue();

	ASSERT_NEAR(value, 1.0, 10e-3);
}
