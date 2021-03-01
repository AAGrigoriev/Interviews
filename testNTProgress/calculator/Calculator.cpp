#include "Calculator.hpp"

namespace ntProgress
{
    double Calculator::calculate(std::queue<std::unique_ptr<IValue>> &que_in)
    {
        while (!que_in.empty())
        {
            que_in.front()->submit(*this);
            que_in.pop();
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