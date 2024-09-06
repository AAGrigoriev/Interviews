#include <lde/cellfy/boox/worksheet.h>

#include <algorithm>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include <ed/core/assert.h>

#include <ed/rasta/dpi.h>
#include <ed/rasta/text_layout.h>

#include <lde/cellfy/boox/cell_addr.h>
#include <lde/cellfy/boox/exception.h>
#include <lde/cellfy/boox/format.h>
#include <lde/cellfy/boox/fx_parser.h>
#include <lde/cellfy/boox/scoped_transaction.h>
#include <lde/cellfy/boox/workbook.h>
#include <lde/cellfy/boox/src/cell_op.h>
#include <lde/cellfy/boox/src/column_op.h>
#include <lde/cellfy/boox/src/row_op.h>


namespace lde::cellfy::boox {


worksheet::worksheet(workbook& book, worksheet_node::it sheet_node)
  : book_(book)
  , sheet_node_(sheet_node)
  , cells_(*this) {

  sheet_node->sheet = this;

  name_ = sheet_node->name;
  selection_ = cells_.cell({0, 0});
  active_cell_ = cells_.cell({0, 0});

  name.attach(name_);
  active.attach(active_);
  selection.attach(selection_);
  active_cell.attach(active_cell_);

  ed::rasta::text_layout layout;
  layout.add(L"0000000000", make_text_format(font_format()));
  default_column_width_ = layout.width() + 2_px;
  default_row_height_ = layout.height() + 2_px;

  actualize_format();
  cells_.apply(actualize_column_format_op());
  cells_.apply(actualize_row_format_op());
  cells_.apply(parse_formulas_op()); // Вынес отдельно, чтобы при actualize_layout_op формула уже была готова.
  cells_.apply(actualize_cell_format_op() | actualize_layout_op());

  changed += std::ref(book_.changed);
}


worksheet_node::it worksheet::node() const noexcept {
  return sheet_node_;
}


const workbook& worksheet::book() const noexcept {
  return book_;
}


workbook& worksheet::book() noexcept {
  return book_;
}


std::size_t worksheet::index() const noexcept {
  return std::distance(
    book_.sheets_.begin(),
    book_.sheets_.iterator_to(*this));
}


range worksheet::cells() const noexcept {
  return cells_;
}


range worksheet::cells(cell_addr first, cell_addr last) const {
  return cells_.cells(first, last);
}


range worksheet::cells(std::wstring_view a1) const {
  return cells_.cells(a1);
}


range worksheet::cell(cell_addr addr) const {
  return cells_.cell(addr);
}


ed::twips<double> worksheet::default_column_width() const noexcept {
  return default_column_width_;
}


ed::twips<double> worksheet::default_row_height() const noexcept {
  return default_row_height_;
}


ed::twips<double> worksheet::columns_width(column_index from, column_index to) const noexcept {
  ED_ASSERT(from <= to);

  auto columns = book().forest().get<column_node>(sheet_node_);
  auto i = std::lower_bound(columns.begin(), columns.end(), column_node{from});

  std::size_t count = 0;
  ed::pixels<long long> width = 0_px;

  while (i != columns.end()) {
    if (i->index > to) {
      break;
    }
    ++count;
    width += i->width;
    ++i;
  }

  return width + ed::pixels<long long>(default_column_width()) * (to - from - count + 1);
}


ed::twips<double> worksheet::rows_height(row_index from, row_index to) const noexcept {
  ED_ASSERT(from <= to);

  auto rows = book().forest().get<row_node>(sheet_node_);
  auto i = std::lower_bound(rows.begin(), rows.end(), row_node{from});

  std::size_t count = 0;
  ed::pixels<long long> height = 0_px;

  while (i != rows.end()) {
    if (i->index > to) {
      break;
    }
    ++count;
    height += i->height;
    ++i;
  }

  return height + ed::pixels<long long>(default_row_height()) * (to - from - count + 1);
}


void worksheet::activate() {
  auto active = *book_.active_sheet;

  if (active == this) {
    return;
  }

  if (active) {
    active->active_ = false;
  }

  active_ = true;
  book_.active_sheet_ = this;
  book_.active_selection.attach(selection);
  book_.active_selection.emit_changed();
  book_.active_cell.attach(active_cell);
  book_.active_cell.emit_changed();
}


bool worksheet::rename(std::wstring_view name) {
  if (name.empty()) {
    return false;
  }

  if (*name_ == name) {
    return true;
  }

  if (book_.sheet_by_name(name)) {
    return false;
  }

  auto n = *sheet_node_;
  n.name = name;

  scoped_transaction tr(book().forest());
  {
    book_.forest().modify(sheet_node_) = std::move(n);
  }
  tr.commit();
  return true;
}


bool worksheet::remove() {
  if (*book_.sheets_count > 1) {
    scoped_transaction tr(book().forest());
    {
      book().forest().erase(sheet_node_);
    }
    tr.commit();
    return true;
  }
  return false;
}


void worksheet::paste(const ed::mime_type& format, const ed::buffer& data) {
  auto cbr_it = book_.clipboard_readers_.find(format);
  if (cbr_it == book_.clipboard_readers_.end()) {
    ED_THROW_EXCEPTION(unsupported_clipboard_format());
  }

  scoped_transaction tr(book().forest());
  {
    boost::iostreams::stream<boost::iostreams::array_source> is(data.data(), data.size());
    range dst = *active_cell_;
    cbr_it->second(is, dst);
  }
  tr.commit();
}


void worksheet::update_formulas() {
  cells_.apply(parse_formulas_op());
  cells_.apply(actualize_layout_op());
}


void worksheet::update_formulas_and_view() {
  update_formulas();
  changed(cells_);
}


column_node::opt_it worksheet::find_column(column_index index) const noexcept {
  auto columns = book().forest().get<column_node>(sheet_node_);
  auto col_i = std::lower_bound(columns.begin(), columns.end(), column_node{index});
  if (col_i != columns.end() && col_i->index == index) {
    return col_i;
  }
  return std::nullopt;
}


row_node::opt_it worksheet::find_row(row_index index) const noexcept {
  auto rows = book().forest().get<row_node>(sheet_node_);
  auto row_i = std::lower_bound(rows.begin(), rows.end(), row_node{index});
  if (row_i != rows.end() && row_i->index == index) {
    return row_i;
  }
  return std::nullopt;
}


cell_node::opt_it worksheet::find_cell(cell_addr addr) const noexcept {
  if (auto row_i = find_row(addr.row())) {
    auto cells = book().forest().get<cell_node>(row_i.value());
    auto cell_i = std::lower_bound(cells.begin(), cells.end(), cell_node{addr.index()});
    if (cell_i != cells.end() && cell_i->index == addr.index()) {
      return cell_i;
    }
  }
  return std::nullopt;
}


void worksheet::changes_started() {

}


void worksheet::changes_finished(const calc_mode mode) {
  if (mode == calc_mode::automatic) {
    for (auto& node : volatile_cells_) {
      auto children = book().forest().get<cell_formula_node>(node);
      ED_EXPECTS(children.size() == 1);
      children.front().is_result_dirty = true;
    }

    for (auto& node : volatile_cells_) {
      if (!changes_.contains(node->index)) {
        cell(node->index).apply(actualize_layout_op());
      }
    }
  }

  if (!changes_.empty()) {
    actualize_format();
    changes_.apply(actualize_column_format_op());
    changes_.apply(actualize_row_format_op());
    changes_.apply(invalidate_layout_op() | actualize_cell_format_op() | actualize_layout_op() | erase_empty_cells_op());
    changes_.apply(actualize_row_height_op());
    changed(changes_);
    changes_ = range();
  }
}


void worksheet::modified(worksheet_node::it node) {
  ED_ASSERT(node->sheet == this);
  if (node->name.size() != name_->size() || !std::equal(node->name.begin(), node->name.end(), name_->begin())) {
    auto modifier = [node](worksheet& sheet) {
      sheet.name_ = node->name;
    };
    book_.sheets_.get<workbook::by_name>().modify(
      book_.sheets_.get<workbook::by_name>().iterator_to(*this),
      modifier);
  }
  changes_ = cells_;
  cells_.apply(invalidate_layout_op());
}


void worksheet::inserted(column_node::it node) {
  changes_ = changes_.join(cells_.entire_column(node->index));
  cells_.entire_column(node->index).apply(invalidate_layout_width_op());
}


void worksheet::erased(column_node::it node) {
  changes_ = changes_.join(cells_.entire_column(node->index));
  cells_.entire_column(node->index).apply(invalidate_layout_width_op());
}


void worksheet::modified(column_node::it node) {
  changes_ = changes_.join(cells_.entire_column(node->index));
  cells_.entire_column(node->index).apply(invalidate_layout_width_op());
}


void worksheet::inserted(row_node::it node) {
  changes_ = changes_.join(cells_.entire_row(node->index));
}


void worksheet::erased(row_node::it node) {
  changes_ = changes_.join(cells_.entire_row(node->index));
}


void worksheet::modified(row_node::it node) {
  changes_ = changes_.join(cells_.entire_row(node->index));
}


void worksheet::inserted(cell_node::it node) {
  changes_ = changes_.join(cell(node->index));
  node->is_layout_dirty = true;
}


void worksheet::erased(cell_node::it node) {
  changes_ = changes_.join(cell(node->index));
  volatile_cells_.erase(node);
}


void worksheet::modified(cell_node::it node) {
  changes_ = changes_.join(cell(node->index));
  node->is_layout_dirty = true;
}


void worksheet::inserted(cell_data_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  changes_ = changes_.join(cell(parent->index));
  parent->is_layout_dirty = true;
}


void worksheet::erased(cell_data_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  changes_ = changes_.join(cell(parent->index));
  parent->is_layout_dirty = true;
}


void worksheet::modified(cell_data_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  changes_ = changes_.join(cell(parent->index));
  parent->is_layout_dirty = true;
}


void worksheet::inserted(text_run_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  changes_ = changes_.join(cell(parent->index));
  parent->is_layout_dirty = true;
}


void worksheet::erased(text_run_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  changes_ = changes_.join(cell(parent->index));
  parent->is_layout_dirty = true;
}


void worksheet::modified(text_run_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  changes_ = changes_.join(cell(parent->index));
  parent->is_layout_dirty = true;
}


void worksheet::inserted(cell_formula_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  changes_ = changes_.join(cell(parent->index));
  parent->is_layout_dirty = true;
  parse_formula(node);
}


void worksheet::erased(cell_formula_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  changes_ = changes_.join(cell(parent->index));
  parent->is_layout_dirty = true;
  volatile_cells_.erase(parent);
}


void worksheet::modified(cell_formula_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  changes_ = changes_.join(cell(parent->index));
  parent->is_layout_dirty = true;
  parse_formula(node);
}


void worksheet::parse_formula(cell_node::it node) {
  ED_ASSERT(node->has_formula);
  auto children = book().forest().get<cell_formula_node>(node);
  ED_EXPECTS(children.size() == 1);
  parse_formula(children.begin());
}


void worksheet::parse_formula(cell_formula_node::it node) {
  auto parent = book().forest().ancestor<cell_node>(node);
  bool is_volatile = false;

  try {
    book().formula_parser().parse(node->formula, node->ast);

    // TODO: Доработать определение is_volatile и зависимостей
    node->is_volatile = true;
    node->is_result_dirty = true;

    if (node->is_volatile) {
      volatile_cells_.insert(parent);
      is_volatile = true;
    } else {
      // TODO: Определение зависемостей и подписка на их изменение
    }
  } catch (const std::exception&) {
    node->is_volatile = false;
    node->is_result_dirty = false;
    node->result = cell_value_error::na; // TODO: Это не точно.
  }

  node->is_parsed = true;

  if (!is_volatile) {
    volatile_cells_.erase(parent);
  }
}


void worksheet::actualize_format() {
  if (sheet_node_->format_key) {
    sheet_node_->format = book_.forest_.find<cell_format_node>(*sheet_node_->format_key)->format;
    ED_ASSERT(sheet_node_->format);
  } else {
    sheet_node_->format = nullptr;
  }
}


} // namespace lde::cellfy::boox
