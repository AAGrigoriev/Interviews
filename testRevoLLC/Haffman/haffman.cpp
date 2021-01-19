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
 
    haffman_code::Node *haffman_code::decode_tree(BitReader &reader)
    {
        if (reader.readBit() == 1)
        {
            return new Node(reader.readByte(), 0);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            Node *out = new Node('*', 0);
            out->left = decode_tree(reader);
            out->right = decode_tree(reader);
            return out;
        }
    }

    void haffman_code::print_tree(Node *root)
    {
        if (root == nullptr)
            return;

        if (root->data != '*')
            std::cout << char(root->data) << std::endl;

        print_tree(root->left);
        print_tree(root->right);
    }

    void haffman_code::free_tree(Node *root)
    {
        if (!root)
            return;

        if (root->left)
            free_tree(root->left);
        if (root->right)
            free_tree(root->right);
        delete root;
    }

    void haffman_code::encode(std::string const &file_in, std::string const &file_out)
    {
        /* В зависимости от размера файла, если не подойдёт можно блоками считывать */
        encoder.encode(file_in, file_out);
    }

    void haffman_code::decode(std::string const &file_in, std::string const &file_out)
    {
        if (std::ifstream is{file_in, std::ios::binary})
        {
            BitReader reader(is);
            Node *root = decode_tree(reader);
            print_tree(root);
        }
    }
} // namespace Revo_LLC