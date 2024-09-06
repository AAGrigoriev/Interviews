#include <lde/cellfy/boox/range_op.h>

#include <algorithm>

#include <lde/cellfy/boox/workbook.h>
#include <lde/cellfy/boox/worksheet.h>


namespace lde::cellfy::boox {


range_op_ctx::range_op_ctx(worksheet& sheet, const area::list& areas) noexcept
  : sheet_(sheet)
  , areas_(areas) {
}


const worksheet& range_op_ctx::sheet() const noexcept {
  return sheet_;
}


worksheet& range_op_ctx::sheet() noexcept {
  return sheet_;
}


const forest_t& range_op_ctx::forest() const noexcept {
  return sheet_.book().forest();
}


forest_t& range_op_ctx::forest() noexcept {
  return sheet_.book().forest();
}


const area::list& range_op_ctx::areas() const noexcept {
  return areas_;
}


void range_op::for_all_columns(range_op_ctx& ctx, range_op& op) {
  op.on_start(ctx);
  auto columns = ctx.forest().get<column_node>(ctx.sheet().node());

  for (auto&& ar : ctx.areas()) {
    auto col_i = std::lower_bound(columns.begin(), columns.end(), column_node{ar.left_column()});

    for (auto index = ar.left_column(); index <= ar.right_column(); ++index) {
      if (col_i != columns.end() && col_i->index == index) {
        if (!op.on_existing_node(ctx, col_i++)) {
          op.on_break(ctx);
          return;
        }
      } else {
        column_node n;
        n.index = index;

        if (!op.on_new_node(ctx, n)) {
          op.on_break(ctx);
          return;
        }

        col_i = ctx.forest().insert(ctx.sheet().node(), col_i, std::move(n));

        if (!op.on_new_node(ctx, col_i++)) {
          op.on_break(ctx);
          return;
        }
      }
    }
  }

  op.on_finish(ctx);
}


void range_op::for_existing_columns(range_op_ctx& ctx, range_op& op) {
  op.on_start(ctx);
  auto columns = ctx.forest().get<column_node>(ctx.sheet().node());

  for (auto&& ar : ctx.areas()) {
    auto col_i = std::lower_bound(columns.begin(), columns.end(), column_node{ar.left_column()});

    while (col_i != columns.end() && col_i->index <= ar.right_column()) {
      if (!op.on_existing_node(ctx, col_i++)) {
        op.on_break(ctx);
        return;
      }
    }
  }

  op.on_finish(ctx);
}


void range_op::for_all_rows(range_op_ctx& ctx, range_op& op) {
  op.on_start(ctx);
  auto rows = ctx.forest().get<row_node>(ctx.sheet().node());

  for (auto&& ar : ctx.areas()) {
    auto row_i = std::lower_bound(rows.begin(), rows.end(), row_node{ar.top_row()});

    for (auto index = ar.top_row(); index <= ar.bottom_row(); ++index) {
      if (row_i != rows.end() && row_i->index == index) {
        if (!op.on_existing_node(ctx, row_i++)) {
          op.on_break(ctx);
          return;
        }
      } else {
        row_node n;
        n.index = index;

        if (!op.on_new_node(ctx, n)) {
          op.on_break(ctx);
          return;
        }

        row_i = ctx.forest().insert(ctx.sheet().node(), row_i, std::move(n));

        if (!op.on_new_node(ctx, row_i++)) {
          op.on_break(ctx);
          return;
        }
      }
    }
  }

  op.on_finish(ctx);
}


void range_op::for_existing_rows(range_op_ctx& ctx, range_op& op) {
  op.on_start(ctx);
  auto rows = ctx.forest().get<row_node>(ctx.sheet().node());

  for (auto&& ar : ctx.areas()) {
    auto row_i = std::lower_bound(rows.begin(), rows.end(), row_node{ar.top_row()});

    while (row_i != rows.end() && row_i->index <= ar.bottom_row()) {
      if (!op.on_existing_node(ctx, row_i++)) {
        op.on_break(ctx);
        return;
      }
    }
  }

  op.on_finish(ctx);
}


void range_op::for_all_cells(range_op_ctx& ctx, range_op& op) {
  op.on_start(ctx);
  auto rows = ctx.forest().get<row_node>(ctx.sheet().node());

  for (auto&& ar : ctx.areas()) {
    auto row_i = std::lower_bound(rows.begin(), rows.end(), row_node{ar.top_row()});

    for (auto row = ar.top_row(); row <= ar.bottom_row(); ++row) {
      if (row_i != rows.end() && row_i->index == row) {
        auto cells = ctx.forest().get<cell_node>(row_i);
        auto cell_i = std::lower_bound(cells.begin(), cells.end(), cell_node{cell_addr(ar.left_column(), row).index()});

        for (auto col = ar.left_column(); col <= ar.right_column(); ++col) {
          cell_addr addr(col, row);
          if (cell_i != cells.end() && cell_i->index == addr.index()) {
            if (!op.on_existing_node(ctx, cell_i++)) {
              op.on_break(ctx);
              return;
            }
          } else {
            cell_node cell_n;
            cell_n.index = addr.index();

            if (!op.on_new_node(ctx, cell_n)) {
              op.on_break(ctx);
              return;
            }

            cell_i = ctx.forest().insert(row_i, cell_i, std::move(cell_n));

            if (!op.on_new_node(ctx, cell_i++)) {
              op.on_break(ctx);
              return;
            }
          }
        }
      } else {
        row_node row_n;
        row_n.index = row;
        row_n.height = ctx.sheet().default_row_height();

        if (!op.on_new_node(ctx, row_n)) {
          op.on_break(ctx);
          return;
        }

        row_i = ctx.forest().insert(ctx.sheet().node(), row_i, std::move(row_n));
        // После вставки новой строки, нужно обновить range строк.
        rows = ctx.forest().get<row_node>(ctx.sheet().node());

        if (!op.on_new_node(ctx, row_i)) {
          op.on_break(ctx);
          return;
        }

        for (auto col = ar.left_column(); col <= ar.right_column(); ++col) {
          cell_node cell_n;
          cell_n.index = cell_addr(col, row).index();

          if (!op.on_new_node(ctx, cell_n)) {
            op.on_break(ctx);
            return;
          }

          auto cell_i = ctx.forest().push_back(row_i, std::move(cell_n));

          if (!op.on_new_node(ctx, cell_i)) {
            op.on_break(ctx);
            return;
          }
        }
      }

      ++row_i;
    }
  }

  op.on_finish(ctx);
}


void range_op::for_existing_cells(range_op_ctx& ctx, range_op& op) {
  op.on_start(ctx);
  auto rows = ctx.forest().get<row_node>(ctx.sheet().node());

  for (auto&& ar : ctx.areas()) {
    auto row_i = std::lower_bound(rows.begin(), rows.end(), row_node{ar.top_row()});

    while (row_i != rows.end() && row_i->index <= ar.bottom_row()) {
      auto cells = ctx.forest().get<cell_node>(row_i);
      auto cell_i = std::lower_bound(cells.begin(), cells.end(), cell_node{cell_addr(ar.left_column(), row_i->index).index()});

      while (cell_i != cells.end() && cell_i->index <= cell_addr(ar.right_column(), row_i->index).index()) {
        if (!op.on_existing_node(ctx, cell_i++)) {
          op.on_break(ctx);
          return;
        }
      }

      ++row_i;
    }
  }

  op.on_finish(ctx);
}


void range_op::on_start(range_op_ctx& ctx) {

}


void range_op::on_finish(range_op_ctx& ctx) {

}


void range_op::on_break(range_op_ctx& ctx) {

}


bool range_op::on_new_node(range_op_ctx& ctx, column_node& node) {
  return true;
}


bool range_op::on_new_node(range_op_ctx& ctx, column_node::it node) {
  return true;
}


bool range_op::on_existing_node(range_op_ctx& ctx, column_node::it node) {
  return true;
}


bool range_op::on_new_node(range_op_ctx& ctx, row_node& node) {
  return true;
}


bool range_op::on_new_node(range_op_ctx& ctx, row_node::it node) {
  return true;
}


bool range_op::on_existing_node(range_op_ctx& ctx, row_node::it node) {
  return true;
}


bool range_op::on_new_node(range_op_ctx& ctx, cell_node& node) {
  return true;
}


bool range_op::on_new_node(range_op_ctx& ctx, cell_node::it node) {
  return true;
}


bool range_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  return true;
}


} // namespace lde::cellfy::boox
