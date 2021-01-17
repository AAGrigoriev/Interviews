#pragma once

#include <fstream>

class BitWriter
{
private:
    std::ofstream&in;
    char buffer;
    unsigned countBits;

public:
    BitWriter(std::ofstream &in) : in(in), buffer(0), countBits(0) {}
    ~BitWriter() {} // think about

    void writeBit(unsigned i);
    void writeByte(unsigned i);
};