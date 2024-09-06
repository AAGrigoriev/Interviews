#include <lde/cellfy/boox/workbook.h>

#include <iterator>

#include <boost/range/adaptor/map.hpp>

#include <ed/core/assert.h>
#include <ed/core/scoped_assign.h>

#include <lde/cellfy/boox/exception.h>
#include <lde/cellfy/boox/scoped_transaction.h>


namespace lde::cellfy::boox {


workbook::workbook() {
  can_undo.attach(can_undo_);
  can_redo.attach(can_redo_);
  sheets_count.attach(sheets_count_);
  active_sheet.attach(active_sheet_);

  auto& book_meta = forest_.allow<workbook_node>(1, 1);
  auto& cell_format_meta = book_meta.allow<cell_format_node>(0, forest::infinite);
  auto& sheet_meta = book_meta.allow<worksheet_node>(1, forest::infinite);
  auto& column_meta = sheet_meta.allow<column_node>(0, forest::infinite);
  auto& row_meta = sheet_meta.allow<row_node>(0, forest::infinite);
  auto& cell_meta = row_meta.allow<cell_node>(0, forest::infinite);
  auto& cell_data_meta = cell_meta.allow<cell_data_node>(0, 1);
  auto& cell_formula_meta = cell_meta.allow<cell_formula_node>(0, 1);
  auto& text_run_meta = cell_meta.allow<text_run_node>(0, forest::infinite);

  forest_conns_.emplace_back(forest_.changes_started += [this](forest::changes_cause cause) {
    for (auto&& sheet : sheets_) {
      const_cast<worksheet&>(sheet).changes_started();
    }

    changes_started(cause);
  });

  forest_conns_.emplace_back(forest_.changes_finished += [this](forest::changes_cause cause) {
    // Удаление не используемых форматов
    if (cause == forest::changes_cause::commit) {
      ED_ASSERT(forest_.in_transaction());
      ed::scoped_assign _(formats_gc_at_work_, true);

      for (auto i = cell_formats_.begin(); i != cell_formats_.end();) {
        if ((*i)->format.use_count() == 1) {
          forest_.erase((*i));
          i = cell_formats_.erase(i);
        } else {
          ++i;
        }
      }
    }

    for (auto&& sheet : sheets_) {
      const_cast<worksheet&>(sheet).changes_finished(calc_mode_); // Если включен режим manual, то формулы пересчитываются только, если пользователь
                                                                  // изменил саму формулу. Т.е формула была заново разобрана.
    }

    can_undo_ = forest_.can_undo();
    can_redo_ = forest_.can_redo();
    changes_finished(cause);
  });

  forest_conns_.emplace_back(cell_format_meta.inserted += [this](typename cell_format_node::it node) {
    cell_formats_.insert(node);
  });

  forest_conns_.emplace_back(cell_format_meta.erased += [this](typename cell_format_node::it node) {
    if (!formats_gc_at_work_) {
      cell_formats_.template get<by_key>().erase(forest_t::key_of(node));
    }
  });

  forest_conns_.emplace_back(cell_format_meta.modified += [](typename cell_format_node::it) {
    ED_ASSERT(false);
  });

  forest_conns_.emplace_back(sheet_meta.inserted += [this](worksheet_node::it node) {
    ED_ASSERT(!node->sheet);
    auto pos = std::distance(forest_.get<worksheet_node>(book_node_).begin(), node);
    auto insert_pos = sheets_.begin();
    std::advance(insert_pos, pos);
    auto [i, ok] = sheets_.emplace(insert_pos, *this, node);
    ED_ASSERT(ok);

    sheets_count_ = *sheets_count_ + 1;
    sheet_inserted(const_cast<worksheet&>(*i));
  });

  forest_conns_.emplace_back(sheet_meta.erased += [this](worksheet_node::it node) {
    ED_ASSERT(node->sheet);
    node->sheet->removed();
    sheet_removed(*node->sheet);
    sheets_.erase(sheets_.iterator_to(*node->sheet));

    ED_ASSERT(*sheets_count_ > 0);
    sheets_count_ = *sheets_count_ - 1;

    if (*active_sheet_ == node->sheet) {
      ED_ASSERT(!sheets_.empty());
      active_sheet_.changed.block();
      active_sheet_ = nullptr;
      active_sheet_.changed.unblock();
      const_cast<worksheet&>(sheets_.front()).activate();
    }
  });

  forest_conns_.emplace_back(sheet_meta.modified += [](worksheet_node::it node) {
    ED_ASSERT(node->sheet);
    node->sheet->modified(node);
  });

  forest_conns_.emplace_back(sheet_meta.shifted += [this](worksheet_node::it node) {
    ED_ASSERT(node->sheet);
    auto i = sheets_.iterator_to(*node->sheet);
    ED_ASSERT(i != sheets_.end());
    auto pos = std::distance(forest_.get<worksheet_node>(book_node_).begin(), node);
    auto to_pos = sheets_.begin();
    std::advance(to_pos, pos);
    sheets_.relocate(to_pos, i);
    sheet_shifted(*node->sheet);
  });

  forest_conns_.emplace_back(column_meta.inserted += [this](column_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->inserted(node);
  });

  forest_conns_.emplace_back(column_meta.erased += [this](column_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->erased(node);
  });

  forest_conns_.emplace_back(column_meta.modified += [this](column_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->modified(node);
  });

  forest_conns_.emplace_back(row_meta.inserted += [this](row_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->inserted(node);
  });

  forest_conns_.emplace_back(row_meta.erased += [this](row_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->erased(node);
  });

  forest_conns_.emplace_back(row_meta.modified += [this](row_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->modified(node);
  });

  forest_conns_.emplace_back(cell_meta.inserted += [this](cell_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->inserted(node);
  });

  forest_conns_.emplace_back(cell_meta.erased += [this](cell_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->erased(node);
  });

  forest_conns_.emplace_back(cell_meta.modified += [this](cell_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->modified(node);
  });

  forest_conns_.emplace_back(cell_data_meta.inserted += [this](cell_data_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->inserted(node);
  });

  forest_conns_.emplace_back(cell_data_meta.erased += [this](cell_data_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->erased(node);
  });

  forest_conns_.emplace_back(cell_data_meta.modified += [this](cell_data_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->modified(node);
  });

  forest_conns_.emplace_back(text_run_meta.inserted += [this](text_run_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->inserted(node);
  });

  forest_conns_.emplace_back(text_run_meta.erased += [this](text_run_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->erased(node);
  });

  forest_conns_.emplace_back(text_run_meta.modified += [this](text_run_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->modified(node);
  });

  forest_conns_.emplace_back(cell_formula_meta.inserted += [this](cell_formula_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->inserted(node);
  });

  forest_conns_.emplace_back(cell_formula_meta.erased += [this](cell_formula_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->erased(node);
  });

  forest_conns_.emplace_back(cell_formula_meta.modified += [this](cell_formula_node::it node) {
    auto sheet_node = forest_.ancestor<worksheet_node>(node);
    ED_ASSERT(sheet_node->sheet);
    sheet_node->sheet->modified(node);
  });

  open();
}


workbook_node::it workbook::node() const noexcept {
  return book_node_;
}


const forest_t& workbook::forest() const noexcept {
  return forest_;
}


forest_t& workbook::forest() noexcept {
  return forest_;
}


const fx::parser& workbook::formula_parser() const noexcept {
  return formula_parser_;
}


fx::parser& workbook::formula_parser() noexcept {
  return formula_parser_;
}


workbook::sheets_crange workbook::sheets() const noexcept {
  return {sheets_};
}


workbook::sheets_range workbook::sheets() noexcept {
  return {sheets_};
}


const worksheet* workbook::sheet_by_name(std::wstring_view name) const noexcept {
  // TODO: Сделать регистронезависимый поиск.
  auto i = sheets_.get<by_name>().find(name);
  if (i != sheets_.get<by_name>().end()) {
    return &(*i);
  }
  return nullptr;
}


worksheet* workbook::sheet_by_name(std::wstring_view name) noexcept {
  // TODO: Сделать регистронезависимый поиск.
  auto i = sheets_.get<by_name>().find(name);
  if (i != sheets_.get<by_name>().end()) {
    return const_cast<worksheet*>(&(*i));
  }
  return nullptr;
}


const worksheet* workbook::sheet_by_index(std::size_t index) const noexcept {
  if (index < sheets_.size()) {
    return &sheets_[index];
  }
  return nullptr;
}


worksheet* workbook::sheet_by_index(std::size_t index) noexcept {
  if (index < sheets_.size()) {
    return const_cast<worksheet*>(&sheets_[index]);
  }
  return nullptr;
}


worksheet& workbook::emplace_sheet(std::size_t index) {
  worksheet_node n;

  std::size_t num = sheets_.size();
  std::wstring name;
  for(;;) {
    name = L"Sheet" + std::to_wstring(++num);
    if (!sheet_by_name(name)) {
      break;
    }
  }
  n.name = name;

  auto pos = forest_.get<worksheet_node>(book_node_).begin();
  std::advance(pos, index);

  scoped_transaction tr(forest_);
  auto sheet_node = forest_.insert(book_node_, pos, std::move(n));
  tr.commit();

  ED_ASSERT(sheet_node->sheet);
  return *sheet_node->sheet;
}


void workbook::undo() {
  if (forest_.can_undo()) {
    forest_.undo();
  }
}


void workbook::redo() {
  if (forest_.can_redo()) {
    forest_.redo();
  }
}


bool workbook::can_read_file_format(const ed::mime_type& format) const noexcept {
  return file_readers_.count(format) > 0;
}


bool workbook::can_write_file_format(const ed::mime_type& format) const noexcept {
  return file_writers_.count(format) > 0;
}


workbook::mimes_range workbook::readable_file_formats() const noexcept {
  return file_readers_ | boost::adaptors::map_keys;
}


workbook::mimes_range workbook::writable_file_formats() const noexcept {
  return file_writers_ | boost::adaptors::map_keys;
}


void workbook::add_file_reader(const ed::mime_type& format, file_reader&& fn) {
  ED_EXPECTS(fn);
  file_readers_[format] = std::move(fn);
}


void workbook::add_file_writer(const ed::mime_type& format, file_writer&& fn) {
  ED_EXPECTS(fn);
  file_writers_[format] = std::move(fn);
}


bool workbook::can_read_clipboard_format(const ed::mime_type& format) const noexcept {
  return clipboard_readers_.count(format) > 0;
}


bool workbook::can_write_clipboard_format(const ed::mime_type& format) const noexcept {
  return clipboard_writers_.count(format) > 0;
}


workbook::mimes_range workbook::readable_clipboard_formats() const noexcept {
  return clipboard_readers_ | boost::adaptors::map_keys;
}


workbook::mimes_range workbook::writable_clipboard_formats() const noexcept {
  return clipboard_writers_ | boost::adaptors::map_keys;
}


void workbook::add_clipboard_reader(const ed::mime_type& format, clipboard_reader&& fn) {
  ED_EXPECTS(fn);
  clipboard_readers_[format] = std::move(fn);
}


void workbook::add_clipboard_writer(const ed::mime_type& format, clipboard_writer&& fn) {
  ED_EXPECTS(fn);
  clipboard_writers_[format] = std::move(fn);
}


void workbook::set_locale(const std::locale& loc) {
  locale_ = loc;
}


const std::locale& workbook::get_locale() const noexcept {
  return locale_;
}


void workbook::open() {
  pre_open(true);

  scoped_transaction tr(forest_);
  {
    book_node_ = forest_.push_back(workbook_node{});

    worksheet_node sheet_n;
    sheet_n.name = L"Sheet1";
    forest_.push_back(book_node_, std::move(sheet_n));
  }
  tr.commit();

  post_open(true);
}


void workbook::open(const ed::mime_type& format, std::istream& is) {
  auto fr_it = file_readers_.find(format);
  if (fr_it == file_readers_.end()) {
    ED_THROW_EXCEPTION(unsupported_file_format());
  }

  pre_open(format == ed::mime_type::known::application_x_cellfy);

  try {
    fr_it->second(is, forest_);
  } catch (const std::exception&) {
    open();
    throw;
  }

  post_open(format == ed::mime_type::known::application_x_cellfy);
}


void workbook::save(const ed::mime_type& format, std::ostream& os) const {
  auto fw_it = file_writers_.find(format);
  if (fw_it == file_writers_.end()) {
    ED_THROW_EXCEPTION(unsupported_file_format());
  }
  fw_it->second(os, forest_);
}


cell_format_node::it workbook::ensure_format(cell_format&& fmt) {
  if (auto i = cell_formats_.find(fmt); i != cell_formats_.end()) {
    return *i;
  }

  scoped_transaction tr(forest_);
  cell_format_node n;
  n.format = std::make_shared<cell_format>(std::move(fmt));
  auto it = forest_.push_back(book_node_, std::move(n));
  tr.commit();
  return it;
}


void workbook::set_calc_mode(const calc_mode mode) {
  if (calc_mode_ != mode) {
    calc_mode_ = mode;
    if (calc_mode_ == calc_mode::automatic) {
      сalculate_formulas_on_all_sheets();
    }
  }
}


calc_mode workbook::get_calc_mode() const noexcept {
  return calc_mode_;
}


void workbook::calculate_formulas_on_active_sheet() {
  (*active_sheet_)->update_formulas_and_view();
}


void workbook::сalculate_formulas_on_all_sheets() {
  for (auto& sheet : sheets_) {
    const_cast<worksheet&>(sheet).update_formulas_and_view();
  }
}


void workbook::pre_open(bool block_signals) {
  about_to_close();

  if (block_signals) {
    for (auto& conn : forest_conns_) {
      conn.block(true);
    }

  }

  sheets_count_ = 0;
  active_sheet_ = nullptr;

  sheets_.clear();
  cell_formats_.clear();
  forest_.clear();
}


void workbook::post_open(bool unblock_signals) {
  forest_.clear_undo_stack();
  can_undo_ = false;
  can_redo_ = false;

  cell_formats_.clear();
  sheets_.clear();

  ED_ENSURES(!forest_.get<workbook_node>().empty());
  book_node_ = forest_.get<workbook_node>().begin();

  auto cell_format_nodes = forest_.get<cell_format_node>(book_node_);
  for (auto node = cell_format_nodes.begin(); node != cell_format_nodes.end(); ++node) {
    cell_formats_.insert(node);
  }

  auto sheet_nodes = forest_.get<worksheet_node>(book_node_);
  ED_EXPECTS(!sheet_nodes.empty());
  for (auto node = sheet_nodes.begin(); node != sheet_nodes.end(); ++node) {
    auto [i, ok] = sheets_.emplace_back(*this, node);
    ED_ASSERT(ok);
  }

  if (unblock_signals) {
    for (auto& conn : forest_conns_) {
      conn.block(false);
    }
  }

  // Формулы могут ссылаться на разные листы.
  // Если сначала создать 1 лист, а у него будет ссылка на лист 2. То формула не рассчитается.
  // После создания всех листов, обновляем layout всех ячеек, чтобы в каждом листе были посчитаны формулы. CEL-314.
  for (auto& sheet_node : sheet_nodes) {
    sheet_node.sheet->update_formulas();
  }

  sheets_count_ = sheets_.size();
  active_sheet_ = nullptr;
  sheets().front().activate();

  opened();
}


} // namespace lde::cellfy::boox
