#pragma once

#include <string>
#include <fstream>

#include "Haffman/haffman_node.hpp"
#include "option/BitReader.hpp"

namespace Revo_LLC
{
    class haffman_decode
    {
    public:
        void decode(std::string const &file_in, std::string const &file_out);

    private:
        Node *decode_tree(BitReader &reader);

        void decode_file(std::ifstream& in,std::string const &file_out);

        Node *root = nullptr;
    };
} // namespace Revo_LLC