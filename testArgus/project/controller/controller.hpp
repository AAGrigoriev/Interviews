#pragma once

#include <vector>
#include "opencv_module.hpp"


namespace testArgus
{
    /*!
    @brief 
    */
    class Controller
    {

    public:
        Controller() = default;
        ~Controller() = default;

        /* */
        void read_from_stream(std::istream &stream);

    private:
        /**/
        opencv_wrapper open_cv_w;
    };
} // namespace testArgus
