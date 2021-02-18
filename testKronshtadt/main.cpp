#include "test.hpp"
#include <iostream>

int main()
{
    std::string console_output;

    std::getline(std::cin, console_output);

    std::cout << recStart(console_output.begin(),console_output.end());
}