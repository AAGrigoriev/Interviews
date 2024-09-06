#pragma once


#include <ed/core/quantity.h>

#include <lde/cellfy/boox/format.h>
#include <lde/cellfy/boox/range_op.h>


namespace lde::cellfy::boox {


class set_column_width_op final : public range_op {
public:
  using processing = for_all_columns_tag;

public:
  explicit set_column_width_op(ed::twips<double> val) noexcept;

  bool on_new_node(range_op_ctx& ctx, column_node& node) override;
  bool on_existing_node(range_op_ctx& ctx, column_node::it node) override;

private:
  ed::twips<double> val_;
};


class actualize_column_format_op final : public range_op {
public:
  using processing = for_existing_columns_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, column_node::it node) override;
};


class get_column_format_op final : public range_op {
public:
  using processing = for_existing_columns_tag;

public:
  explicit get_column_format_op(cell_format& format) noexcept;

  void on_start(range_op_ctx& ctx) override;
  bool on_existing_node(range_op_ctx& ctx, column_node::it node) override;

private:
  cell_format* format_          = nullptr;
  std::size_t  processed_count_ = 0;
};


class clear_column_format_op final : public range_op {
public:
  using processing = for_existing_columns_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, column_node::it node) override;
};


class change_existing_column_format_op : public range_op {
public:
  using processing = for_existing_columns_tag;

public:
  explicit change_existing_column_format_op(cell_format::changes changes) noexcept;

  bool on_existing_node(range_op_ctx& ctx, column_node::it node) override;

protected:
  cell_format::changes changes_;
};


class change_column_format_op final : public change_existing_column_format_op {
public:
  using processing = for_all_columns_tag;

public:
  explicit change_column_format_op(cell_format::changes changes) noexcept;

  bool on_new_node(range_op_ctx& ctx, column_node& node) override;
};


class change_column_width final : public range_op {
public:
  using processing = for_all_columns_tag;

public:
  bool on_new_node(range_op_ctx& ctx, column_node& node) override;
  bool on_existing_node(range_op_ctx& ctx, column_node::it node) override;

private:
  ed::twips<double> get_max_cells_width(range_op_ctx& ctx, const column_node& node);
};

} // namespace lde::cellfy::boox
