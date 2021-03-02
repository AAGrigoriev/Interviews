#include "Converter.hpp"
#include <iostream>
using namespace ntProgress;

int main()
{
    Calculator calc;

    std::cout << calc.calculate("3+4*2-1") << std::endl;

    return 0;
}