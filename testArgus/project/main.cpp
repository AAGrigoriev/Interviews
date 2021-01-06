#include <iostream>
#include "controller/controller.hpp"

using namespace testArgus;

int main()
{
    Controller controller;

    controller.read_from_stream(std::cin);

    return 0;
}