#include "dfs_bomber_man.hpp"

namespace dfs_bfs_resolver {

abstract_resolver::result dfs_bomber_man::calculate() {
  bomber_result res{};
  dfs(start_pos_x_, start_pos_y_, res);
  return res;
}

void dfs_bomber_man::dfs(int x, int y, bomber_result& res) {
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

    if (map_[tx][ty] == 2 && !visited_points_[tx][ty]) {
      visited_points_[tx][ty] = 1;
      dfs(tx, ty, res);
    }
  }
}

bool dfs_bomber_man::parse_header(const std::vector<std::size_t>& vec)
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
