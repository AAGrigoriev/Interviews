#pragma once


#include <cstdint>
#include <unordered_set>

#include <lde/cellfy/boox/cell_value.h>
#include <lde/cellfy/boox/format.h>
#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/node.h>
#include <lde/cellfy/boox/range_op.h>


namespace lde::cellfy::boox {


using cell_index_set = std::unordered_set<cell_index>;

using double_table    = std::vector<std::pair<cell_index, double>>;
using wstring_table   = std::vector<std::pair<cell_index, std::wstring>>;
using rich_text_table = std::vector<std::pair<cell_index, rich_text>>;
using ast_table       = std::vector<std::pair<cell_index, fx::ast::tokens>>;
using bool_table      = std::vector<std::pair<cell_index, bool>>;
using error_table     = std::vector<std::pair<cell_index, cell_value_error>>;

using double_subrange    = std::pair<std::optional<cell_format>, double_table>;
using wstring_subrange   = std::pair<std::optional<cell_format>, wstring_table>;
using rich_text_subrange = std::pair<std::optional<cell_format>, rich_text_table>;
using ast_subrange       = std::pair<std::optional<cell_format>, ast_table>;
using bool_subrange      = std::pair<std::optional<cell_format>, bool_table>;
using error_subrange     = std::pair<std::optional<cell_format>, error_table>;
// Диапазоны значений ячеек разбитые по типам и форматам. Пример: boox::range в виде 1 столбца.
//   index    type     number_format
//    0      double       data        - > first ranges
//    1      empty        empty       - > first ranges  - пустые ячейки не учитываются при рассчётах, только при применении к новому boox::range.
//    2      double       data        - > first ranges
//    3      double       exp         - > second ranges - несмотря на то что тип один и тот же, но формат отличается, поэтому это уже другой поддиапазон.
//    4      string       dur         - > third ranges  - другой тип, другой поддиапазон
//    5      string       exp         - > fourth ranges
// Относительные индексы от начала поддиапазона с [0, 2 - 5] тоже сохраняются и используются при применении к новому boox::range,
// чтобы так же пропускать пустые ячейки при копировании в новый boox::range.
// Поскольку разбиваются по значениям и по формату, то также сохраняется формат для каждого диапазона, что потом его применить на новый boox::range.
using value_ranges = std::vector<std::variant<double_subrange, wstring_subrange, rich_text_subrange, ast_subrange, bool_subrange, error_subrange>>;


template<typename Fn>
class cell_nodes_visitor_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  explicit cell_nodes_visitor_op(Fn&& fn) noexcept
    : fn_(std::move(fn)) {
  }

  bool on_existing_node(range_op_ctx&, cell_node::it node) override {
    return fn_(node);
  }

private:
  Fn fn_;
};


class get_cell_format_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  explicit get_cell_format_op(cell_format& format) noexcept;

  void on_start(range_op_ctx& ctx) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  cell_format* format_          = nullptr;
  std::size_t  processed_count_ = 0;
};


class clear_cell_format_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  clear_cell_format_op() = default;
  clear_cell_format_op(cell_index_set&& except) noexcept;

  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  cell_index_set except_;
};


class change_existing_cell_format_op : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  explicit change_existing_cell_format_op(cell_format::changes changes) noexcept;

  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

protected:
  cell_format::changes changes_;
};


class change_cell_format_op final : public change_existing_cell_format_op {
public:
  using processing = for_all_cells_tag;

public:
  explicit change_cell_format_op(cell_format::changes changes) noexcept;

  bool on_new_node(range_op_ctx& ctx, cell_node& node) override;
};


class get_cell_value_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  explicit get_cell_value_op(cell_value& value) noexcept;

  void on_start(range_op_ctx& ctx) override;
  void on_finish(range_op_ctx& ctx) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  cell_value* value_           = nullptr;
  std::size_t processed_count_ = 0;
  std::size_t existing_count_  = 0;
};


class get_cell_values_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  explicit get_cell_values_op(cell_value::list& values) noexcept;

  void on_start(range_op_ctx& ctx) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  cell_value::list* values_ = nullptr;
};


class get_cell_values_matrix_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  explicit get_cell_values_matrix_op(cell_value::matrix& values) noexcept;

  void on_start(range_op_ctx& ctx) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  cell_value::matrix* values_ = nullptr;
};


// Для заполнения и индексирования ячеек используем относительные индексы от начала диапазона.
// Это нужно, чтобы пропускать пустые ячейки между существующими при применении на новый range.
// Для этого сохраняем стартовые значения строки и столбца и количесвто столбцов. Чтобы для каждой существующей ячейки можно было вычислить её
// относительный индекс по формуле: index = i * column_count + j.
class split_by_format_and_type final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  explicit split_by_format_and_type(value_ranges& value_ranges) noexcept;

  void on_start(range_op_ctx& ctx) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it it) override;

private:
  using value_data = std::variant<fx::ast::tokens, double, std::wstring, rich_text, bool, cell_value_error>;
  // Разбивается boox::range, если отличаются cell_value::type или cell_format.
  struct cell_format_type final {
    std::optional<cell_value_type> type;
    std::optional<cell_format>     format;
  };

private:
  auto get_value_data_from_node(range_op_ctx& ctx, cell_node::it it);
  void create_new_subrange(range_op_ctx& ctx, cell_node::it it, cell_index cell_index);
  void expand_current_subrange(range_op_ctx& ctx, cell_node::it it, cell_index cell_index);

private:
  value_ranges&    value_ranges_ ;          ///< Поддиапазоны, заполняется из текущего boox::range.
  cell_format_type format_type_;            ///< Типы по которым разбиваются boox::range на диапазоны.
  column_index     start_column_index_ = 0; ///< Cтартовый индекс столбца.
  row_index        start_row_index_    = 0; ///< Стартовый индекс строки.
  std::size_t      columns_count_      = 0; ///< Количество столбцов.
};


class clear_cell_value_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  clear_cell_value_op() = default;
  clear_cell_value_op(cell_index_set&& except) noexcept;

  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  cell_index_set except_;
};


class set_cell_value_op final : public range_op {
public:
  using processing = for_all_cells_tag;

public:
  explicit set_cell_value_op(const cell_value& value) noexcept;

  bool on_new_node(range_op_ctx& ctx, cell_node& node) override;
  bool on_new_node(range_op_ctx& ctx, cell_node::it node) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  const cell_value* value_ = nullptr;
};


class get_cell_formula_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  explicit get_cell_formula_op(std::wstring& formula) noexcept;

  void on_start(range_op_ctx& ctx) override;
  void on_finish(range_op_ctx& ctx) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  std::wstring* formula_         = nullptr;
  std::size_t   processed_count_ = 0;
  std::size_t   existing_count_  = 0;
};


class clear_cell_formula_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  clear_cell_formula_op() = default;
  clear_cell_formula_op(cell_index_set&& except) noexcept;

  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  cell_index_set except_;
};


class set_cell_formula_op final : public range_op {
public:
  using processing = for_all_cells_tag;

public:
  explicit set_cell_formula_op(const std::wstring& formula);

  bool on_new_node(range_op_ctx& ctx, cell_node& node) override;
  bool on_new_node(range_op_ctx& ctx, cell_node::it node) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  std::wstring formula_;
};


class merge_cells_op final : public range_op {
public:
  using processing = for_all_cells_tag;

public:
  void on_start(range_op_ctx& ctx) override;
  bool on_new_node(range_op_ctx& ctx, cell_node& node) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;

private:
  cell_index merged_with_ = 0;
};


class unmerge_cells_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  void on_start(range_op_ctx& ctx) override;
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;
};


class erase_empty_cells_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;
};


class actualize_cell_format_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;
};


class actualize_layout_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;
};


class invalidate_layout_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;
};


class invalidate_layout_width_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;
};


class parse_formulas_op final : public range_op {
public:
  using processing = for_existing_cells_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override;
};


} // namespace lde::cellfy::boox
