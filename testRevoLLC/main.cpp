#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <array>

int main()
{
    // prepare file for next snippet

    std::array<char, 10000> buffer;

    if (std::ifstream is{"text_test.txt"})
    {
        if (is.read(buffer.data(), 10000))
        {
            std::cout << is.gcount() << std::endl;

            for (auto elem : buffer)
            {
                std::cout << elem;
            }
            std::cout << std::endl;
        }
        /*     while (is.read(buffer.data(), 10000))
        {
            std::cout << is.gcount() << std::endl;
            for (auto elem : buffer)
            {
                std::cout << elem;
            }
            std::cout << std::endl;
        }
*/
        /*        auto size = is.tellg();
        std::string str(size, '\0'); // construct string to stream size
        is.seekg(0);
        if (is.read(&str[0], size))
            std::cout << str << '\n';
    */
    }

    return 0;
}