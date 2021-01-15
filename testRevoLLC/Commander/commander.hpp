#pragma once

#include <unordered_map>
#include <functional>
#include <iostream>
#include "haffman.hpp"

/*!
    @brief
*/
namespace Revo_LLC
{
    class commander
    {
    public:
        commander();
        ~commander() = default;

        void read_stream(std::istream &in);

    private:
        haffman_code haffman;

        std::unordered_map<std::string, std::function<void(std::string const &, std::string const &)>> command_table;
    };
} // namespace Revo_LLC