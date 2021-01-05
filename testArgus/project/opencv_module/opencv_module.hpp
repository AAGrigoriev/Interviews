#pragma once

#include <string>
#include <map>
#include <opencv4/opencv2/opencv.hpp>

namespace testArgus
{
    class opencv_wrapper
    {
    public:
        void load_image(const std::string &name, const std::string &file_name);

        void store_image(const std::string &name, const std::string &file_name);
        
        /*! 
            @brief Функция сглаживания 
            В функции используется медианный фидьтр
        */
        void blur_image(const std::string &from_name, const std::string &to_name, int size);

        void resize_image(const std::string &from_name, const std::string &to_name, int new_width, int new_height);

    private:
        std::map<std::string, cv::Mat> map_image;
    };
} // namespace testArgus