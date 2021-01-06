#include <boost/algorithm/string.hpp>
#include <charconv>
#include "controller.hpp"

namespace testArgus
{
    void Controller::read_from_stream(std::istream &stream)
    {
        std::string input_string;
        std::vector<std::string> results;

        while (std::getline(stream, input_string))
        {
            boost::split(results, input_string, [](char c) { return c == ' '; });

            if (results.size() > 0)
            {
                if (results[0] == "q" || results[0] == "quit")
                {
                    break;
                }
                else if (results.size() > 2 && (results[0] == "load" || results[0] == "ld"))
                {
                    open_cv_w.load_image(results[1], results[2]);
                }
                else if (results.size() > 2 && (results[0] == "store" || results[0] == "s"))
                {
                    if (results[2].find(".jpg") != std::string::npos)
                        open_cv_w.store_image(results[1], results[2]);
                    else
                        std::cerr << "Error: need \"name.jpg\"\n";
                }
                else if (results.size() > 3 && results[0] == "blur")
                {
                    int size_blur;
                    if (auto [p, ec] = std::from_chars(results[3].data(), results[3].data() + results[3].size(), size_blur); ec == std::errc())
                    {
                        open_cv_w.blur_image(results[1], results[2], size_blur);
                    }
                    else
                    {
                        std::cerr << "Error : wrong size \n";
                    }
                }
                else if (results.size() > 4 && results[0] == "resize")
                {
                    int width, height;
                    do
                    {
                        if (auto [p, ec] = std::from_chars(results[3].data(), results[3].data() + results[3].size(), width); ec != std::errc())
                        {
                            std::cerr << "Error : wrong width \n";
                            break;
                        }
                        if (auto [p, ec] = std::from_chars(results[4].data(), results[4].data() + results[4].size(), height); ec != std::errc())
                        {
                            std::cerr << "Error : wrong height \n";
                            break;
                        }
                        open_cv_w.resize_image(results[1], results[2], width, height);
                        break;
                    } while (true);
                }
                else if (results[0] == "h" || results[0] == "help")
                {
                    std::cout << "load, ld <name> <filename> \n"
                              << "store, s <name> <filename> \n"
                              << "blur <from_name> <to_name> <size> \n"
                              << "resize <from_name> <to_name> <new_width> <new_height> \n"
                              << "quit, q - exit \n";
                }
                else
                {
                    std::cerr << "wrong commnad\n";
                }
            }
        }
    }
} // namespace testArgus