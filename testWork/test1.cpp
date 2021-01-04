/* Перевод целого положительного в двоичный вид */
#include <climits>
#include <array>
#include <iostream>
#include <bitset>

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
void print_binary(T number)
{
    std::size_t nBits = sizeof(T) * CHAR_BIT;

    std::size_t mask = 1U << (nBits - 1);

    for (std::size_t i = 0; i < nBits; ++i, mask >>= 1)
    {
        std::cout << ((number & mask) != 0);
    }
    std::cout << std::endl;
}

/* 2 вариант функции */
void to_binary(unsigned long n)
{
    unsigned int r = n % 2;
    if (n >= 2)
    {
        to_binary(n / 2);
    }
    std::cout << ((r == 1) ? 1 : 0);
}

#include <cstdlib>


int main()
{

    int array[3];

    *(array + 2) = 10;

    std::cout << array[2];

    // int x = 2000;
    //int x2 = -2000;

    // int x3 = 5;

    //print_binary(x);
    //print_binary(x2);
    // to_binary(x3);
    //std::cout << std::bitset<20>(x) << std::endl;
    //std::cout << std::bitset<20>(x2) << std::endl;

    return 0;
}