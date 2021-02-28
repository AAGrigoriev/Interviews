#pragma once

#include <queue>
#include <memory>
#include "Calculator.hpp"

namespace ntProgress
{
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
        Value(T val) : value_(val) {}
        void submit(const Calculator &calc) override
        {
            calc.visitValue(value_);
        }
        virtual ~Value() = default;

    private:
        T value_;
    };

    using que_rpn = std::queue<std::unique_ptr<IValue>>;
}