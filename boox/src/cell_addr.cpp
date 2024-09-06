#include <lde/cellfy/boox/cell_addr.h>

#include <ed/core/assert.h>

#include <lde/cellfy/boox/src/base26.h>


namespace lde::cellfy::boox {


std::wstring cell_addr::row_name(row_index index) {
  ED_EXPECTS(index < max_row_count);

  return std::to_wstring(index + 1);
}


std::wstring cell_addr::column_name(column_index index) {
  ED_EXPECTS(index < max_column_count);

  std::wstring str;
  base26::encode(index, str);
  return str;
}


cell_addr::cell_addr(cell_index index)
  : index_(index) {

  ED_EXPECTS(column() < max_column_count);
  ED_EXPECTS(row() < max_row_count);
}


cell_addr::cell_addr(column_index col, row_index row)
  : index_(cell_index(row) * (max_column_count) + col) {

  ED_EXPECTS(col < max_column_count);
  ED_EXPECTS(row < max_row_count);
}


bool cell_addr::operator==(const cell_addr& rhs) const noexcept {
  return index_ == rhs.index_;
}


bool cell_addr::operator<(const cell_addr& rhs) const noexcept {
  return index_ < rhs.index_;
}


cell_addr& cell_addr::operator+=(const cell_addr& rhs) noexcept {
  *this = cell_addr(column() + rhs.column(), row() + rhs.row());
  return *this;
}


cell_addr::operator std::wstring() const {
  return column_name(column()) + row_name(row());
}


cell_index cell_addr::index() const noexcept {
  return index_;
}


column_index cell_addr::column() const noexcept {
  return column_index(index_ % (max_column_count));
}


row_index cell_addr::row() const noexcept {
  return row_index(index_ / (max_column_count));
}


} // namespace lde::cellfy::boox
