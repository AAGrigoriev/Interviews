#pragma once


#include <string>

#include <boost/operators.hpp>

#include <lde/cellfy/boox/fwd.h>


namespace lde::cellfy::boox {


/// Адрес ячейки
class cell_addr final :
  public boost::equality_comparable<cell_addr>,
  public boost::less_than_comparable<cell_addr>,
  public boost::addable<cell_addr> {

public:
  static inline constexpr row_index    max_row_count    = 1048576;
  static inline constexpr column_index max_column_count = 16384;

public:
  static std::wstring row_name(row_index index);
  static std::wstring column_name(column_index index);

public:
  cell_addr() = default;
  cell_addr(cell_index index);
  cell_addr(column_index col, row_index row);

  bool operator==(const cell_addr& rhs) const noexcept;
  bool operator<(const cell_addr& rhs) const noexcept;
  cell_addr& operator+=(const cell_addr& rhs) noexcept;

  /// Строковое представление (например F4)
  operator std::wstring() const;

  /// Индекс. Увеличивается слева направо, сверку вниз
  cell_index index() const noexcept;

  /// Индекс колонки
  column_index column() const noexcept;

  /// Индекс строки
  row_index row() const noexcept;

private:
  cell_index index_ = 0;
};


} // namespace lde::cellfy::boox
