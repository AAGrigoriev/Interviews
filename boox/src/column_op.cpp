#include <lde/cellfy/boox/src/column_op.h>

#include <algorithm>

#include <ed/rasta/dpi.h>

#include <lde/cellfy/boox/workbook.h>
#include <lde/cellfy/boox/worksheet.h>


namespace lde::cellfy::boox {


set_column_width_op::set_column_width_op(ed::twips<double> val) noexcept
  : val_(val) {
}


bool set_column_width_op::on_new_node(range_op_ctx& ctx, column_node& node) {
  node.width = val_;
  node.custom_width = true;
  return true;
}


bool set_column_width_op::on_existing_node(range_op_ctx& ctx, column_node::it node) {
  if (node->width != val_ || !node->custom_width) {
    column_node n = *node;
    n.width = val_;
    n.custom_width = true;
    ctx.forest().modify(node) = n;
  }
  return true;
}



bool actualize_column_format_op::on_existing_node(range_op_ctx& ctx, column_node::it node) {
  if (node->format_key) {
    node->format = ctx.forest().find<cell_format_node>(*node->format_key)->format;
    ED_ASSERT(node->format);
  } else {
    node->format = nullptr;
  }
  return true;
}


get_column_format_op::get_column_format_op(cell_format& format) noexcept
  : format_(&format) {
}


void get_column_format_op::on_start(range_op_ctx& ctx) {
  ED_ASSERT(format_);
  *format_ = {};
  processed_count_ = 0;
}


bool get_column_format_op::on_existing_node(range_op_ctx& ctx, column_node::it node) {
  ED_ASSERT(format_);
  if (node->format_key) {
    ED_ASSERT(node->format);
    if (processed_count_ == 0) {
      *format_ = *node->format;
    } else {
      *format_ = format_->intersect(*node->format);
    }
    ++processed_count_;
  }
  return true;
}


bool clear_column_format_op::on_existing_node(range_op_ctx& ctx, column_node::it node) {
  if (node->format_key) {
    ED_ASSERT(node->format);
    column_node n = *node;
    n.format_key = std::nullopt;
    n.format = nullptr;
    ctx.forest().modify(node) = n;
  }
  return true;
}


change_existing_column_format_op::change_existing_column_format_op(cell_format::changes changes) noexcept
  : changes_(std::move(changes)) {
}


bool change_existing_column_format_op::on_existing_node(range_op_ctx& ctx, column_node::it node) {
  if (node->format_key) {
    ED_ASSERT(node->format);
    auto new_fmt = node->format->apply(changes_);
    if (new_fmt != *node->format) {
      auto format_node = ctx.sheet().book().ensure_format(std::move(new_fmt));
      ED_ENSURES(format_node->format);

      column_node n = *node;
      n.format_key = forest_t::key_of(format_node);
      n.format = format_node->format;
      ctx.forest().modify(node) = n;
    }
  } else {
    cell_format fmt = ctx.sheet().node()->format ? ctx.sheet().node()->format->apply(changes_) : cell_format().apply(changes_);
    auto format_node = ctx.sheet().book().ensure_format(std::move(fmt));
    ED_ENSURES(format_node->format);

    column_node n = *node;
    n.format_key = forest_t::key_of(format_node);
    n.format = format_node->format;
    ctx.forest().modify(node) = n;
  }
  return true;
}


change_column_format_op::change_column_format_op(cell_format::changes changes) noexcept
  : change_existing_column_format_op(std::move(changes)) {
}


bool change_column_format_op::on_new_node(range_op_ctx& ctx, column_node& node) {
  cell_format fmt = ctx.sheet().node()->format ? ctx.sheet().node()->format->apply(changes_) : cell_format().apply(changes_);
  auto format_node = ctx.sheet().book().ensure_format(std::move(fmt));
  ED_ENSURES(format_node->format);

  node.width = ctx.sheet().default_column_width();
  node.format_key = forest_t::key_of(format_node);
  node.format = format_node->format;
  return true;
}


bool change_column_width::on_new_node(range_op_ctx& ctx, column_node& node) {
  node.width = ctx.sheet().default_column_width();

  if (const auto max_width = get_max_cells_width(ctx, node); max_width > node.width) {
    node.width = max_width;
  }

  return true;
}


bool change_column_width::on_existing_node(range_op_ctx& ctx, column_node::it node) {
  if (const auto max_width = get_max_cells_width(ctx, *node); max_width != node->width) {
    auto column_node = *node;
    column_node.width = max_width;
    ctx.forest().modify(node) = column_node;
  }

  return true;
}


ed::twips<double> change_column_width::get_max_cells_width(range_op_ctx& ctx, const column_node& node) {
  ed::twips<double> max_width;

  // Ячейки берутся из строки, а не из столбца.
  auto rows = ctx.forest().get<row_node>(ctx.sheet().node());

  for (auto row = rows.begin(); row != rows.end(); ++row) {
    auto cells = ctx.forest().get<cell_node>(row);
    const auto cell_index = cell_addr{node.index, row->index}.index();
    if (auto cell = std::lower_bound(cells.begin(), cells.end(), cell_node{cell_index});
        cell != cells.end()       &&
        cell->index == cell_index &&
        cell->layout              &&
        cell->column_span == 1) { // Обьединённые ячейки не участвуют в расширении по ширине. Только обычные ячейки.

      max_width = std::max(max_width, ed::twips<double>{cell->layout->width()});
    }
  }

  return max_width;
}

} // namespace lde::cellfy::boox
