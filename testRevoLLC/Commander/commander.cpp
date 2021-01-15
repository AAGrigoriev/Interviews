#include <commander.hpp>
#include <option.hpp>

static std::size_t command_size = 3; // Я знаю, что исправная комманда состоит из 3 слов

namespace Revo_LLC
{
    commander::commander()
    {
        using std::placeholders::_1;
        using std::placeholders::_2;

        command_table["encode"] = std::bind(&haffman_code::encode, haffman, _1, _2);
        command_table["decode"] = std::bind(&haffman_code::decode, haffman, _1, _2);
    }

    void commander::read_stream(std::istream &in)
    {
        std::string command;
        std::vector<std::string> v_string;

        v_string.reserve(command_size);

        while (std::getline(in, command))
        {
            split_string(v_string, command, " ");

            if (v_string.size() < command_size)
                break;

            if (auto it = command_table.find(v_string[0]); it != command_table.end())
            {
                it->second(v_string[1], v_string[2]);
            }
            else
            {
                std::cerr << "wrong command\n";
            }
        }
    }

} // namespace Revo_LLC