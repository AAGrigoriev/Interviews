#pragma once

#include <queue>

#include "abstract_resolver.hpp"

namespace bomber_man {

class bfs_bomber_man : public abstract_resolver {
 public:
  result calculate() override final;

  struct note {
    int x;
    int y;
  };

 private:
  std::queue<note> queue_;
};

}  // namespace bomber_man
