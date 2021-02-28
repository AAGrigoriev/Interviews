#include <cctype>
#include "RPNConverter.hpp"

namespace ntProgress
{
    Converter::Converter()
    {
        priority_.insert({'(', 0});
        priority_.insert({'+', 1});
        priority_.insert({'-', 1});
        priority_.insert({'*', 2});
        priority_.insert({'/', 2});
    }

    std::stack<std::unique_ptr<IValue>> Converter::toRPN(const std::string &infix)
    {
        const char *p_value = infix.c_str();

        std::stack<std::unique_ptr<IValue>> out;

        while (*p_value)
        {
            if (std::isspace(*p_value))
            {
                ++p_value;
            }

            if (std::isdigit(*p_value))
            {
                char *next = nullptr;
                out.push(new Value<double>(std::strtod(p_value, &next)));
                p_value = next;
            }
            else
            {
                check_op(p_value, out);
            }
        }
    }

    void Converter::check_op(const char *p_value, std::stack<std::unique_ptr<IValue>> &out)
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

    void Converter::collapse_bracket(std::stack<std::unique_ptr<IValue>> &out)
    {
        while (!operand_.empty())
        {
            char temp = operand_.top();
            if (temp != '(')
            {
                out.push(new Value<char>(temp));
                operand_.pop();
            }
            else
            {
                operand_.pop();
                break;
            }
        }
    }

    void Converter::assign_op(char op, std::stack<std::unique_ptr<IValue>> &out)
    {
    }
}