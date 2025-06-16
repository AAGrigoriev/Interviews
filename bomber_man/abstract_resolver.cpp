#include "abstract_resolver.hpp"

#include <fstream>
#include <string_view>
#include <vector>

namespace bomber_man {

namespace _ {

std::vector<std::size_t> split_string(std::string_view string, char token) {
  std::string_view::size_type begin = 0;
  std::string_view::size_type next = string.find(token);
  std::vector<std::size_t> result;

  for (; next != std::string_view::npos;
       begin = next + 1, next = string.find(token, begin)) {
    result.push_back(std::stoul(std::string{string.substr(begin, next)}));
  }

  result.push_back(std::stoul(std::string{string.substr(begin)}));

  return result;
}

}  // namespace _

abstract_resolver::abstract_resolver()
    : next_{std::array{0, 1}, {1, 0}, {0, -1}, {-1, 0}} {}

bool abstract_resolver::init_map(const std::filesystem::__cxx11::path& path) {
  namespace fs = std::filesystem;

  if (fs::is_regular_file(path)) {
    if (auto str = std::ifstream(path); str.is_open()) {
      std::string line;
      std::getline(str, line);
      if (const auto sizes = _::split_string(line, ';'); sizes.size() == 4) {
        row_count_ = sizes[0];
        col_count = sizes[1];
        start_pos_x_ = sizes[2];
        start_pos_y_ = sizes[3];

        visited_points_.resize(row_count_, std::vector<bool>(col_count, false));
        map_.reserve(row_count_);

        for (std::size_t i{}; i < row_count_; ++i) {
          std::vector<int> row;
          for (std::size_t j{}; j < col_count; ++j) {
            int elem{};
            str >> elem;
            row.push_back(elem);
          }
          map_.push_back(std::move(row));
        }
        return true;
      }
    }
  }
  return false;
}

int abstract_resolver::get_enemy_killed(int i, int j) {
  int result = 0, x, y;

  x = i;
  y = j;

  // Движение вверх
  while (map_[x][y] != 0) {
    if (map_[x][y] == 1) {
      ++result;
    }
    --x;
  }

  x = i;
  y = j;

  // вниз
  while (map_[x][y] != 0) {
    if (map_[x][y] == 1) {
      ++result;
    }
    ++x;
  }

  x = i;
  y = j;

  // лево
  while (map_[x][y] != 0) {
    if (map_[x][y] == 1) {
      ++result;
    }
    --y;
  }

  x = i;
  y = j;

  // право
  while (map_[x][y] != 0) {
    if (map_[x][y] == 1) {
      ++result;
    }
    ++y;
  }

  return result;
}

}  // namespace bomber_man
