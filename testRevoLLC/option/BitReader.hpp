#pragma once

#include <istream>

class BitReader
{
private:
    std::istream &in;

    char buffer;
    unsigned count;

public:

    BitReader(std::istream& in): in(in),buffer(0),count(8) {}

    int readBit();

    int readByte();
};
