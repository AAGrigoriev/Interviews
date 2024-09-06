#pragma once


#include <optional>

#include <ed/core/quantity.h>

#include <lde/cellfy/boox/format.h>
#include <lde/cellfy/boox/range_op.h>


namespace lde::cellfy::boox {


class set_row_height_op final : public range_op {
public:
  using processing = for_all_rows_tag;

public:
  explicit set_row_height_op(ed::twips<double> val) noexcept;

  bool on_new_node(range_op_ctx& ctx, row_node& node) override;
  bool on_existing_node(range_op_ctx& ctx, row_node::it node) override;

private:
  ed::twips<double> val_;
};


class actualize_row_height_op final : public range_op {
public:
  using processing = for_existing_rows_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, row_node::it node) override;
};


class actualize_row_format_op final : public range_op {
public:
  using processing = for_existing_rows_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, row_node::it node) override;
};


class get_row_format_op final : public range_op {
public:
  using processing = for_existing_rows_tag;

public:
  explicit get_row_format_op(cell_format& format) noexcept;

  void on_start(range_op_ctx& ctx) override;
  bool on_existing_node(range_op_ctx& ctx, row_node::it node) override;

private:
  cell_format* format_          = nullptr;
  std::size_t  processed_count_ = 0;
};


class clear_row_format_op final : public range_op {
public:
  using processing = for_existing_rows_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, row_node::it node) override;
};


class change_existing_row_format_op : public range_op {
public:
  using processing = for_existing_rows_tag;

public:
  explicit change_existing_row_format_op(cell_format::changes changes) noexcept;

  bool on_existing_node(range_op_ctx& ctx, row_node::it node) override;

protected:
  cell_format::changes changes_;
};


class change_row_format_op final : public change_existing_row_format_op {
public:
  using processing = for_all_rows_tag;

public:
  explicit change_row_format_op(cell_format::changes changes) noexcept;

  bool on_new_node(range_op_ctx& ctx, row_node& node) override;
};


class set_up_default_row_height final : public range_op {
public:
  using processing = for_existing_rows_tag;

public:
  bool on_existing_node(range_op_ctx& ctx, row_node::it node) override;
};

} // namespace lde::cellfy::boox
