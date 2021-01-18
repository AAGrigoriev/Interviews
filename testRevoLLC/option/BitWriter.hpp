#pragma once

#include <fstream>

class BitWriter
{
private:

    std::ofstream& out;
    char buffer;
    unsigned countBits;

public:
    BitWriter(std::ofstream &out) : out(out), buffer(0), countBits(0) {}
    
    ~BitWriter() {} 

    void writeBit(unsigned i);
    void writeByte(unsigned byte);
    void flush();
};