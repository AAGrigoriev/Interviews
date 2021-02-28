#pragma once

#include <type_traits>

#include "option.hpp"
#include "Converter.hpp"
namespace ntProgress
{
    class Calculator
    {
    private:
        /* Итоговые значения для вывода */
        std::stack<double> result;

        template <typename T>
        void visitValue(T value);

        double top_pop();

    public:
        Calculator(/* args */) = default;
        ~Calculator() = default;
        double calculate(que_rpn &que_in);

        template <typename T>
        friend class Value;
    };

    template <typename T>
    void Calculator::visitValue(T value)
    {
        if constexpr (std::is_floating_point<T>::value)
        {
            result.push(value);
        }
        else
        {
            switch (value)
            {
            case '+':
            {
                auto right = top_pop();
                auto left = top_pop();
                result.push(left + right);
                break;
            }
            case '*':
            {
                auto right = top_pop();
                auto left = top_pop();
                result.push(left * right);
            }
            break;
            case '-':
            {
                auto right = top_pop();
                auto left = top_pop();
                result.push(left - right);
            }
            break;
            case '/':
            {
                auto right = top_pop();
                auto left = top_pop();
                result.push(left / right);
            }
            break;
            default:
                break;
            }
        }
    }
}