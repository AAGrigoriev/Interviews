#include <fstream>
#include <algorithm>
#include <utility>
#include <iostream>
#include <iterator>
#include <bitset>

#include "Haffman/haffman.hpp"

namespace Revo_LLC
{
    unsigned haffman_code::Node::count = 0;

    void haffman_code::encode_tree(Node *root, std::ofstream &out)
    {
        if (root->left == nullptr)
        {
            out << std::bitset<1>(1);
            out << std::bitset<8>(root->data);
        }
        else
        {
            out << std::bitset<1>(0);
            encode_tree(root->left, out);
            encode_tree(root->right, out);
        }
    }

    void haffman_code::build_freq_table(std::string const &in)
    {
        auto lamda = [this](char ch) { symbol_freq[ch]++; };
        std::for_each(in.begin(), in.end(), lamda);
    }

    void haffman_code::fill_deque()
    {
        auto lamda = [this](auto &pair) { pr_que.emplace(new Node(pair.first, pair.second)); };
        std::for_each(symbol_freq.begin(), symbol_freq.end(), lamda);
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

    void haffman_code::store_code(Node *root, std::string data)
    {
        if (root == nullptr)
            return;

        if (root->data != '*')
            symbol_code[root->data] = data;

        store_code(root->left, data + "0");
        store_code(root->right, data + "1");
    }

    void haffman_code::build_tree()
    {
        while (pr_que.size() > 1)
        {
            auto left = pr_que.top();
            pr_que.pop();
            auto right = pr_que.top();
            pr_que.pop();

            auto node = new Node('*', left->freq + right->freq);
            node->left = left;
            node->right = right;
            pr_que.push(node);
        }
        store_code(pr_que.top(), "");
    }

    void haffman_code::encode(std::string const &file_in, std::string const &file_out)
    {
        /* В зависимости от размера файла, если не подойдёт можно блоками считывать */
        if (std::ifstream is{file_in, std::ios::ate})
        {
            auto size = is.tellg();
            std::string str(size, '\0');
            is.seekg(0);
            if (is.read(&str[0], size))
            {
                build_freq_table(str);
                fill_deque();
                build_tree();

                if (std::ofstream out{file_out, std::ios::binary})
                {
                    encode_tree(pr_que.top(), out);
                    out << " ";

                    for (char elem : str)
                    {
                        out << symbol_code[elem];
                    }
                }
                free_tree(pr_que.top());
            }
        }
    }

    void haffman_code::decode(std::string const &file_in, std::string const &file_out)
    {
    }

} // namespace Revo_LLC