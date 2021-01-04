#pragma once

#include <string>

namespace testArgus
{
    class open_cv_wrapper
    {
    public:
        void load_image(std::string &name, std::string &file_name) {}

        void store_image(std::string &name, std::string &file_name) {}

        void blur(std::string &from_name, std::string &to_name, int size) {}

        void resize(std::string &from_name, std::string &to_name, int new_width, int new_height) {}

    private:
        // map
    };
} // namespace testArgus