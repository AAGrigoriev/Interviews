#include "Calculator.hpp"
#include "Converter.hpp"
namespace ntProgress
{
    double Calculator::calculate(const std::string &infix_notation)
    {
        Converter conv;
        
        auto que_ = conv.toRPN(infix_notation);

        while (!que_.empty())
        {
            que_.front()->submit(*this);
            que_.pop();
        }
        return result.top();
    }

    double Calculator::top_pop() const 
    {
        double out = result.top();
        result.pop();
        return out;
    }

}