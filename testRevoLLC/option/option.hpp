#pragma once

#include <vector>
#include <string>

/*!
    @brief Аналог функции boost::split 
    В задании запрет на библиотеки, поэтому пишем велосипеды 
*/
void split_string(std::vector<std::string>& out,std::string& in,std::string delim);