#pragma once

#include <array>
#include <filesystem>
#include <vector>

namespace bomber_man {

class abstract_bomber_man {
 public:
  struct result {
    int sum{};
    int mx{};
    int my{};
  };

 protected:
  using square_matrix = std::vector<std::vector<int>>;
  using visited_matrix = std::vector<std::vector<bool>>;
  using direction = std::array<std::array<int, 2>, 4>;

 public:
  abstract_bomber_man();

  bool init_map(const std::filesystem::path& path);

  virtual result calculate() = 0;

 protected:
  int get_enemy_killed(int i, int j);

 protected:
  direction next_;
  square_matrix map_;
  visited_matrix visited_points_;
  int start_pos_x_;
  int start_pos_y_;
  std::size_t row_count_{};
  std::size_t col_count{};
};
}  // namespace bomber_man
