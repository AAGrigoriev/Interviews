#include <option/option.hpp>

#include <iostream>

void split_string(std::vector<std::string> &out, std::string &in, std::string delim)
{
    std::string::size_type start = 0U;
    std::string::size_type end = in.find(delim);

    while (end != std::string::npos)
    {
        out.emplace_back(in.substr(start, end - start));
        start = end + delim.length();
        end = in.find(delim, start);
    }
    out.emplace_back(in.substr(start, end - start));
}