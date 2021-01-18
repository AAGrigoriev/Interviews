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
    void haffman_code::encode_tree(Node *root, BitWriter &writer)
    {
        if (root->left == nullptr)
        {
            writer.writeBit(1);
            writer.writeByte(root->data);
        }
        else
        {
            writer.writeBit(0);
            encode_tree(root->left, writer);
            encode_tree(root->right, writer);
        }
    }

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
                    BitWriter writer(out);
                    encode_tree(pr_que.top(), writer);
                    writer.flush();
                    //out << " ";
                    /*         for (char elem : str)
                    {
                        out << symbol_code[elem];
                    }
                    */
                    out.close();
                }
                print_tree(pr_que.top());
                std::cout << "\n";
                free_tree(pr_que.top());
            }
            is.close();
        }
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