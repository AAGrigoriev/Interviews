#include <lde/cellfy/boox/area.h>

#include <algorithm>

#include <boost/spirit/home/x3.hpp>

#include <ed/core/assert.h>

#include <lde/cellfy/boox/src/base26.h>


namespace lde::cellfy::boox {


area::area(cell_addr addr) noexcept
  : tl_(addr)
  , br_(addr) {
}


area::area(cell_addr first, cell_addr second) noexcept
  : tl_(std::min(first.column(), second.column()), std::min(first.row(), second.row()))
  , br_(std::max(first.column(), second.column()), std::max(first.row(), second.row())) {
}


area::area(std::wstring_view a1) {
  using namespace boost::spirit;

  auto begin = a1.begin();
  bool ok = x3::parse(
    begin,
    a1.end(),
    (+x3::unicode::char_(L"a-zA-Z") >> x3::uint32)[([this](auto& ctx) {
      column_index col;
      auto& a1_col = boost::fusion::at_c<0>(x3::_attr(ctx));
      base26::decode(std::wstring(a1_col.begin(), a1_col.end()), col);
      tl_ = cell_addr(col, boost::fusion::at_c<1>(x3::_attr(ctx)) - 1);
      br_ = tl_;
    })] >> -(L":" >> (+x3::unicode::char_(L"a-zA-Z") >> x3::uint32)[([this](auto& ctx) {
      column_index col;
      auto& a1_col = boost::fusion::at_c<0>(x3::_attr(ctx));
      base26::decode(std::wstring(a1_col.begin(), a1_col.end()), col);
      br_ = cell_addr(col, boost::fusion::at_c<1>(x3::_attr(ctx)) - 1);
    })]));

  ED_EXPECTS(ok && begin == a1.end());
  ED_EXPECTS(tl_ <= br_);
}


bool area::operator==(const area& rhs) const noexcept {
  return tl_ == rhs.tl_ && br_ == rhs.br_;
}


bool area::operator<(const area& rhs) const noexcept {
  return tl_ < rhs.tl_ || (tl_ == rhs.tl_ && br_ < rhs.br_);
}


area::operator std::wstring() const {
  if (tl_ == br_) {
    return tl_;
  }

  if (left_column() == 0 && right_column() == cell_addr::max_column_count - 1) {
    return cell_addr::row_name(top_row()) + L":" + cell_addr::row_name(bottom_row());
  } else if (top_row() == 0 && bottom_row() == cell_addr::max_row_count - 1) {
    return cell_addr::column_name(left_column()) + L":" + cell_addr::column_name(right_column());
  } else {
    return std::wstring(tl_) + L":" + std::wstring(br_);
  }
}


column_index area::left_column() const noexcept {
  return tl_.column();
}


column_index area::right_column() const noexcept {
  return br_.column();
}


row_index area::top_row() const noexcept {
  return tl_.row();
}


row_index area::bottom_row() const noexcept {
  return br_.row();
}


cell_addr area::top_left() const noexcept {
  return tl_;
}


cell_addr area::top_right() const noexcept {
  return {br_.column(), tl_.row()};
}


cell_addr area::bottom_left() const noexcept {
  return {tl_.column(), br_.row()};
}


cell_addr area::bottom_right() const noexcept {
  return br_;
}


std::size_t area::columns_count() const noexcept {
  return br_.column() - tl_.column() + 1;
}


std::size_t area::rows_count() const noexcept {
  return br_.row() - tl_.row() + 1;
}


std::uint64_t area::cells_count() const noexcept {
  return static_cast<std::uint64_t>(columns_count()) * rows_count();
}


bool area::single_cell() const noexcept {
  return tl_ == br_;
}


bool area::contains_entire_sheet() const noexcept {
  return contains_entire_rows() && contains_entire_columns();
}


bool area::contains_entire_rows() const noexcept {
  return left_column() == 0 && right_column() == cell_addr::max_column_count - 1;
}


bool area::contains_entire_columns() const noexcept {
  return top_row() == 0 && bottom_row() == cell_addr::max_row_count - 1;
}


bool area::contains_column(column_index index) const noexcept {
  return index >= left_column() && index <= right_column();
}


bool area::contains_row(row_index index) const noexcept {
  return index >= top_row() && index <= bottom_row();
}


bool area::contains(cell_addr addr) const noexcept {
  return contains_column(addr.column()) && contains_row(addr.row());
}


bool area::contains(const area& ar) const noexcept {
  return contains(ar.tl_) && contains(ar.br_);
}


bool area::intersects(const area& ar) const noexcept {
  if (left_column() > ar.right_column() || ar.left_column() > right_column()) {
    return false;
  }

  if (top_row() > ar.bottom_row() || ar.top_row() > bottom_row()) {
    return false;
  }

  return true;
}


std::optional<area> area::intersect(const area& ar) const noexcept {
  if (!intersects(ar)) {
    return std::nullopt;
  }

  auto l = std::max(left_column(), ar.left_column());
  auto t = std::max(top_row(), ar.top_row());
  auto r = std::min(right_column(), ar.right_column());
  auto b = std::min(bottom_row(), ar.bottom_row());

  return area({l, t}, {r, b});
}


area area::offset(int column_offs, int row_offs) const noexcept {
  auto l = static_cast<column_index>(std::clamp(
    static_cast<int>(left_column()) + column_offs,
    0,
    static_cast<int>(cell_addr::max_column_count - 1)));

  auto r = static_cast<column_index>(std::clamp(
    static_cast<int>(right_column()) + column_offs,
    0,
    static_cast<int>(cell_addr::max_column_count - 1)));

  auto t = static_cast<row_index>(std::clamp(
    static_cast<int>(top_row()) + row_offs,
    0,
    static_cast<int>(cell_addr::max_row_count - 1)));

  auto b = static_cast<row_index>(std::clamp(
    static_cast<int>(bottom_row()) + row_offs,
    0,
    static_cast<int>(cell_addr::max_row_count - 1)));

  return area({l, t}, {r, b});
}


area area::entire_column(column_index rel_index) const noexcept {
  column_index index = left_column() + rel_index;
  ED_ASSERT(contains_column(index));
  return {{index, top_row()}, {index, bottom_row()}};
}


area area::entire_columns(column_index rel_from, column_index rel_to) const noexcept {
  ED_ASSERT(rel_from <= rel_to);
  column_index from = left_column() + rel_from;
  column_index to = left_column() + rel_to;
  ED_ASSERT(contains_column(from));
  ED_ASSERT(contains_column(to));

  return {{from, top_row()}, {to, bottom_row()}};
}


area area::entire_row(row_index rel_index) const noexcept {
  row_index index = top_row() + rel_index;
  ED_ASSERT(contains_row(index));

  return {{left_column(), index}, {right_column(), index}};
}


area area::entire_rows(row_index rel_from, row_index rel_to) const noexcept {
  ED_ASSERT(rel_from <= rel_to);
  row_index from = top_row() + rel_from;
  row_index to = top_row() + rel_to;
  ED_ASSERT(contains_row(from));
  ED_ASSERT(contains_row(to));

  return {{left_column(), from}, {right_column(), to}};
}


area area::unite(const area& ar) const noexcept {
  auto l = std::min(left_column(), ar.left_column());
  auto t = std::min(top_row(), ar.top_row());
  auto r = std::max(right_column(), ar.right_column());
  auto b = std::max(bottom_row(), ar.bottom_row());

  return {{l, t}, {r, b}};
}


std::pair<area, area> area::split_horizontal(column_index rel_index) const noexcept {
  ED_ASSERT(contains_column(left_column() + rel_index));
  ED_ASSERT(rel_index > 0);
  return {
    entire_columns(0, rel_index - 1),
    entire_columns(rel_index, columns_count() - 1)
  };
}


std::pair<area, area> area::split_vertical(row_index rel_index) const noexcept {
  ED_ASSERT(contains_row(top_row() + rel_index));
  ED_ASSERT(rel_index > 0);
  return {
    entire_rows(0, rel_index - 1),
    entire_rows(rel_index, rows_count() - 1)
  };
}


area::list area::join(const area& ar) const noexcept {
  if (ar.top_row() == top_row() && ar.bottom_row() == bottom_row() && ar.right_column() == left_column() - 1) {
    return {{ar.top_left(), bottom_right()}};
  }
  if (ar.left_column() == left_column() && ar.right_column() == right_column() && ar.bottom_row() == top_row() - 1) {
    return {{ar.top_left(), bottom_right()}};
  }
  if (ar.left_column() == left_column() && ar.right_column() == right_column() && ar.top_row() == bottom_row() + 1) {
    return {{top_left(), ar.bottom_right()}};
  }
  if (ar.top_row() == top_row() && ar.bottom_row() == bottom_row() && ar.left_column() == right_column() + 1) {
    return {{top_left(), ar.bottom_right()}};
  }
  return {*this, ar};
}


area::list area::disjoin(const area& ar) const {
  if (ar.contains(*this)) {
    return {};
  }

  auto intr = intersect(ar);
  if (!intr.has_value()) {
    return {*this};
  }

  list result;
  area rest = *this;

  if (intr->top_left() != rest.top_left()) {
    if (intr->top_row() == rest.top_row()) {
      auto [l, r] = rest.split_horizontal(intr->left_column() - rest.left_column());
      result.push_back(l);
      rest = r;
    } else if (intr->left_column() == rest.left_column()) {
      auto [t, b] = rest.split_vertical(intr->top_row() - rest.top_row());
      result.push_back(t);
      rest = b;
    } else {
      auto [t, b] = rest.split_vertical(intr->top_row() - rest.top_row());
      result.push_back(t);
      auto [l, r] = b.split_horizontal(intr->left_column() - rest.left_column());
      result.push_back(l);
      rest = r;
    }
  }

  if (intr->top_right() != rest.top_right()) {
    if (intr->top_row() == rest.top_row()) {
      auto [l, r] = rest.split_horizontal(intr->right_column() - rest.left_column() + 1);
      result.push_back(r);
      rest = l;
    } else if (intr->right_column() == rest.right_column()) {
      auto [t, b] = rest.split_vertical(intr->top_row() - rest.top_row());
      result.push_back(t);
      rest = b;
    } else {
      auto [t, b] = rest.split_vertical(intr->top_row() - rest.top_row());
      result.push_back(t);
      auto [l, r] = b.split_horizontal(intr->right_column() - rest.left_column() + 1);
      result.push_back(r);
      rest = l;
    }
  }

  if (intr->bottom_left() != rest.bottom_left()) {
    if (intr->bottom_row() == rest.bottom_row()) {
      auto [l, r] = rest.split_horizontal(intr->left_column() - rest.left_column());
      result.push_back(l);
      rest = r;
    } else if (intr->left_column() == rest.left_column()) {
      auto [t, b] = rest.split_vertical(intr->bottom_row() - rest.top_row() + 1);
      result.push_back(b);
      rest = t;
    } else {
      auto [t, b] = rest.split_vertical(intr->bottom_row() - rest.top_row() + 1);
      result.push_back(b);
      auto [l, r] = t.split_horizontal(intr->left_column() - rest.left_column());
      result.push_back(l);
      rest = r;
    }
  }

  if (intr->bottom_right() !=rest.bottom_right()) {
    if (intr->bottom_row() == rest.bottom_row()) {
      auto [l, r] = rest.split_horizontal(intr->right_column() - rest.left_column() + 1);
      result.push_back(r);
      rest = l;
    } else if (intr->right_column() == rest.right_column()) {
      auto [t, b] = rest.split_vertical(intr->top_row() - rest.top_row() + 1);
      result.push_back(b);
      rest = t;
    } else {
      auto [t, b] = rest.split_vertical(intr->top_row() - rest.top_row() + 1);
      result.push_back(b);
      auto [l, r] = t.split_horizontal(intr->right_column() - rest.left_column() + 1);
      result.push_back(r);
      rest = l;
    }
  }

  return result;
}


} // namespace lde::cellfy::boox
