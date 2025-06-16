#include "island_counter.hpp"

#include <iostream>
#include <iomanip>

namespace bomber_man {

abstract_resolver::result island_counter::calculate() {
  int current_color = 0;
  for (std::size_t i{}; i < row_count_; ++i) {
    for (std::size_t j{}; j < col_count; ++j) {
      if (map_[i][j] > 0) {
        --current_color;
        dfs(i, j, current_color);
      }
    }
  }

  const auto width = std::to_string(-current_color).length() + 1;
  for (std::size_t i{}; i < row_count_; ++i) {
    for (std::size_t j{}; j < col_count; ++j) {
      std::cout << std::setw(width) << map_[i][j] << " ";
    }
    std::cout << std::endl;
  }

  result res; // todo
  return res;
}

void island_counter::dfs(int x, int y, int current_color) {
  visited_points_[x][y] = true;
  map_[x][y] = current_color;
  for (const auto& dir : next_) {
    int tx = x + dir[0];
    int ty = y + dir[1];
    if (tx < 0 || tx >= row_count_ || ty < 0 || ty >= col_count) continue;
    if (map_[tx][ty] > 0 && !visited_points_[tx][ty])
    {
      dfs(tx, ty, current_color);
    }
  }
}
}
