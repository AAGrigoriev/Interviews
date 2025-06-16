#pragma once

#include "abstract_resolver.hpp"

namespace bomber_man {

class dfs_bomber_man : public abstract_resolver {
 public:
  result calculate() override final;

 private:
  void dfs(int x, int y, result& res);
};

}  // namespace bomber_man
