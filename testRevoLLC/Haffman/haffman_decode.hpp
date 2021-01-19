#pragma once

#include <string>
#include "Haffman/haffman_node.hpp"
#include "option/BitReader.hpp"

namespace Revo_LLC
{
    class haffman_decode
    {
    public:
    
        void decode(std::string const &file_in, std::string &file_out);

    private:

        Node *decode_tree(BitReader& reader);

        Node *root = nullptr;
    };
} // namespace Revo_LLC