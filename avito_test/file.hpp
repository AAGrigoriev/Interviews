#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>


namespace avito_test {

std::vector<std::pair<std::size_t, std::string>> sorted_vector(std::filesystem::path path);

}

