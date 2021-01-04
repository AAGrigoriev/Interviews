#include <boost/algorithm/string.hpp>

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

            if (results.size() != 0)
            {
                if (results[0] == "q")
                    break;
                else if (results[0] == "load" || results[0] == "ld")
                {
                    //open_cv_wrapper.load_image(results[1],results[2]);
                }
                else if(results[0] == "store" || results[0] == "s")
                {

                }
                else if(results[0] == "blur")
                {

                }
                else if(results[0] == "resize")
                {

                }
                else if(results[0] == "h" || results[0] == "help")
                {

                }
            }
        }
    }

} // namespace testArgus