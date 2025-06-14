#include <iostream>

#include "bfs_bomber_man.hpp"
#include "dfs_bomber_man.hpp"

int main() {
  bomber_man::bfs_bomber_man bfs_bm;
  if (bfs_bm.init_map(std::filesystem::path("map.txt"))) {
    const auto res = bfs_bm.calculate();
    std::cout << res.sum << " " << res.mx << " " << res.my << std::endl;
  }

  bomber_man::dfs_bomber_man dfs_bm;
  if (dfs_bm.init_map(std::filesystem::path("map.txt"))) {
    const auto res = dfs_bm.calculate();
    std::cout << res.sum << " " << res.mx << " " << res.my << std::endl;
  }

  return 0;
}
