#include "Converter.hpp"
#include <iostream>
using namespace ntProgress;

int main()
{
    try
    {
        Calculator calc;

        std::cout << calc.calculate("-1+5+3+dfghdfgdfg") << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }


    return 0;
}