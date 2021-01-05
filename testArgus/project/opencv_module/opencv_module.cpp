#include <iostream>
#include <opencv_module.hpp>

using namespace cv;

namespace testArgus
{
    void opencv_wrapper::load_image(const std::string &name, const std::string &file_name)
    {
        Mat in_im;
        in_im = imread(file_name, IMREAD_UNCHANGED);

        if (in_im.empty())
        {
            std::cerr << "Error: load image failed \n";
        }

        const auto [hint, success] = map_image.insert({name, in_im});
        if (!success)
        {
            std::cerr << "Error: name is already taken \n";
        }
    }

    void opencv_wrapper::store_image(const std::string &name, const std::string &file_name)
    {
        if (auto pair = map_image.find(name); pair != map_image.end())
        {
            imwrite(file_name, pair->second);
        }
        else
        {
            std::cerr << "Error: not found image\n";
        }
    }

    void opencv_wrapper::blur_image(const std::string &from_name, const std::string &to_name, int size)
    {
        if (auto pair = map_image.find(from_name); pair != map_image.end())
        {
            Mat dst;
            medianBlur(pair->second, dst, size);
            const auto [hint, success] = map_image.insert({to_name, dst});
            if (!success)
            {
                std::cerr << "Error: name is already taken \n";
            }
        }
        else
        {
            std::cerr << "Error: not found image to blur\n";
        }
    }

    void opencv_wrapper::resize_image(const std::string &from_name, const std::string &to_name, int new_width, int new_height)
    {
        if (auto pair = map_image.find(from_name); pair != map_image.end())
        {
            Mat dst;
            resize(pair->second, dst, Size(new_width,new_height));
            const auto [hint, success] = map_image.insert({to_name, dst});
            if (!success)
            {
                std::cerr << "Error: name is already taken \n";
            }
        }
        else
        {
            std::cerr << "Error: not found image to blur\n";
        }
    }

} // namespace testArgus