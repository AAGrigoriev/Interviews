#include <fstream>
#include <algorithm>
#include <utility>
#include <iostream>
#include <iterator>
#include <bitset>
#include <thread>

#include "Haffman/haffman.hpp"

namespace Revo_LLC
{

    void haffman_code::encode(std::string const &file_in, std::string const &file_out)
    {
        /* В зависимости от размера файла, если не подойдёт можно блоками считывать */
        encoder.encode(file_in, file_out);
    }

    void haffman_code::decode(std::string const &file_in, std::string const &file_out)
    {
        decoder.decode(file_in,file_out);
    }
} // namespace Revo_LLC