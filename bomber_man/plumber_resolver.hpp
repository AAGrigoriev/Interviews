#pragma once

#include "abstract_resolver.hpp"

namespace dfs_bfs_resolver {

class plumber_resolver : public abstract_resolver {
public:
  virtual bool parse_header(const std::vector<std::size_t>& vec) override;
  virtual result calculate() override;

private:
  struct note {
    int x;
    int y;
  };

private:
  void dfs (int x, int y, int front);

private:
  std::vector<note> path_;
  bool flag = false;
};


} // namespace dfs_bfs_resolver
