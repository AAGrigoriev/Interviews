#include "Commander/commander.hpp"

using namespace Revo_LLC;

#include <bitset>

int main()
{

    /*  auto value = std::bitset<1>(1);
    std::cout << " dgsdgsdgsdg\n";
    std::cout << value.size();
*/
    //commander com;

    //com.read_stream(std::cin);

    /*    std::ofstream out{"text222.txt", std::ios::binary};

    std::bitset<8> bit_1(19);

    std::cout << bit_1 << std::endl;

    std::cout << bit_1.size() << std::endl;

    ulong value = bit_1.to_ulong();

    out.write((const char *)(&value), bit_1.size());

    out.close();

    std::ifstream in{"text222.txt", std::ios::binary};

    ulong value1;

    in.read((char *)(&value1), 8);

    std::cout << value1 << std::endl;
  
  */

    unsigned char a = 256;

    //a |= 1U << 8;

    printf("a = %u\n", a);

    return 0;
}