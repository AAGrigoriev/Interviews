#include <cctype>
#include <algorithm>
#include <stdexcept>
	

#include "Converter.hpp"
namespace ntProgress
{
    Converter::Converter() : allower_symbols_{'(', ')', '+', '-', '*', '/'}
    {
        priority_.insert({'(', 0});
        priority_.insert({'+', 1});
        priority_.insert({'-', 1});
        priority_.insert({'*', 2});
        priority_.insert({'/', 2});
    }

    std::queue<std::unique_ptr<IValue>> Converter::toRPN(const std::string &infix)
    {
        const char *p_value = infix.c_str();

        std::queue<std::unique_ptr<IValue>> out;

        while (*p_value)
        {
            if (std::isspace(*p_value))
            {
                ++p_value;
            }
            else if (std::isdigit(*p_value))
            {
                char *next = nullptr;
                out.push(std::unique_ptr<IValue>(new Value<double>(std::strtod(p_value, &next))));
                p_value = next;
            }
            else
            {
                check_op(p_value, out);
                ++p_value;
            }
        }

        if (!wrong_symbols_.empty())
        {
            throw std::invalid_argument("Invalid param " + wrong_symbols_);
        }

        while (!operand_.empty())
        {
            out.push(std::unique_ptr<IValue>(new Value<char>(operand_.top())));
            operand_.pop();
        }
        return out;
    }

    void Converter::check_op(const char *p_value, std::queue<std::unique_ptr<IValue>> &out)
    {
        char op = *p_value;
        switch (op)
        {
        case '(':
            operand_.push(op);
            break;
        case ')':
            collapse_bracket(out);
            break;
        default:
            assign_op(op, out);
            break;
        }
    }

    void Converter::collapse_bracket(std::queue<std::unique_ptr<IValue>> &out)
    {
        while (!operand_.empty())
        {
            char temp = operand_.top();
            if (temp != '(')
            {
                out.push(std::unique_ptr<IValue>(new Value<char>(temp)));
                operand_.pop();
            }
            else
            {
                operand_.pop();
                break;
            }
        }
    }

    void Converter::assign_op(char op, std::queue<std::unique_ptr<IValue>> &out)
    {
        if (std::find(allower_symbols_.begin(), allower_symbols_.end(), op) != std::end(allower_symbols_))
        {
            while (!operand_.empty() && priority_[op] <= priority_[operand_.top()])
            {
                out.push(std::unique_ptr<IValue>(new Value<char>(operand_.top())));
                operand_.pop();
            }
            operand_.push(op);
        }
        else
        {
            wrong_symbols_.push_back(op);
        }
    }
}