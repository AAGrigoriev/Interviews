#include "dfs_bomber_man.hpp"

namespace bomber_man {

abstract_bomber_man::result dfs_bomber_man::calculate() {
  result res{};
  dfs(start_pos_x_, start_pos_y_, res);
  return res;
}

void dfs_bomber_man::dfs(int x, int y, result& res) {
  const auto sum = get_enemy_killed(x, y);

  if (sum > res.sum) {
    res.sum = sum;
    res.mx = x;
    res.my = y;
  }

  for (auto k = 0; k < next_.size(); ++k) {
    auto tx = x + next_[k][0];
    auto ty = y + next_[k][1];

    if (tx < 0 || tx >= row_count_ || ty < 0 || ty >= col_count) continue;

    if (map_[tx][ty] == 2 && visited_points_[tx][ty] == 0) {
      visited_points_[tx][ty] = 1;
      dfs(tx, ty, res);
    }
  }

  return;
}

}  // namespace bomber_man
