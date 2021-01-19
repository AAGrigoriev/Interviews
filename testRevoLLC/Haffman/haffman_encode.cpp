#include <fstream>
#include <algorithm>

#include "Haffman/haffman_encode.hpp"

namespace Revo_LLC
{
    void haffman_encode::encode(std::string const &file_way_in, std::string const &file_way_out)
    {
        if (std::ifstream in{file_way_in, std::ios::ate})
        {
            auto size = in.tellg();
            std::string str(size, '\0');
            in.seekg(0);
            if (in.read(&str[0], size))
            {
                u_freq symbol_freq;
                pr_queue pr_que;

                build_freq_table(str, symbol_freq);
                fill_deque(symbol_freq, pr_que);
                build_tree(pr_que);
                store_code(root, "");

                if (std::ofstream out{file_way_out, std::ios::binary})
                {
                    BitWriter writer(out);
                    encode_tree(pr_que.top(), writer);
                    writer.flush();
                    free_tree(root);

                    for (char elem : str)
                    {
                        auto &value = symbol_code[elem];
                        out.write(value.c_str(), value.size());
                    }
                    out.close();
                }
            }
            /* TODO: up close function */
            in.close();
        }
    }

    void haffman_encode::build_freq_table(std::string const &in, u_freq &map_freq)
    {
        auto lamda = [&map_freq](char ch) { map_freq[ch]++; };
        std::for_each(in.begin(), in.end(), lamda);
    }

    void haffman_encode::fill_deque(u_freq &map_freq, pr_queue &que)
    {
        auto lamda = [&map_freq, &que](auto &pair) { que.emplace(new Node(pair.first, pair.second)); };
        std::for_each(map_freq.begin(), map_freq.end(), lamda);

        /* Add pseudo_eof */
        que.emplace(new Node(PSEUDO_EOF, 1));
    }

    void haffman_encode::build_tree(pr_queue &que)
    {
        while (que.size() > 1)
        {
            auto left = que.top();
            que.pop();
            auto right = que.top();
            que.pop();

            auto node = new Node('*', left->freq + right->freq);
            node->left = left;
            node->right = right;
            que.push(node);
        }
        root = que.top();
    }

    void haffman_encode::store_code(Node *root, std::string data)
    {
        if (root == nullptr)
            return;

        if (root->data != '*')
            symbol_code[root->data] = data;

        store_code(root->left, data + "0");
        store_code(root->right, data + "1");
    }

    void haffman_encode::encode_tree(Node *root, BitWriter &writer)
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

} // namespace Revo_LLC