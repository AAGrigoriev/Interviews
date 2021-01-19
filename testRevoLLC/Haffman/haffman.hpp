#pragma once

#include <string>
#include "Haffman/haffman_encode.hpp"

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
        void free_tree(Node *root);

        // Node *decode_tree(BitReader &in);

        void print_tree(Node *root);

        haffman_encode encoder;
    };
} // namespace Revo_LLC