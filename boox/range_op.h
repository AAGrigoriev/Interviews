#pragma once


#include <memory>
#include <type_traits>
#include <vector>

#include <lde/cellfy/boox/area.h>
#include <lde/cellfy/boox/forest.h>
#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/node.h>


namespace lde::cellfy::boox {


class range_op_ctx final {
public:
  range_op_ctx(worksheet& sheet, const area::list& areas) noexcept;

  range_op_ctx(const range_op_ctx&) = delete;
  range_op_ctx& operator=(const range_op_ctx&) = delete;

  const worksheet& sheet() const noexcept;
  worksheet& sheet() noexcept;

  const forest_t& forest() const noexcept;
  forest_t& forest() noexcept;

  const area::list& areas() const noexcept;

private:
  worksheet& sheet_;
  area::list areas_;
};


struct for_all_columns_tag {};
struct for_existing_columns_tag {};
struct for_all_rows_tag {};
struct for_existing_rows_tag {};
struct for_all_cells_tag {};
struct for_existing_cells_tag {};


class range_op {
public:
  static void for_all_columns(range_op_ctx& ctx, range_op& op);
  static void for_existing_columns(range_op_ctx& ctx, range_op& op);
  static void for_all_rows(range_op_ctx& ctx, range_op& op);
  static void for_existing_rows(range_op_ctx& ctx, range_op& op);
  static void for_all_cells(range_op_ctx& ctx, range_op& op);
  static void for_existing_cells(range_op_ctx& ctx, range_op& op);

public:
  virtual ~range_op() = default;

  virtual void on_start(range_op_ctx& ctx);
  virtual void on_finish(range_op_ctx& ctx);
  virtual void on_break(range_op_ctx& ctx);

  virtual bool on_new_node(range_op_ctx& ctx, column_node& node);
  virtual bool on_new_node(range_op_ctx& ctx, column_node::it node);
  virtual bool on_existing_node(range_op_ctx& ctx, column_node::it node);

  virtual bool on_new_node(range_op_ctx& ctx, row_node& node);
  virtual bool on_new_node(range_op_ctx& ctx, row_node::it node);
  virtual bool on_existing_node(range_op_ctx& ctx, row_node::it node);

  virtual bool on_new_node(range_op_ctx& ctx, cell_node& node);
  virtual bool on_new_node(range_op_ctx& ctx, cell_node::it node);
  virtual bool on_existing_node(range_op_ctx& ctx, cell_node::it node);

protected:
  range_op() = default;
};


template<typename Op1, typename Op2>
class composite_range_op : public range_op {
  static_assert(std::is_base_of_v<range_op, Op1>);
  static_assert(std::is_base_of_v<range_op, Op2>);

public:
  template<typename Processing1, typename Processing2>
  struct choose_processing;

  template<typename Processing>
  struct choose_processing<Processing, Processing> {
    using type = Processing;
  };

  template<>
  struct choose_processing<for_all_columns_tag, for_existing_columns_tag> {
    using type = for_all_columns_tag;
  };

  template<>
  struct choose_processing<for_existing_columns_tag, for_all_columns_tag> {
    using type = for_all_columns_tag;
  };

  template<>
  struct choose_processing<for_all_rows_tag, for_existing_rows_tag> {
    using type = for_all_rows_tag;
  };

  template<>
  struct choose_processing<for_existing_rows_tag, for_all_rows_tag> {
    using type = for_all_rows_tag;
  };

  template<>
  struct choose_processing<for_all_cells_tag, for_existing_cells_tag> {
    using type = for_all_cells_tag;
  };

  template<>
  struct choose_processing<for_existing_cells_tag, for_all_cells_tag> {
    using type = for_all_cells_tag;
  };

  using processing = typename choose_processing<
    typename Op1::processing,
    typename Op2::processing
  >::type;

public:
  composite_range_op(Op1&& op1, Op2&& op2)
    : op1_(std::move(op1))
    , op2_(std::move(op2)) {
  }

  void on_start(range_op_ctx& ctx) override {
    op1_.on_start(ctx);
    op2_.on_start(ctx);
  }

  void on_finish(range_op_ctx& ctx) override {
    op1_.on_finish(ctx);
    op2_.on_finish(ctx);
  }

  bool on_new_node(range_op_ctx& ctx, column_node& node) override {
    return on_new_node_impl(ctx, node);
  }

  bool on_new_node(range_op_ctx& ctx, column_node::it node) override {
    return on_new_node_impl(ctx, node);
  }

  bool on_existing_node(range_op_ctx& ctx, column_node::it node) override {
    return on_existing_node_impl(ctx, node);
  }

  bool on_new_node(range_op_ctx& ctx, row_node& node) override {
    return on_new_node_impl(ctx, node);
  }

  bool on_new_node(range_op_ctx& ctx, row_node::it node) override {
    return on_new_node_impl(ctx, node);
  }

  bool on_existing_node(range_op_ctx& ctx, row_node::it node) override {
    return on_existing_node_impl(ctx, node);
  }

  bool on_new_node(range_op_ctx& ctx, cell_node& node) override {
    return on_new_node_impl(ctx, node);
  }

  bool on_new_node(range_op_ctx& ctx, cell_node::it node) override {
    return on_new_node_impl(ctx, node);
  }

  bool on_existing_node(range_op_ctx& ctx, cell_node::it node) override {
    return on_existing_node_impl(ctx, node);
  }

private:
  template<typename Node>
  bool on_new_node_impl(range_op_ctx& ctx, Node&& node) {
    if (!static_cast<range_op&>(op1_).on_new_node(ctx, std::forward<Node>(node))) {
      return false;
    }
    if (!static_cast<range_op&>(op2_).on_new_node(ctx, std::forward<Node>(node))) {
      return false;
    }
    return true;
  }

  template<typename Node>
  bool on_existing_node_impl(range_op_ctx& ctx, Node&& node) {
    if (!static_cast<range_op&>(op1_).on_existing_node(ctx, std::forward<Node>(node))) {
      return false;
    }
    if (!static_cast<range_op&>(op2_).on_existing_node(ctx, std::forward<Node>(node))) {
      return false;
    }
    return true;
  }

private:
  Op1 op1_;
  Op2 op2_;
};


template<typename Op1, typename Op2>
std::enable_if_t<
  std::is_base_of_v<range_op, Op1> && std::is_base_of_v<range_op, Op2>,
  composite_range_op<Op1, Op2>
>
operator|(Op1&& op1, Op2&& op2) {
  return {std::move(op1), std::move(op2)};
}


} // namespace lde::cellfy::boox
