#include "Converter.hpp"
#include <vector>

using namespace ntProgress;

int main()
{
    Converter conv;

    que_rpn out_ = conv.toRPN("3+4*2-1");

    Calculator calc;

    calc.calculate(out_);

    return 0;
}