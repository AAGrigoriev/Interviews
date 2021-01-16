#include <fstream>
#include <algorithm>
#include <utility>
#include <iostream>

#include "haffman.hpp"

namespace Revo_LLC
{

    haffman_code::Node::~Node()
    {
        std::cout << "destructor\n";
    }    

    void haffman_code::build_freq_table(std::string const &in)
    {
        auto lamda = [this](char ch) { symbol_freq[ch]++; };
        std::for_each(in.begin(), in.end(), lamda);
    }

    void haffman_code::fill_deque()
    {
        auto lamda = [this](auto &pair) { pr_que.emplace(std::make_unique(pair->first, pair->second)); };
        std::for_each(symbol_freq.begin(), symbol_freq.end(), lamda);
    }

    void haffman_code::delete_tree(Node* root)
    {
        if(!root)
            return;
        
        if(root->left)
            delete_tree(root->left);
        if(root->right)
            delete_tree(root->right);

        delete root;
    }

    void haffman_code::store_code(Node *root, std::string data)
    {
        if (root == nullptr)
            return;

        if (root->data != '*')
            symbol_code[root->data] = data;

        if (root->left)
            store_code(root->left, data + "0");
        if (root->right)
            store_code(root->left, data + "1");
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

        store_code(pr_que.top(),"");
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
            }
        }
    }

    void haffman_code::decode(std::string const &file_in, std::string const &file_out)
    {
    }

} // namespace Revo_LLC