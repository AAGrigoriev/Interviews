#include "bfs_bomber_man.hpp"

#include <vector>

namespace bomber_man {

// 0 - стена
// 1 - враг
// 2 - свободное поле
bfs_bomber_man::result bfs_bomber_man::calculate() {
  result res;
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

}  // namespace bomber_man
