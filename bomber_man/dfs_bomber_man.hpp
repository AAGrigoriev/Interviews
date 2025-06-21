#pragma once

#include "abstract_resolver.hpp"

namespace dfs_bfs_resolver {

class dfs_bomber_man : public abstract_resolver {
 public:
  result calculate() override final;

  bool parse_header(const std::vector<std::size_t>& vec) override;

 private:
  void dfs(int x, int y, bomber_result& res);

 private:
  int start_pos_x_{};
  int start_pos_y_{};
};

}  // namespace bomber_man
