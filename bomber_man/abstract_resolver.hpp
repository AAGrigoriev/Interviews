#pragma once

#include <any>
#include <array>
#include <filesystem>
#include <vector>

namespace dfs_bfs_resolver {

class abstract_resolver {
 public:
  struct bomber_result {
    int sum{};
    int mx{};
    int my{};
  };

  using result = std::any;

 protected:
  using square_matrix = std::vector<std::vector<int>>;
  using visited_matrix = std::vector<std::vector<bool>>;
  using direction = std::array<std::array<int, 2>, 4>;

 public:
  abstract_resolver();

  bool init_map(const std::filesystem::path& path);

  virtual bool parse_header(const std::vector<std::size_t>& vec) = 0;
  virtual result calculate() = 0;

 protected:
  int get_enemy_killed(int i, int j);

 protected:
  direction next_;
  square_matrix map_;
  visited_matrix visited_points_;
  std::size_t row_count_{};
  std::size_t col_count{};
};
}  // namespace dfs_bfs_resolver
