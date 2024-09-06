#pragma once


#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include <boost/container/small_vector.hpp>
#include <boost/operators.hpp>

#include <lde/cellfy/boox/cell_addr.h>


namespace lde::cellfy::boox {


class area final :
  public boost::equality_comparable<area>,
  public boost::less_than_comparable<cell_addr> {

public:
  using list = boost::container::small_vector<area, 32>;

public:
  area() = default;
  area(cell_addr addr) noexcept;
  area(cell_addr first, cell_addr second) noexcept;
  area(std::wstring_view a1);

  bool operator==(const area& rhs) const noexcept;
  bool operator<(const area& rhs) const noexcept;

  /// Строковое представление, например А7:D20
  operator std::wstring() const;

  /// Левая колонка.
  column_index left_column() const noexcept;

  /// Правая колонка.
  column_index right_column() const noexcept;

  /// Верхняя строка.
  row_index top_row() const noexcept;

  /// Нижняя строка.
  row_index bottom_row() const noexcept;

  /// Верхняя левая ячейка.
  cell_addr top_left() const noexcept;

  /// Верхняя правая ячейка.
  cell_addr top_right() const noexcept;

  /// Нижняя левая ячейка.
  cell_addr bottom_left() const noexcept;

  /// Нижняя правая ячейка.
  cell_addr bottom_right() const noexcept;

  /// Кол-во колонок.
  std::size_t columns_count() const noexcept;

  /// Кол-во строк.
  std::size_t rows_count() const noexcept;

  /// Кол-во ячеек
  std::uint64_t cells_count() const noexcept;

  /// В области только одна ячейка
  bool single_cell() const noexcept;

  /// Содержит весь лист.
  bool contains_entire_sheet() const noexcept;

  /// Содержит строки целиком.
  bool contains_entire_rows() const noexcept;

  /// Содержит колонки целиком.
  bool contains_entire_columns() const noexcept;

  /// Проверка на вхождение колонки
  bool contains_column(column_index index) const noexcept;

  /// Проверка на вхождение строки
  bool contains_row(row_index index) const noexcept;

  /// Проверка на вхождение ячейки.
  bool contains(cell_addr addr) const noexcept;

  /// Проверка на полное вхождение другой области
  bool contains(const area& ar) const noexcept;

  /// Проверка на наличие пересечения.
  bool intersects(const area& ar) const noexcept;

  /// Пересечение 2-х облатей
  std::optional<area> intersect(const area& ar) const noexcept;

  /// Возвращает область смещенную относительно текущей.
  area offset(int column_offs, int row_offs) const noexcept;

  /// Возвращает область всей колонки.
  area entire_column(column_index rel_index) const noexcept;

  /// Область целых колонок от from до to
  area entire_columns(column_index rel_from, column_index rel_to) const noexcept;

  /// Возвращает область целой строки.
  area entire_row(row_index rel_index) const noexcept;

  /// Область целых строк от from до to
  area entire_rows(row_index rel_from, row_index rel_to) const noexcept;

  /// Объединяющая область
  area unite(const area& ar) const noexcept;

  /// Разбить по горизонтали. index попадет в правую область.
  std::pair<area, area> split_horizontal(column_index rel_index) const noexcept;

  /// Разбить по вертикали. index попадет в нижнюю область
  std::pair<area, area> split_vertical(row_index rel_index) const noexcept;

  /// Если область рядом и можно объединить, то возвращает объединенную, иначе две области
  list join(const area& ar) const noexcept;

  /// Изъятие ar из текущей области. Возвращается 0 или более получившихся областей
  list disjoin(const area& ar) const;

private:
  cell_addr tl_;
  cell_addr br_;
};


} // namespace lde::cellfy::boox
