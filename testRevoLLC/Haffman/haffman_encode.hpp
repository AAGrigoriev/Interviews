#pragma once
#include <string>
#include <unordered_map>
#include <queue>
#include "Haffman/haffman_node.hpp"
#include "option/BitWriter.hpp"

namespace Revo_LLC
{
    class haffman_encode
    {
    public:
        void encode(std::string const &file_way_in, std::string const &file_way_out);

    private:
        using u_freq = std::unordered_map<int, unsigned>;
        using u_key = std::unordered_map<int, std::string>;
        using pr_queue = std::priority_queue<Node *, std::vector<Node *>, compare>;

        void build_freq_table(std::string const &in, u_freq &map_freq);

        void fill_deque(u_freq &map_freq, pr_queue &que);

        void build_tree(pr_queue &que);

        void store_code(Node *root, std::string data);

        void encode_tree(Node *root, BitWriter &out);

        void encode_data();

        /* Data */
        Node *root = nullptr;

        int PSEUDO_EOF = 256;

        u_key symbol_code;
    };

} // namespace Revo_LLC