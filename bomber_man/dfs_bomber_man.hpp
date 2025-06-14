#pragma once

#include "abstract_bomber_man.hpp"

namespace bomber_man {

class dfs_bomber_man : public abstract_bomber_man {
 public:
  result calculate() override final;

 private:
  void dfs(int x, int y, result& res);
};

}  // namespace bomber_man
