/*
    Не реализовано:
    - Обработка двух-трехзначных чисел
    - Нужно проверить наличие скобки на 1 месте, т.е (X+Y)......
    - Добавить поддержку многократных скобок
    - Проверка на ошибки при вводе 
 */

#include <iostream>
#include <test.hpp>

double recStart(std::string::const_iterator beg, std::string::const_iterator end)
{
    if (beg == end)
        return 0;

    return recursive_wrapper2(*beg - '0', beg, end);
}

bool is_number(char c)
{
    return c >= '0' || c <= '9';
}

double recAdd(std::string::const_iterator beg, std::string::const_iterator end)
{
    char symbol = *beg;

    if (symbol == '(')
    {
        auto next_iter = std::next(beg);
        return recursive_wrapper2(*next_iter - '0', next_iter, end);
    }
    else
    {
        if (is_number(symbol))
        {
            return recursive_wrapper2(symbol - '0', beg, end);
        }
    }
}

double recSub(std::string::const_iterator beg, std::string::const_iterator end)
{
    char symbol = *beg;

    if (symbol == '(')
    {
        auto next_iter = std::next(beg);
        return -recursive_wrapper2(*next_iter - '0', next_iter, end);
    }
    else
    {
        if (is_number(symbol))
        {
            return recursive_wrapper2(-(symbol - '0'), beg, end);
        }
    }
}

double recMult(double number, std::string::const_iterator beg, std::string::const_iterator end)
{
    char symbol = *beg;

    if (symbol == '(')
    {
        auto next_iter = std::next(beg);
        return number * recursive_wrapper2(*next_iter - '0', next_iter, end);
    }
    else
    {
        if (is_number(symbol))
        {
            return recursive_wrapper2(number * (symbol - '0'), beg, end);
        }
    }
}

double recDiv(double number, std::string::const_iterator beg, std::string::const_iterator end)
{
    char symbol = *beg;

    if (symbol == '(')
    {
        auto next_iter = std::next(beg);
        return number / recursive_wrapper2(*next_iter - '0', next_iter, end);
    }
    else
    {
        if (is_number(symbol))
        {
            return recursive_wrapper2(number / (symbol - '0'), beg, end);
        }
    }
}

double recursive_wrapper2(double number, std::string::const_iterator beg, std::string::const_iterator end)
{
    if (beg == end)
    {
        return number;
    }
    else
    {
        if (is_number(*beg))
        {
            auto iter_temp = std::next(beg);

            if (iter_temp == end)
            {
                return number;
            }

            auto next_elem = std::next(iter_temp);

            switch (*iter_temp)
            {
            case '+':
                return number + recAdd(next_elem, end);
            case '-':
                return number + recSub(next_elem, end);
            case '*':
                return recMult(number, next_elem, end);
            case '/':
                return recDiv(number, next_elem, end);
            case ')':
                return number;
            case '(':
                return recursive_wrapper2(0.0, next_elem, end);
            default:
                std::cerr << "wrong type";
                return 0;
                break;
            }
        }
    }
}