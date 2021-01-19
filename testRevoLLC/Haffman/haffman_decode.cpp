
#include <fstream>
#include <thread>
#include <iostream>

#include "Haffman/haffman_decode.hpp"
namespace Revo_LLC
{
    void haffman_decode::decode(std::string const &file_in, std::string const &file_out)
    {

        if (std::ifstream in{file_in, std::ios::binary})
        {
            BitReader reader(in);
            root = decode_tree(reader);
            decode_file(in, file_out);
            in.close();
        }
    }

    void haffman_decode::decode_file(std::ifstream &in, std::string const &file_out)
    {
        if (std::ofstream out{file_out})
        {
            char a;
            Node *curr = root;

            while (in.read(&a, sizeof(char)))
            {
                if (a == '0')
                    curr = curr->left;
                else
                    curr = curr->right;

                // reached leaf node
                if (curr->left == NULL and curr->right == NULL)
                {
                    if (curr->data == PSEUDO_EOF)
                        break;
                    out << char(curr->data);
                    curr = root;
                }
            }
            out.close();
        }
    }

    Node *haffman_decode::decode_tree(BitReader &reader)
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

} // namespace Revo_LLC