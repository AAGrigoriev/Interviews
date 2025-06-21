#include <iostream>

#include "bfs_bomber_man.hpp"
#include "dfs_bomber_man.hpp"
#include "island_counter.hpp"
#include "plumber_resolver.hpp"

int main() {
  // bomber_man::bfs_bomber_man bfs_bm;
  // if (bfs_bm.init_map(std::filesystem::path("map.txt"))) {
  //   const auto res = bfs_bm.calculate();
  //   std::cout << res.sum << " " << res.mx << " " << res.my << std::endl;
  // }

  // dfs_bfs_resolver::dfs_bomber_man dfs_bm;
  // if (dfs_bm.init_map(std::filesystem::path("map.txt"))) {
  //   const auto res = dfs_bm.calculate();
  //   std::cout << res.sum << " " << res.mx << " " << res.my << std::endl;
  // }

  //dfs_bfs_resolver::island_counter dfs_island;
  //if (dfs_island.init_map(std::filesystem::path("island_map.txt"))) {
  //  dfs_island.calculate();
  //}

  dfs_bfs_resolver::plumber_resolver dfs_blumber;
  if (dfs_blumber.init_map(std::filesystem::path("plumber.txt"))) {
    dfs_blumber.calculate();
  }
  return 0;
}
