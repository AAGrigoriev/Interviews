#pragma once

#include <stack>
#include <unordered_map>
#include <string>

#include "option.hpp"

namespace ntProgress
{
    class Converter
    {
    public:
        Converter();
        Converter(const Converter &) = delete;
        Converter(Converter &&) = delete;

        que_rpn toRPN(const std::string &infix);

    private:
        void check_op(const char *symbol, que_rpn &out);
        void collapse_bracket(que_rpn &out);
        void assign_op(char op, que_rpn &out);

        std::stack<char> operand_;                  // Стек для временных операндов
        std::unordered_map<char, int8_t> priority_; // Таблица для приоритетов
    };
}