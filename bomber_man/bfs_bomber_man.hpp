#pragma once

#include <queue>

#include "abstract_resolver.hpp"

namespace dfs_bfs_resolver {

class bfs_bomber_man : public abstract_resolver {
 public:
   struct note {
     int x;
     int y;
   };
 public:
  result calculate() override final;
  bool parse_header(const std::vector<std::size_t>& vec) override;

 private:
  int start_pos_x_{};
  int start_pos_y_{};
  std::queue<note> queue_;
};

}  // namespace dfs_bfs_resolver
