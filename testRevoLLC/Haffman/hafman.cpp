#include <fstream>
#include <algorithm>
#include "haffman.hpp"

namespace Revo_LLC
{

    void haffman_code::fill_freq(std::string const &in)
    {
        auto lamda = [this](char ch) {freq[ch]++;};
        std::for_each(in.begin(),in.end(),lamda);
    }

    void haffman_code::create_deque()
    {
        auto lamda = [this](auto& pair) { pr_que.emplace(std::make_unique(pair->first,pair->second));};
        std::for_each(freq.begin(),freq.end(),lamda);
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
                fill_freq(str);
            }
        }
    }

    void haffman_code::decode(std::string const &file_in, std::string const &file_out)
    {
    }

} // namespace Revo_LLC