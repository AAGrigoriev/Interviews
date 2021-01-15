#pragma once

#include <string>
#include <unordered_map>
#include <queue>
#include <memory>

namespace Revo_LLC
{

    class haffman_code
    {
    public:
        haffman_code() = default;

        ~haffman_code() = default;

        void encode(std::string const &file_way_in, std::string const &file_way_out);

        void decode(std::string const &file_way_in, std::string const &file_way_out);

    private:

        void fill_freq(std::string const &in);

        void create_deque();

        struct Node
        {
            Node(char data,unsigned freq) : data(data),freq(freq) {}

            std::unique_ptr<Node> left;
            std::unique_ptr<Node> right;

            char data;
            unsigned freq;
        };

        struct compare
        {
            bool operator()(std::unique_ptr<Node> const &left, std::unique_ptr<Node> const &right)
            {
                return left.get()->freq > right.get()->freq;
            }
        };

        std::unordered_map<char, unsigned> freq;

        std::unordered_map<char, std::string> decoding;

        std::priority_queue<std::unique_ptr<Node>, std::vector<std::unique_ptr<Node>>, compare> pr_que;
    };
} // namespace Revo_LLC