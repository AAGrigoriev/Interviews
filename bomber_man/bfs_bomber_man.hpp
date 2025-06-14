#pragma once

#include <queue>

#include "abstract_bomber_man.hpp"

namespace bomber_man {

class bfs_bomber_man : public abstract_bomber_man {
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
