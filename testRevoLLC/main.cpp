#include "Commander/commander.hpp"

using namespace Revo_LLC;

#include <bitset>

int main()
{

    /*  auto value = std::bitset<1>(1);
    std::cout << " dgsdgsdgsdg\n";
    std::cout << value.size();
*/
    commander com;

    com.read_stream(std::cin);

    return 0;
}