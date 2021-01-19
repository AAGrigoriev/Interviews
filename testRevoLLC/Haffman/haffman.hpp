#pragma once

#include <string>
#include "Haffman/haffman_encode.hpp"
#include "Haffman/haffman_decode.hpp"

namespace Revo_LLC
{

    class haffman_code
    {
        struct Node;

    public:
        haffman_code() = default;

        ~haffman_code() = default;

        void encode(std::string const &file_way_in, std::string const &file_way_out);

        void decode(std::string const &file_way_in, std::string const &file_way_out);

    private:
        haffman_encode encoder;
        haffman_decode decoder;
    };
} // namespace Revo_LLC