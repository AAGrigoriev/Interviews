#pragma once

#include <stack>
#include <memory>
#include <unordered_map>
#include <string>

namespace ntProgress
{
    /* Можно через гетерогенный контейнер std::any или std::variant */
    struct IValue
    {
        virtual void submit() = 0;
        virtual ~IValue() = default;
    };

    /* Для char/double */
    template <typename T>
    struct Value : IValue
    {
        Value(T val): value1_(val);
        void submit() override;
        virtual Value() = default;

    private:
        T value_;
    };

    class Converter
    {
        using stack_rpn = std::stack<std::unique_ptr<IValue>>;

    public:
        Converter();
        Converter(const Converter &) = delete;
        Converter(Converter &&) = delete;

        std::stack<std::unique_ptr<IValue>> toRPN(const std::string &infix);

    private:
        void check_op(const char* symbol,std::stack<std::unique_ptr<IValue>>& out);
        void collapse_bracket(std::stack<std::unique_ptr<IValue>>& out);
        void assign_op(char op,std::stack<std::unique_ptr<IValue>>& out);

        std::stack<char>operand_;                  // Стек для временных операндов
        std::unordered_map<char, int8_t> priority_; // Таблица для приоритетов
    };
}