#include <option/BitReader.hpp>
#include <bitset>
#include <iostream>

int BitReader::readBit()
{
    unsigned out = 0;
    if (count == 8)
    {
        in.get(buffer);
        //std::bitset<8> buf(buffer);
        //std::cout << "input buffer " << buf.to_string() << std::endl;
        count = 0;
    }

    out = (1U & buffer >> (7 - count++));
    ///printf("out = %d\n", out);
    return out;
}

int BitReader::readByte()
{
    unsigned out = 0;

    //std::cout << "start readByte\n";

    for (int i = 7; i >= 0; --i)
    {
        out |= readBit() << i;
    }
    //std::cout << " alfabet " << std::bitset<8>(out).to_string() << std::endl;
    

    return out;
}