#include "bfs_bomber_man.hpp"

#include <vector>

namespace dfs_bfs_resolver {

// 0 - стена
// 1 - враг
// 2 - свободное поле
bfs_bomber_man::result bfs_bomber_man::calculate() {
  bomber_result res;
  res.mx = start_pos_x_;
  res.my = start_pos_y_;
  res.sum = get_enemy_killed(start_pos_x_, start_pos_y_);
  queue_.push(note{start_pos_x_, start_pos_y_});

  while (!queue_.empty()) {
    const auto front = queue_.front();
    queue_.pop();

    for (const auto& dir : next_) {
      int tx = front.x + dir[0];
      int ty = front.y + dir[1];

      if (tx < 0 || tx >= row_count_ || ty < 0 || ty >= col_count) continue;

      if (map_[tx][ty] == 2 && !visited_points_[tx][ty]) {
        visited_points_[tx][ty] = true;
        queue_.push(note{tx, ty});
        if (const auto temp_sum = get_enemy_killed(tx, ty);
            temp_sum > res.sum) {
          res.sum = temp_sum;
          res.mx = tx;
          res.my = ty;
        }
      }
    }
  }
  return res;
}

bool bfs_bomber_man::parse_header(const std::vector<std::size_t>& vec)
{
  if (vec.size() == 4) {
    row_count_ = vec[0];
    col_count = vec[1];
    start_pos_x_ = vec[2];
    start_pos_y_ = vec[3];
    return true;
  }
  return false;
}

}  // namespace bomber_man
