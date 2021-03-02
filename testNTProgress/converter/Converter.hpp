#pragma once

#include <unordered_map>
#include <string>
#include <array>

#include "Calculator.hpp"

namespace ntProgress
{
    class Converter
    {
    public:
        Converter();
        Converter(const Converter &)    = delete;
        Converter(Converter &&)         = delete;

        std::queue<std::unique_ptr<IValue>> toRPN(const std::string &infix);

    private:
        void check_op(const char *symbol, std::queue<std::unique_ptr<IValue>> &out);
        void collapse_bracket(std::queue<std::unique_ptr<IValue>> &out);
        void assign_op(char op, std::queue<std::unique_ptr<IValue>> &out);

        std::stack<char>                 operand_;                  // Стек для временных операндов
        std::array<char,6>               allower_symbol_;           // Массив разрешенных символов 
        std::unordered_map<char, int8_t> priority_;                 // Таблица для приоритетов
    };
}