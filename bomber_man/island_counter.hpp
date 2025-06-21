#pragma once

#include "abstract_resolver.hpp"

namespace dfs_bfs_resolver {

class island_counter : public abstract_resolver
{
public:
  result calculate() override final;

  bool parse_header(const std::vector<std::size_t>& vec) override;

private:
  void dfs(int x, int y, int current_color);
};

} // namespace bomber_man

