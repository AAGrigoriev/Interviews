
#include <fstream>
#include "Haffman/haffman_decode.hpp"
namespace Revo_LLC
{
    void haffman_decode::decode(std::string const &file_in, std::string &file_out)
    {
        if (std::ifstream in{file_in, std::ios::binary})
        {
            BitReader reader(in);
            Node *root = decode_tree(reader);
            print_tree(root);
        }
    }

} // namespace Revo_LLC