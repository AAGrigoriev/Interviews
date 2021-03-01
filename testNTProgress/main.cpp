#include "Converter.hpp"
#include <iostream>
using namespace ntProgress;

int main()
{
    Converter conv;
     Calculator calc;

    std::queue<std::unique_ptr<IValue>> rpn = conv.toRPN("3+4*2-1");

    std::cout << calc.calculate(rpn) << std::endl;

    return 0;
}