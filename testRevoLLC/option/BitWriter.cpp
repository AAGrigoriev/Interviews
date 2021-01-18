#include <option/BitWriter.hpp>
#include <bitset>
#include <iostream>

void BitWriter::writeBit(unsigned i)
{
    if (countBits == 8)
    {
        out.put(buffer);
        out.flush();
        countBits = 0;
        buffer = 0;
    }

    buffer |= (i << 7 - countBits++);
}

void BitWriter::writeByte(unsigned byte)
{
    for (int i = 7; i >= 0; --i)
    {
        writeBit((byte >> i) & 1);
    }
}

void BitWriter::flush()
{
    out.put(buffer);
    buffer = 0;
    countBits = 0;
}