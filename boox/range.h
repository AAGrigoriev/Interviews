#pragma once


#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>

#include <boost/operators.hpp>

#include <ed/core/color.h>
#include <ed/core/fwd.h>
#include <ed/core/mime.h>
#include <ed/core/point.h>
#include <ed/core/quantity.h>
#include <ed/core/rect.h>

#include <ed/rasta/fwd.h>

#include <lde/cellfy/boox/area.h>
#include <lde/cellfy/boox/cell_value.h>
#include <lde/cellfy/boox/exception.h>
#include <lde/cellfy/boox/format.h>
#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/node.h>
#include <lde/cellfy/boox/range_op.h>


namespace lde::cellfy::boox {


/// Диапазон ячеек
class range final : public boost::equality_comparable<range> {
public:
  struct paint_options {
    ed::color                grid_line_color = default_line_color;
    std::optional<cell_addr> hidden_cell;
  };

  struct column_info {
    column_index        index = 0;
    ed::twips<double>   left  = 0._tw;
    ed::twips<double>   width = 0._tw;
    column_node::opt_it node;
  };

  struct row_info {
    row_index         index  = 0;
    ed::twips<double> top    = 0._tw;
    ed::twips<double>     height = 0._tw;
    row_node::opt_it  node;
  };

  using area_fn   = std::function<void(const range&)>;
  using column_fn = std::function<void(const column_info&)>;
  using row_fn    = std::function<void(const row_info&)>;
  using cell_fn   = std::function<void(range)>;

public:
  range() = default;
  explicit range(worksheet& sheet);

  bool operator==(const range& rhs) const noexcept;

  /// Строковое представление, например А7:D20
  operator std::wstring() const;

  const worksheet& sheet() const;
  worksheet& sheet();

  bool empty() const noexcept;

  /// Первая колонка в самой первой area.
  column_index column() const;

  /// Первая строка в самой первой area.
  row_index row() const;

  /// Адрес верхней левой ячейки в самой первой area.
  cell_addr addr() const;

  /// В диапазоне только одна ячейка.
  bool single_cell() const noexcept;

  /// В диапазоне только одна ячейка объединяющая остальные в этом range.
  bool single_merged_cell() const noexcept;

  /// В диапазоне только одна область.
  bool single_area() const noexcept;

  /// Содержит весь лист.
  bool contains_entire_sheet() const noexcept;

  /// Содержит строки целиком.
  bool contains_entire_rows() const noexcept;

  /// Содержит колонки целиком.
  bool contains_entire_columns() const noexcept;

  /// Содержит объединенные ячейки.
  bool contains_merged() const noexcept;

  /// Содержит только объединенные ячейки.
  bool contains_only_merged() const;

  /// Проверка на вхождение колонки.
  bool contains_column(column_index index) const noexcept;

  /// Проверка на вхождение строки.
  bool contains_row(row_index index) const noexcept;

  /// Проверка на вхождение ячейки.
  bool contains(cell_addr addr) const noexcept;

  /// Проверка на полное вхождение другой области.
  bool contains(const range& rng) const noexcept;

  /// Проверка на наличие пересечений.
  bool intersects(const range& rng) const noexcept;

  /// Тип содержимого ячеек. Если разный тип, то вернет std::nullopt
  std::optional<cell_value_type> value_type() const;

  /// Кол-во ячеек
  std::uint64_t cells_count() const noexcept;

  /// Кол-во колонок. Только для single_area.
  std::size_t columns_count() const;

  /// Кол-во строк. Только для single_area.
  std::size_t rows_count() const;

  /// Верхняя левая ячейка.
  range top_left() const;

  /// Верхняя правая ячейка.
  range top_right() const;

  /// Нижняя левая ячейка.
  range bottom_left() const;

  /// Нижняя правая ячейка.
  range bottom_right() const;

  /// Возвращает поддиапазон. Только для single_area.
  range cells(cell_addr rel_first, cell_addr rel_last) const;

  /// Возвращает поддиапазон. Только для single_area. a1 - формат записи A1 (A1:H8, H8)
  range cells(std::wstring_view a1) const;

  /// Возвращает диапазон с одной ячейкой. Только для single_area.
  range cell(cell_addr rel_addr) const;

  /// Ячейка по координатам.
  range cell_by_xy(const ed::point_tw<double>& xy) const;

  /// Возвращает диапазон смещенный относительно текущего.
  range offset(int column_offs, int row_offs) const;

  /// Возвращает диапазон с имененным размером;
  range resize(std::optional<std::size_t> columns, std::optional<std::size_t> rows) const;

  /// Пересечение 2-х диапазонов.
  range intersect(const range& rng) const;

  /// Получение диапазона пересекающегося с rect.
  range intersect(const ed::rect_tw<double>& rect) const;

  /// Возвращает диапазон всей колонки.
  range entire_column(column_index rel_index) const;

  /// Возвращает диапазон всей строки.
  range entire_row(row_index rel_index) const;

  /// Строка над этой областью.
  range row_above() const;

  /// Строка под этой областью.
  range row_below() const;

  /// Колонка левее этой области.
  range column_at_left() const;

  /// Колонка правее этой области.
  range column_at_right() const;

  /// Объединение в один диапазон с одной областью.
  range unite(const range& rng) const noexcept;

  /// Объединение в один диапазон с одной областью.
  range united() const noexcept;

  /// Объединение диапазонов.
  range join(const range& rng) const;

  /// Возвращает диапазон c учетом объединения ячеек.
  range merge_area() const;

  /// Возвращает диапазон в который не включены пустые ячейки расположенные справа.
  range right_trimmed() const;

  /// Возвращает диапазон в который не включены пустые ячейки расположенные снизу.
  range bottom_trimmed() const;

  /// Получить список всех поддиапазонов.
  void for_each_area(const area_fn& fn) const;

  /// Пройтись по всем колонкам.
  void for_each_column(const column_fn& fn) const;

  /// Пройтись по всем строкам.
  void for_each_row(const row_fn& fn) const;

  /// Пройтись по всем ячейкам.
  void for_each_cell(const cell_fn& fn) const;

  /// Пройтись по существующим ячейкам.
  void for_existing_cells(const cell_fn& fn) const;

  /// Координата x относительно A1.
  ed::twips<double> left() const;

  /// Координата y относительно A1.
  ed::twips<double> top() const;

  /// Ширина.
  ed::twips<double> width() const;

  /// Высота.
  ed::twips<double> height() const;

  /// Получить формат ячеек
  cell_format format() const;

  /// Получить значение ячеек. Если значения разные то вернется nullptr.
  cell_value value() const;

  /// Получить значения всех не пустых ячеек.
  cell_value::list values() const;

  /// Получить значения всех ячеек. Вызывать только для single_area.
  /// Возвращает матрицу значений размером rows_count x columns_count.
  cell_value::matrix values_matrix() const;

  /// Получить значение или формулу ячеек в текстовом виде.
  std::wstring text() const;

  /// Получить значение ячейки для редактирования.
  std::wstring text_for_edit() const;

  /// Объединить ячейки
  range& merge();

  /// Разъединить ячейки
  range& unmerge();

  /// Задать формат ячеек
  range& set_format(cell_format::changes&& changes);

  /// Задать определенные поля формата ячеек
  template<typename ... Field>
  range& set_format(std::optional<typename Field::value_type>&& ... value);

  /// Очистить формат ячеек
  range& clear_format();

  /// Подготовить и применить к ячейкам числовой формат.
  /// Автоматический формат - пустая строка форматирования.
  /// Текстовый формат - строка с @.
  /// Любой другой числовой формат - это не пустая строка форматирования.
  void prepare_and_apply_number_format(const number_format::value_type& format);

  /// Функция заполнение range. Передаётся range, который включает исходный (this).
  /// Заполняется данными из исходного range.
  /// todo: 2 параметра :
  ///  - только копирование (значения и формат), без применения формул и рассчётов.
  ///  - применение определённого формата на все ячейки, не зависимо от текущих форматов, установленного в ячейках.
  /// https://docs.microsoft.com/ru-ru/office/vba/api/excel.range.autofill
  void fill(range& out) const;

  /// Обновить ширину столбца.
  range& update_column_width();

  /// Задать дефолтную высоту строки.
  range& set_default_row_height();

  /// Задать все границы.
  range& set_all_borders(const border& br);

  /// Задать внутренние границы.
  range& set_inner_borders(const border& br);

  /// Задать внутренние горизонтальные границы.
  range& set_horizontal_borders(const border& br);

  /// Задать внутренние вертикальные границы.
  range& set_vertical_borders(const border& br);

  /// Задать внешние границы.
  range& set_outer_borders(const border& br);

  /// Задать внешнюю верхнюю границу.
  range& set_top_borders(const border& br);

  /// Задать внешнюю нижнюю границу.
  range& set_bottom_borders(const border& br);

  /// Задать внешнюю левую границу.
  range& set_left_borders(const border& br);

  /// Задать внешнюю правую границу.
  range& set_right_borders(const border& br);

  /// Убрать все границы.
  range& clear_all_borders();

  /// Задать ширину колонок.
  range& set_column_width(ed::twips<double> val);

  /// Задать высоту строк.
  range& set_row_height(ed::twips<double> val);

  /// Задать значение.
  range& set_value(const cell_value& val);

  /// Задать значение. Может задать или число или строку или формулу, зависит от val.
  range& set_text(const std::wstring& val);

  /// Применить операцию (наследник от range_op) к диапазону
  template<typename Op>
  void apply(Op&& op) const;

  /// Выделить диапазон
  void select() const;

  /// Сделать top_left ячейку активной. Ячейка должна быть в выделении.
  void activate() const;

  /// Нарисовать диапазон
  void paint(ed::rasta::painter& pnt, const paint_options& opts) const;

  /// Получить копию данных в определенном вормате.
  ed::buffer copy(const ed::mime_type& format) const;

private:
  range(worksheet& sheet, const area& ar);
  range(worksheet& sheet, area::list&& areas);

  column_index column_by_x(ed::twips<double> x) const noexcept;
  row_index row_by_y(ed::twips<double> y) const noexcept;

private:
  worksheet* sheet_ = nullptr;
  area       united_;
  area::list areas_;
};


template<typename ... Field>
range& range::set_format(std::optional<typename Field::value_type>&& ... value) {
  static_assert((cell_format::can_hold<Field>() && ...), "cell format field expected");
  cell_format::changes changes;
  (changes.set<Field>(std::move(value)), ...);
  return set_format(std::move(changes));
}


template<typename Op>
void range::apply(Op&& op) const {
  static_assert(std::is_base_of_v<range_op, Op>);
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  using processing = typename Op::processing;
  range_op_ctx ctx(*sheet_, areas_);

  if constexpr (std::is_same_v<processing, for_all_columns_tag>) {
    range_op::for_all_columns(ctx, op);
  } else if constexpr (std::is_same_v<processing, for_existing_columns_tag>) {
    range_op::for_existing_columns(ctx, op);
  } else if constexpr (std::is_same_v<processing, for_all_rows_tag>) {
    range_op::for_all_rows(ctx, op);
  } else if constexpr (std::is_same_v<processing, for_existing_rows_tag>) {
    range_op::for_existing_rows(ctx, op);
  } else if constexpr (std::is_same_v<processing, for_all_cells_tag>) {
    range_op::for_all_cells(ctx, op);
  } else if constexpr (std::is_same_v<processing, for_existing_cells_tag>) {
    range_op::for_existing_cells(ctx, op);
  }
}


} // namespace lde::cellfy::boox
