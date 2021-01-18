#pragma once

#include <string>
#include <unordered_map>
#include <queue>
#include <memory>
#include <fstream>

#include "option/BitWriter.hpp"
#include "option/BitReader.hpp"

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
        void build_freq_table(std::string const &in);

        void fill_deque();

        void build_tree();

        void store_code(Node *root, std::string data);

        void free_tree(Node *root);

        void encode_tree(Node *root, BitWriter &out);

        Node *decode_tree(BitReader &in);

        void print_tree(Node* root);

        struct Node
        {
            Node(int data, unsigned freq) : data(data), freq(freq) {}
            ~Node() {};
            Node *left = nullptr;
            Node *right = nullptr;

            int data;
            unsigned freq;
        };

        struct compare
        {
            bool operator()(Node *left, Node *right)
            {
                return left->freq > right->freq;
            }
        };

        std::unordered_map<int, unsigned> symbol_freq;

        std::unordered_map<int, std::string> symbol_code;

        std::priority_queue<Node *, std::vector<Node *>, compare> pr_que;
    };
} // namespace Revo_LLC