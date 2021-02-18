#pragma once

#include <string>

double recStart(std::string::const_iterator beg, std::string::const_iterator end);

bool is_number(char c);

double recMult(double number, std::string::const_iterator beg, std::string::const_iterator end);

double recDiv(double number, std::string::const_iterator beg, std::string::const_iterator end);

double recAdd(std::string::const_iterator beg, std::string::const_iterator end);

double recSub(std::string::const_iterator beg, std::string::const_iterator end);

double recursive_wrapper2(double number, std::string::const_iterator beg, std::string::const_iterator end);

