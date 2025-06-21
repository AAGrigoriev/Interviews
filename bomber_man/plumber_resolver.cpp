#include "plumber_resolver.hpp"

#include <iostream>

namespace dfs_bfs_resolver {

bool plumber_resolver::parse_header(const std::vector<std::size_t>& vec)
{
  if (vec.size() == 2) {
    row_count_ = vec[0];
    col_count = vec[1];
    return true;
  }
  return false;
}

abstract_resolver::result plumber_resolver::calculate()
{
  dfs(0, 0, 1);
  return flag;
}

void plumber_resolver::dfs(int x, int y, int front)
{
  if (x == row_count_ - 1 && y == col_count) {
    flag = true;
    for (const auto point : path_) {
        std::cout << point.x << " " << point.y << std::endl;
    }
    return;
  }

  if (x < 0 || x >= row_count_ || y < 0 || y >= col_count)
    return;

  if (visited_points_[x][y])
    return;

  visited_points_[x][y] = true;

  path_.push_back(note{x, y});
  bool result = false;
  // Прямые трубы
  if (map_[x][y] >= 5 && map_[x][y] <= 6) {
    if (front == 1) { // вход слева
      dfs(x, y + 1, 1);
    } else if (front == 2) { // вход сверху
      dfs(x + 1, y, 2);
    } else if (front == 3) { // вход справа
      dfs(x, y - 1, 3);
    } else if (front == 4) { // вход снизу
      dfs(x - 1, y, 4);
    }
  } else if (map_[x][y] >= 1 && map_[x][y] <= 4) { // Угловые трубы
    if (front == 1) { // вход слева
      dfs(x + 1, y, 2);
      dfs(x - 1, y, 4);
    } else if (front == 2) { // вход сверху
      dfs(x, y + 1, 1);
      dfs(x, y - 1, 3);
    } else if (front == 3) { // вход справа
      dfs(x - 1, y, 4);
      dfs(x + 1, y, 2);
    } else if (front == 4) { // вход снизу
      dfs(x, y + 1, 1);
      dfs(x, y - 1, 3);
    }
  }

  visited_points_[x][y] = false;
  path_.pop_back();


  return;
}
} // dfs_bfs_resolver


