#include "file.hpp"

#include <algorithm>
#include <fstream>
#include <regex>


namespace avito_test {

std::vector<std::string> split_string(const std::string& input) {
  std::regex regex(R"([^a-zA-Z]+)");
  return {std::sregex_token_iterator{input.begin(), input.end(), regex, -1},
           std::sregex_token_iterator{}};
}


auto create_freq_map(std::filesystem::path path) {
  auto map_comp = [](const std::string& a, const std::string& b) {
    auto string_comp = [](auto c1, auto c2) {
      return std::tolower(c1) < std::tolower(c2);
    };
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), string_comp);
  };

  std::map<std::string, std::size_t, decltype (map_comp)> out(map_comp);
  std::ifstream input{path};
  std::string buffer;

  auto fill_map = [](auto& map, auto&& vec) {
    for (auto&& str : vec) {
      map[std::move(str)]++;
    }
  };

  /// TODO: размер можно подобрать получше
  if (std::filesystem::file_size(path) < 1'000'000'000) {
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    input.seekg(0);
    buffer.resize(size);
    input.read(buffer.data(), size);
    fill_map(out, split_string(buffer));
  } else {
    while (std::getline(input, buffer)) {
      auto vec_string = split_string(buffer);
      fill_map(out, split_string(buffer));
    }
  }

  input.close();
  return out;
}


std::vector<std::pair<std::size_t, std::string>> sorted_vector(std::filesystem::path path) {
  std::vector<std::pair<std::size_t, std::string>> out;
  auto freq_map = create_freq_map(path);

  out.reserve(freq_map.size());
  for(auto&& p : freq_map) {
    out.emplace_back(p.second, p.first);
  }

  auto comp = [](auto& first, auto& second) {
    return first.first > second.first;
  };

  std::stable_sort(out.begin(), out.end(), comp);

  return out;
}
}
