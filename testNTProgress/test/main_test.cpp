#include <gtest/gtest.h>
#include <string>

#include "Calculator.hpp"

TEST(test_simple, test_integer)
{
    ntProgress::Calculator calc;

    ASSERT_EQ(calc.calculate("2+3*5-2*(2+3)"), 7);
}

TEST(test_simple, test_double)
{
    ntProgress::Calculator calc;

    ASSERT_EQ(calc.calculate("2.5+3.5*2"), 9.5);
}

TEST(test_simple, test_wrong)
{
    ntProgress::Calculator calc;

    ASSERT_THROW(calc.calculate("1+3+abc"), std::invalid_argument);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}