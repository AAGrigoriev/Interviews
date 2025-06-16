#pragma once

#include "abstract_resolver.hpp"

namespace bomber_man {

class island_counter : public abstract_resolver
{
public:
  result calculate() override final;

private:
  void dfs(int x, int y, int current_color);
};

} // namespace bomber_man

