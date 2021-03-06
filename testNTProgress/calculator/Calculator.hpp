#pragma once

#include <type_traits>
#include <stack>
#include <queue>
#include <memory>
#include <string>

namespace ntProgress
{
    class Calculator;

    /* Можно через гетерогенный контейнер std::any или std::variant */
    struct IValue
    {
        virtual void submit(const Calculator &calc) = 0;
        virtual ~IValue() = default;
    };

    /* Для char/double */
    template <typename T>
    struct Value : public IValue
    {
        Value(T val) noexcept : value_(val) {}
        void submit(const Calculator &calc) override
        {
            calc.visitValue(value_);
        }
        virtual ~Value() = default;

    private:
        T value_;
    };

    class Calculator
    {
    private:
        /* Итоговые значения для вывода */
        mutable std::stack<double> result;

        template <typename T>
        void visitValue(T value) const;

        double top_pop() const;

    public:
        Calculator(/* args */) = default;
        ~Calculator() = default;
        double calculate(const std::string &infix_notation);

        template <typename T>
        friend class Value;
    };

    template <typename T>
    void Calculator::visitValue(T value) const
    {
        if constexpr (std::is_floating_point<T>::value)
        {
            result.push(value);
        }
        else
        {
            if (result.size() == 1)
            {
                if(value == '-')
                    result.push(-top_pop());
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
}