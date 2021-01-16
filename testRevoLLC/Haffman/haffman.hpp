#pragma once

#include <string>
#include <unordered_map>
#include <queue>
#include <memory>

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

        void store_code(Node *root,std::string data);

        void delete_tree(Node* root);

        struct Node
        {
            Node(char data, unsigned freq) : data(data), freq(freq) {}
            ~Node();
            Node *left = nullptr;
            Node *right = nullptr;

            char data;
            unsigned freq;
        };

        struct compare
        {
            bool operator()(Node *left, Node *right)
            {
                return left->freq > right->freq;
            }
        };

        std::unordered_map<char, unsigned>      symbol_freq;

        std::unordered_map<char, std::string>   symbol_code;

        std::priority_queue<Node *, std::vector<Node *>, compare> pr_que;
    };
} // namespace Revo_LLC