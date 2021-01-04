#pragma once

#include "openc_cv_module/open_cv.hpp"

#include <vector>

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
        open_cv_wrapper open_cv_w;
    };
} // namespace testArgus
