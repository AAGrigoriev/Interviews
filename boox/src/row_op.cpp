#include <lde/cellfy/boox/src/row_op.h>

#include <algorithm>

#include <ed/core/rect.h>

#include <ed/rasta/dpi.h>
#include <ed/rasta/matrix.h>
#include <ed/rasta/text_layout.h>

#include <lde/cellfy/boox/workbook.h>
#include <lde/cellfy/boox/worksheet.h>


namespace lde::cellfy::boox {


set_row_height_op::set_row_height_op(ed::twips<double> val) noexcept
  : val_(val) {
}


bool set_row_height_op::on_new_node(range_op_ctx& ctx, row_node& node) {
  node.height = val_;
  node.custom_height = true;
  return true;
}


bool set_row_height_op::on_existing_node(range_op_ctx& ctx, row_node::it node) {
  if (node->height != val_ || !node->custom_height) {
    row_node n = *node;
    n.height = val_;
    n.custom_height = true;
    ctx.forest().modify(node) = n;
  }
  return true;
}


bool actualize_row_height_op::on_existing_node(range_op_ctx& ctx, row_node::it node) {
  // TODO: Доработать для объединенных ячеек
  if (!node->custom_height) {
    ed::twips<double> h = ctx.sheet().default_row_height();

    for (auto& cell_n : ctx.forest().get<cell_node>(node)) {
      if (cell_n.layout) {
        ed::twips<double> cell_h = cell_n.layout->height();
        if (cell_n.format && cell_n.format->holds<text_rotation>()) {
          const auto rect = ed::rasta::matrix{}.
                            rotate(cell_n.format->get<text_rotation>()).
                            map(ed::rect_px<double>{ed::point_px<double>{}, ed::size_px<double>{cell_n.layout->width(), cell_n.layout->height()}});

          cell_h = std::max(cell_h, ed::twips<double>{rect.height});
        }
        h = std::max(h, cell_h);
      }
    }

    if (h != node->height) {
      row_node n = *node;
      n.height = h;
      ctx.forest().modify(node) = n;
    }
  }
  return true;
}


bool actualize_row_format_op::on_existing_node(range_op_ctx& ctx, row_node::it node) {
  if (node->format_key) {
    node->format = ctx.forest().find<cell_format_node>(*node->format_key)->format;
    ED_ASSERT(node->format);
  } else {
    node->format = nullptr;
  }
  return true;
}


get_row_format_op::get_row_format_op(cell_format& format) noexcept
  : format_(&format) {
}


void get_row_format_op::on_start(range_op_ctx& ctx) {
  ED_ASSERT(format_);
  *format_ = {};
  processed_count_ = 0;
}


bool get_row_format_op::on_existing_node(range_op_ctx& ctx, row_node::it node) {
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


bool clear_row_format_op::on_existing_node(range_op_ctx& ctx, row_node::it node) {
  if (node->format_key) {
    ED_ASSERT(node->format);
    row_node n = *node;
    n.format_key = std::nullopt;
    n.format = nullptr;
    ctx.forest().modify(node) = n;
  }
  return true;
}


change_existing_row_format_op::change_existing_row_format_op(cell_format::changes changes) noexcept
  : changes_(std::move(changes)) {
}


bool change_existing_row_format_op::on_existing_node(range_op_ctx& ctx, row_node::it node) {
  if (node->format_key) {
    ED_ASSERT(node->format);
    auto new_fmt = node->format->apply(changes_);
    if (new_fmt != *node->format) {
      auto format_node = ctx.sheet().book().ensure_format(std::move(new_fmt));
      ED_ENSURES(format_node->format);

      row_node n = *node;
      n.format_key = forest_t::key_of(format_node);
      n.format = format_node->format;
      ctx.forest().modify(node) = n;
    }
  } else {
    cell_format fmt = ctx.sheet().node()->format ? ctx.sheet().node()->format->apply(changes_) : cell_format().apply(changes_);
    auto format_node = ctx.sheet().book().ensure_format(std::move(fmt));
    ED_ENSURES(format_node->format);

    row_node n = *node;
    n.format_key = forest_t::key_of(format_node);
    n.format = format_node->format;
    ctx.forest().modify(node) = n;
  }
  return true;
}


change_row_format_op::change_row_format_op(cell_format::changes changes) noexcept
  : change_existing_row_format_op(std::move(changes)) {
}


bool change_row_format_op::on_new_node(range_op_ctx& ctx, row_node& node) {
  cell_format fmt = ctx.sheet().node()->format ? ctx.sheet().node()->format->apply(changes_) : cell_format().apply(changes_);
  auto format_node = ctx.sheet().book().ensure_format(std::move(fmt));
  ED_ENSURES(format_node->format);

  node.height = ctx.sheet().default_row_height();
  node.format_key = forest_t::key_of(format_node);
  node.format = format_node->format;
  return true;
}


bool set_up_default_row_height::on_existing_node(range_op_ctx& ctx, row_node::it node) {
  if (node->custom_height || node->collapsed) {
    row_node n = *node;
    n.custom_height = false;
    n.collapsed     = false;
    n.hidden        = false;
    ctx.forest().modify(node) = n;
  }
  return true;
}

} // namespace lde::cellfy::boox
