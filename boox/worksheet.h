#pragma once


#include <memory>
#include <string>
#include <unordered_set>

#include <ed/core/fwd.h>
#include <ed/core/mime.h>
#include <ed/core/property.h>
#include <ed/core/quantity.h>

#include <lde/cellfy/boox/forest.h>
#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/node.h>
#include <lde/cellfy/boox/range.h>


namespace lde::cellfy::boox {


/// Лист
class worksheet final {
  friend class range;
  friend class workbook;
  friend class parse_formulas_op;
  friend class change_existing_cell_format_op;
  friend class change_cell_format_op;

public:
  ed::ro_proxy_property<std::wstring> name;
  ed::ro_proxy_property<bool>         active;
  ed::ro_proxy_property<range>        selection;
  ed::ro_proxy_property<range>        active_cell;
  ed::signal<const range&>            changed;
  ed::signal<>                        removed;

public:
  worksheet(workbook& book, worksheet_node::it sheet_node);

  worksheet(const worksheet&) = delete;
  worksheet& operator=(const worksheet&) = delete;

  worksheet_node::it node() const noexcept;

  const workbook& book() const noexcept;
  workbook& book() noexcept;

  /// Индекс листа в книге
  std::size_t index() const noexcept;

  /// Все ячейки.
  range cells() const noexcept;

  /// Ячейки из диапазона.
  range cells(cell_addr first, cell_addr last) const;
  range cells(std::wstring_view a1) const;

  /// Возвращает диапазон с одной ячейкой.
  range cell(cell_addr addr) const;

  /// Стандартная ширина ячейки
  ed::twips<double> default_column_width() const noexcept;

  /// Стандартная высота ячейки
  ed::twips<double> default_row_height() const noexcept;

  /// Ширина колонок
  ed::twips<double> columns_width(column_index from, column_index to) const noexcept;

  /// Высота строк
  ed::twips<double> rows_height(row_index from, row_index to) const noexcept;

  /// Сделать страницу активной
  void activate();

  /// Переименовать
  bool rename(std::wstring_view name);

  /// Удалить лист
  bool remove();

  /// Вставить данные определенного формата.
  void paste(const ed::mime_type& format, const ed::buffer& data);

  /// Обновить layout всех ячеек на листе, не заносится в undo/redo.
  void update_formulas();

  /// Заново рассчитать все формулы на листе и обновить gui, не заносится в undo/redo.
  void update_formulas_and_view();

private:
  column_node::opt_it find_column(column_index index) const noexcept;
  row_node::opt_it find_row(row_index index) const noexcept;
  cell_node::opt_it find_cell(cell_addr addr) const noexcept;

  void changes_started();
  void changes_finished(calc_mode mode);

  void modified(worksheet_node::it node);

  void inserted(column_node::it node);
  void erased(column_node::it node);
  void modified(column_node::it node);

  void inserted(row_node::it node);
  void erased(row_node::it node);
  void modified(row_node::it node);

  void inserted(cell_node::it node);
  void erased(cell_node::it node);
  void modified(cell_node::it node);

  void inserted(cell_data_node::it node);
  void erased(cell_data_node::it node);
  void modified(cell_data_node::it node);

  void inserted(text_run_node::it node);
  void erased(text_run_node::it node);
  void modified(text_run_node::it node);

  void inserted(cell_formula_node::it node);
  void erased(cell_formula_node::it node);
  void modified(cell_formula_node::it node);

  void parse_formula(cell_node::it node);
  void parse_formula(cell_formula_node::it node);

  void actualize_format();

private:
  using volatile_cells = std::unordered_set<cell_node::it>;

  ed::property<std::wstring> name_;
  ed::property<bool>         active_ = {false};
  ed::property<range>        selection_;
  ed::property<range>        active_cell_;
  workbook&                  book_;
  worksheet_node::it         sheet_node_;
  range                      cells_;
  range                      changes_;
  ed::twips<double>          default_column_width_;
  ed::twips<double>          default_row_height_;
  volatile_cells             volatile_cells_;
};


} // namespace lde::cellfy::boox
