#include <lde/cellfy/boox/range.h>

#include <algorithm>
#include <cwchar>
#include <functional>
#include <optional>
#include <tuple>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/lexical_cast.hpp>

#include <ed/core/assert.h>

#include <ed/rasta/dpi.h>
#include <ed/rasta/painter.h>
#include <ed/rasta/physical_font.h>
#include <ed/rasta/matrix.h>
#include <ed/rasta/text_layout.h>

#include <lde/cellfy/boox/fx_engine.h>
#include <lde/cellfy/boox/fx_parser.h>
#include <lde/cellfy/boox/scoped_transaction.h>
#include <lde/cellfy/boox/value_format.h>
#include <lde/cellfy/boox/vector_2d.h>
#include <lde/cellfy/boox/workbook.h>
#include <lde/cellfy/boox/worksheet.h>
#include <lde/cellfy/boox/src/cell_op.h>
#include <lde/cellfy/boox/src/column_op.h>
#include <lde/cellfy/boox/src/row_op.h>
#include <lde/cellfy/boox/src/value_parser.h>


namespace lde::cellfy::boox {
namespace _ {
namespace {


struct cell_info {
  using matrix = vector_2d<cell_info>;

  std::size_t                    matrix_row            = 0;
  std::size_t                    matrix_col            = 0;
  cell_node::opt_it              node;
  const cell_node*               merged_with           = nullptr;
  cell_format::ptr               format;
  std::optional<cell_value_type> formula_type; ///< Тип значения формулы. Если формула была посчитана.
  const cell_info*               left_non_overlapping  = nullptr;
  const cell_info*               right_non_overlapping = nullptr;
  ed::rect_px<long long>         rect;
  ed::rect_px<long long>         merged_rect;
  ed::rect_px<long long>         layout_rect;
  bool                           draw_top_border       = true;
  bool                           draw_bottom_border    = true;
  bool                           draw_left_border      = true;
  bool                           draw_right_border     = true;
  bool                           fill_sharp            = false;
  ed::rasta::matrix              rotate_matrix;
};


int border_power(border_style style) noexcept {
  switch (style) {
    case boox::border_style::none:   return 0;
    case boox::border_style::thin:   return 1;
    case boox::border_style::medium: return 2;
    case boox::border_style::thick:  return 3;
  }
  ED_ASSERT(false);
  return 1;
}


template<typename Border1, typename Border2>
bool is_more_power(const cell_format::ptr& fmt1, const cell_format::ptr& fmt2) noexcept {
  auto border1 = fmt1 ? fmt1->get_optional<Border1>() : std::nullopt;
  auto border2 = fmt2 ? fmt2->get_optional<Border2>() : std::nullopt;

  if (border1 && border2) {
    return border_power(border1->style) > border_power(border2->style);
  }
  return border1.has_value();
}


template<typename Border1, typename Border2>
bool is_more_power_eq(const cell_format::ptr& fmt1, const cell_format::ptr& fmt2) noexcept {
  auto border1 = fmt1 ? fmt1->get_optional<Border1>() : std::nullopt;
  auto border2 = fmt2 ? fmt2->get_optional<Border2>() : std::nullopt;

  if (border1 && border2) {
    return border_power(border1->style) >= border_power(border2->style);
  }
  return !border2.has_value();
}


ed::pixels<double> border_width(border_style style) noexcept {
  switch (style) {
    case boox::border_style::none:   return 0_px;
    case boox::border_style::thin:   return 1_px;
    case boox::border_style::medium: return 2_px;
    case boox::border_style::thick:  return 3_px;
  }
  ED_ASSERT(false);
  return 1_px;
}


void draw_border_side(
  ed::rasta::painter& pnt,
  const ed::point_px<double>& p1,
  const ed::point_px<double>& p2,
  const border& fmt) noexcept {

  pnt.move_to(p1);
  pnt.line_to(p2);
  pnt.line_width(border_width(fmt.style));
  pnt.source(fmt.line_color);
  pnt.stroke();
}


void draw_border_sides(
  ed::rect_px<double> rect,
  const std::optional<border>& top_side,
  const std::optional<border>& bottom_side,
  const std::optional<border>& left_side,
  const std::optional<border>& right_side,
  ed::rasta::painter& pnt) noexcept {

  if (top_side) {
    draw_border_side(
      pnt,
      rect.top_left().x_added(-border_width(top_side->style) / 2.),
      rect.top_right().x_added(border_width(top_side->style) / 2.),
      *top_side);
  }
  if (bottom_side) {
    draw_border_side(
      pnt,
      rect.bottom_left().x_added(-border_width(bottom_side->style) / 2.),
      rect.bottom_right().x_added(border_width(bottom_side->style) / 2.),
      *bottom_side);
  }
  if (left_side) {
    draw_border_side(
      pnt,
      rect.top_left().y_added(-border_width(left_side->style) / 2.),
      rect.bottom_left().y_added(border_width(left_side->style) / 2.),
      *left_side);
  }
  if (right_side) {
    draw_border_side(
      pnt,
      rect.top_right().y_added(-border_width(right_side->style) / 2.),
      rect.bottom_right().y_added(border_width(right_side->style) / 2.),
      *right_side);
  }
}


void draw_grid_lines(const cell_info& info, const range::paint_options& opts, ed::rasta::painter& pnt) {
  std::optional<border> top_side;
  std::optional<border> bottom_side;
  std::optional<border> left_side;
  std::optional<border> right_side;

  if (info.draw_top_border) {
    if (!info.format || !info.format->holds<top_border>()) {
      top_side = border{border_style::thin, opts.grid_line_color};
    }
  }
  if (info.draw_bottom_border) {
    if (!info.format || !info.format->holds<bottom_border>()) {
      bottom_side = border{border_style::thin, opts.grid_line_color};
    }
  }
  if (info.draw_left_border) {
    if (!info.format || !info.format->holds<left_border>()) {
      left_side = border{border_style::thin, opts.grid_line_color};
    }
  }
  if (info.draw_right_border) {
    if (!info.format || !info.format->holds<right_border>()) {
      right_side = border{border_style::thin, opts.grid_line_color};
    }
  }

  ed::rect_px<double> cell_border = info.rect;
  cell_border.x -= 0.5_px;
  cell_border.y -= 0.5_px;

  draw_border_sides(cell_border, top_side, bottom_side, left_side, right_side, pnt);
}


void draw_border(const cell_info& info, ed::rasta::painter& pnt) {
  std::optional<border> top_side;
  std::optional<border> bottom_side;
  std::optional<border> left_side;
  std::optional<border> right_side;

  if (info.draw_top_border) {
    if (info.format && info.format->holds<top_border>()) {
      top_side = info.format->get<top_border>();
    }
  }
  if (info.draw_bottom_border) {
    if (info.format && info.format->holds<bottom_border>()) {
      bottom_side = info.format->get<bottom_border>();
    }
  }
  if (info.draw_left_border) {
    if (info.format && info.format->holds<left_border>()) {
      left_side = info.format->get<left_border>();
    }
  }
  if (info.draw_right_border) {
    if (info.format && info.format->holds<right_border>()) {
      right_side = info.format->get<right_border>();
    }
  }

  ed::rect_px<double> cell_border = info.rect;
  cell_border.x -= 0.5_px;
  cell_border.y -= 0.5_px;

  draw_border_sides(cell_border, top_side, bottom_side, left_side, right_side, pnt);
}


void draw_cell_content(const cell_info& info, ed::rasta::painter& pnt) {
  if (info.node && info.node.value()->merged_with) {
    return;
  }

  if (info.format && info.format->holds<fill_pattern_type>() && info.format->holds<fill_background>()) {
    ed::rect_px<double> fill_area = info.merged_rect;
    fill_area.x -= 0.5_px;
    fill_area.y -= 0.5_px;

    pnt.source(info.format->get<fill_background>());
    pnt.rectangle(fill_area);
    pnt.fill();
  }

  if (info.node) {
    auto& cell = *info.node.value();

    if (cell.layout) {
      ED_ASSERT(!cell.is_layout_dirty);
      ed::rasta::painter::scoped_save _(pnt);

      auto clip_rect = info.layout_rect;
      bool clip = false;
      const bool format_holds_text_rotation = cell.format && cell.format->holds<text_rotation>();

      if (cell.row_span > 1 || cell.column_span > 1) { // Объединенные ячейки всегда надо обрезать
        if (info.layout_rect.left() < info.merged_rect.left()) {
          clip_rect.left(info.merged_rect.left());
          clip = true;
        }
        if (info.layout_rect.right() > info.merged_rect.right()) {
          clip_rect.right(info.merged_rect.right());
          clip = true;
        }
      } else {
        if (info.left_non_overlapping) {
          if (info.left_non_overlapping->merged_rect.right() > info.layout_rect.left()) {
            clip_rect.left(info.left_non_overlapping->merged_rect.right());
            clip = true;
          }
        }
        if (info.right_non_overlapping) {
          if (info.right_non_overlapping->merged_rect.left() < info.layout_rect.right()) {
            clip_rect.right(info.right_non_overlapping->merged_rect.left());
            clip = true;
          }
        }
      }

      // Дополнтительно обрезаем текст сверху/снизу. Если он не влазит в ячейку по высоте.
      if (format_holds_text_rotation) {
        const auto rot = cell.format->get<text_rotation>();
        if (rot > 0._deg) {
          clip_rect.bottom(info.merged_rect.bottom());
        } else {
          clip_rect.top(info.merged_rect.top());
        }
        clip = true;
      }

      if (clip) {
        pnt.rectangle(clip_rect);
        pnt.clip();
      }

      // Если выбран поворот, то ячейки не заполняются #, если значение ячейки не помещается в ячейку.
      if (format_holds_text_rotation) {
        pnt.transform(info.rotate_matrix);
        cell.layout->paint(pnt);
      } else if (cell.format && info.fill_sharp) {
        const auto sharp       = L'#';
        const auto fmt         = make_text_format(*cell.format);
        const auto glyph_width = fmt.fnt.text_extents(std::wstring{sharp}).width;
        const auto fill_count  = static_cast<std::wstring::size_type>(std::max(info.merged_rect.width / glyph_width, {}));
        ed::rasta::text_layout{}.add(std::wstring(fill_count, sharp), fmt).paint(info.merged_rect.top_left(), pnt);
      } else {
        cell.layout->paint(info.layout_rect.top_left(), pnt);
      }
    }
  }
}


template <typename T>
using type_functor = std::function<void (const area, const area, T&)>;


template <class... Conts>
auto zip_range(Conts&... conts) ->
  decltype(boost::make_iterator_range(
           boost::make_zip_iterator(boost::make_tuple(conts.begin()...)),
           boost::make_zip_iterator(boost::make_tuple(conts.end()...)))) {
  return {boost::make_zip_iterator(boost::make_tuple(conts.begin()...)),
          boost::make_zip_iterator(boost::make_tuple(conts.end()...))};
}


std::pair<double, double> calc_approx(auto beg, auto end) noexcept {
  auto sum_x = 0.;
  auto sum_y = 0.;
  auto sum_x2 = 0.;
  auto sum_xy = 0.;
  auto i = 0u;

  for (; beg < end; ++beg, ++i) {
    sum_x += i;
    sum_y += beg->second;
    sum_x2 += i * i;
    sum_xy += i * beg->second;
  }

  const auto a = (i * sum_xy - (sum_x * sum_y)) / (i * sum_x2 - sum_x * sum_x);
  const auto b = (sum_y - a * sum_x) / i;
  return {a, b};
}


// Для проверки арифметической прогрессии. Используем свойство, что шаг прогрессии на всём массиве должен быть один и тот же.
// Может есть и более быстрые способы.
bool check_progression_step(auto beg, auto end, double d) noexcept {
  for (auto start = std::next(beg, 2); start < end; ++start) {
    if (!ed::fuzzy_equal(start->second - std::prev(start)->second, d)) {
      return false;
    }
  }
  return true;
};


// При работе с числовыми массивами, нужно определить алгоритм.
// Их 2:
// - Арифметическая прогрессия с шагом d. Прогрессия используется для случаев если одно число в range, либо 2 числа.
// - МНК (метод наименьших квадратов). Где следующие значения ячеек получаются из уравнения y = ax + b. Нужно найти a и b.
std::function<void (range&, cell_index)> generate_function_to_set_double(auto beg, auto end) {
  const auto dist = std::distance(beg, end);

  if (dist == 1) {
    return [start = beg->second](range& range, cell_index relative_index) {
      range.set_value(start + relative_index);
    };
  } else if (dist == 2) {
    return [start = beg->second, d = std::next(beg)->second - beg->second](range& range, cell_index relative_index) {
      range.set_value(start + relative_index * d);
    };
  } else {
    ED_ASSERT(dist > 2);
    if (auto d = std::next(beg)->second - beg->second; _::check_progression_step(beg, end, d)) {
      return [start = beg->second, d](range& range, cell_index relative_index) {
        range.set_value(start + relative_index * d);
      };
    } else {
      return [start = beg->second, a_b =_::calc_approx(beg, end)](range& range, cell_index relative_index) {
        range.set_value(a_b.first * relative_index + a_b.second);
      };
    }
  }
}


constexpr auto map_rect(const auto& point_px, const auto text_rotation_px, const auto& rect_px) {
  return ed::rasta::matrix{}.
    translate(point_px).
    rotate(text_rotation_px).
    map(rect_px);
}

}} // namespace _


range::range(worksheet& sheet)
  : sheet_(&sheet)
  , united_({0, 0}, {cell_addr::max_column_count - 1, cell_addr::max_row_count - 1})
  , areas_{united_} {
}


range::range(worksheet& sheet, const area& ar)
  : sheet_(&sheet)
  , united_(ar)
  , areas_{ar} {
}


range::range(worksheet& sheet, area::list&& areas)
  : sheet_(&sheet)
  , areas_{std::move(areas)} {

  ED_ASSERT(!areas_.empty());
  united_ = areas_.front();
  if (areas_.size() > 1) {
    for (auto&& ar : areas_) {
      united_ = united_.unite(ar);
    }
  }

  // Для корректного сравнения
  std::sort(areas_.begin(), areas_.end());
}


bool range::operator==(const range& rhs) const noexcept {
  return sheet_ == rhs.sheet_ && areas_ == rhs.areas_;
}


range::operator std::wstring() const {
  std::wstring a1;

  for (auto& ar : areas_) {
    if (!a1.empty()) {
      a1 += L" ";
    }
    a1 += ar;
  }

  return a1;
}


const worksheet& range::sheet() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }
  return *sheet_;
}


worksheet& range::sheet() {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }
  return *sheet_;
}


bool range::empty() const noexcept {
  return areas_.empty();
}


column_index range::column() const {
  if (areas_.empty()) {
    ED_THROW_EXCEPTION(range_is_empty());
  }
  return areas_.front().left_column();
}


row_index range::row() const {
  if (areas_.empty()) {
    ED_THROW_EXCEPTION(range_is_empty());
  }
  return areas_.front().top_row();
}


cell_addr range::addr() const {
  if (areas_.empty()) {
    ED_THROW_EXCEPTION(range_is_empty());
  }
  return areas_.front().top_left();
}


bool range::single_cell() const noexcept {
  return areas_.size() == 1 && areas_.front().single_cell();
}


bool range::single_merged_cell() const noexcept {
  if (!sheet_) {
    return false;
  }

  if (areas_.size() > 1) {
    return false;
  }

  if (auto m_cell_i = sheet_->find_cell(united_.top_left())) {
    if ((m_cell_i.value()->column_span > 1 || m_cell_i.value()->row_span > 1) &&
        united_.columns_count() == m_cell_i.value()->column_span &&
        united_.rows_count() == m_cell_i.value()->row_span) {
      return true;
    }
  }

  return false;
}


bool range::single_area() const noexcept {
  return areas_.size() == 1;
}


bool range::contains_entire_sheet() const noexcept {
  return contains_entire_rows() && contains_entire_columns();
}


bool range::contains_entire_rows() const noexcept {
  if (areas_.size() == 1) {
    return areas_.front().contains_entire_rows();
  }
  return false;
}


bool range::contains_entire_columns() const noexcept {
  if (areas_.size() == 1) {
    return areas_.front().contains_entire_columns();
  }
  return false;
}


bool range::contains_merged() const noexcept {
  bool contains = false;
  if (!empty()) {
    apply(cell_nodes_visitor_op([&contains](cell_node::it node) {
      if (node->column_span > 1 || node->row_span > 1) {
        contains = true;
        return false;
      }
      return true;
    }));
  }
  return contains;
}


bool range::contains_only_merged() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  for (auto& ar : areas_) {
    if (auto m_cell_i = sheet_->find_cell(ar.top_left())) {
      if (m_cell_i.value()->column_span > 1 || m_cell_i.value()->row_span > 1) {
        if (ar.columns_count() != m_cell_i.value()->column_span ||
            ar.rows_count() != m_cell_i.value()->row_span) {
          return false;
        }
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  return true;
}


bool range::contains_column(column_index index) const noexcept {
  for (auto& ar : areas_) {
    if (ar.contains_column(index)) {
      return true;
    }
  }
  return false;
}


bool range::contains_row(row_index index) const noexcept {
  for (auto& ar : areas_) {
    if (ar.contains_row(index)) {
      return true;
    }
  }
  return false;
}


bool range::contains(cell_addr addr) const noexcept {
  for (auto& ar : areas_) {
    if (ar.contains(addr)) {
      return true;
    }
  }
  return false;
}


bool range::contains(const range& rng) const noexcept {
  if (rng.sheet_ != sheet_ || !sheet_) {
    return false;
  }

  for (auto& rng_ar : rng.areas_) {
    bool cont = false;
    for (auto& ar : areas_) {
      if (ar.contains(rng_ar)) {
        cont = true;
        break;
      }
    }
    if (!cont) {
      return false;
    }
  }
  return true;
}


bool range::intersects(const range& rng) const noexcept {
  if (rng.sheet_ != sheet_ || !sheet_) {
    return false;
  }

  for (auto& rng_ar : rng.areas_) {
    for (auto& ar : areas_) {
      if (ar.intersects(rng_ar)) {
        return true;
      }
    }
  }
  return false;
}


std::optional<cell_value_type> range::value_type() const {
  if (empty()) {
    return std::nullopt;
  } else {
    std::optional<cell_value_type> type;
    std::uint64_t processed = 0;

    apply(cell_nodes_visitor_op([&type, &processed](cell_node::it node) {
      if (!node->merged_with) {
        if (processed == 0) {
          type = node->value_type;
        } else if (type != node->value_type) {
          type = std::nullopt;
          return false;
        }
        ++processed;
      }
      return true;
    }));

    if (type && processed != cells_count()) {
      type = std::nullopt;
    }
    return type;
  }
}


std::uint64_t range::cells_count() const noexcept {
  std::uint64_t count = 0;
  for (auto& ar : areas_) {
    count += ar.cells_count();
  }
  return count;
}


std::size_t range::columns_count() const {
  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return united_.columns_count();
}


std::size_t range::rows_count() const {
  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return united_.rows_count();
}


range range::top_left() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return {*sheet_, united_.top_left()};
}


range range::top_right() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return {*sheet_, united_.top_right()};
}


range range::bottom_left() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return {*sheet_, united_.bottom_left()};
}


range range::bottom_right() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return {*sheet_, united_.bottom_right()};
}


range range::cells(cell_addr rel_first, cell_addr rel_last) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return {*sheet_, area(united_.top_left() + rel_first, united_.top_left() + rel_last)};
}


range range::cells(std::wstring_view a1) const {
  area ar(a1);
  return cells(ar.top_left(), ar.bottom_right());
}


range range::cell(cell_addr rel_addr) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return {*sheet_, united_.top_left() + rel_addr};
}


range range::cell_by_xy(const ed::point_tw<double>& xy) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return {*sheet_, area(cell_addr(column_by_x(xy.x), row_by_y(xy.y)))};
}


range range::offset(int column_offs, int row_offs) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return {*sheet_, united_.offset(column_offs, row_offs)};
}


range range::resize(std::optional<std::size_t> columns, std::optional<std::size_t> rows) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  if (!columns) {
    columns = united_.columns_count();
  }

  if (!rows) {
    rows = united_.rows_count();
  }

  ED_EXPECTS(*columns > 0);
  ED_EXPECTS(*rows > 0);
  auto tl = united_.top_left();

  return {
    *sheet_,
    area(tl, {static_cast<column_index>(tl.column() + *columns - 1), static_cast<row_index>(tl.row() + *rows - 1)})
  };
}


range range::intersect(const range& rng) const {
  if (sheet_ != rng.sheet_) {
    return {};
  }

  if (empty() || rng.empty()) {
    return {};
  }

  if (!single_area() || !rng.single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  if (auto intersect = united_.intersect(rng.united_)) {
    return {*sheet_, *intersect};
  }

  return {};
}


range range::intersect(const ed::rect_tw<double>& rect) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  return {
    *sheet_,
    area({column_by_x(rect.left()), row_by_y(rect.top())}, {column_by_x(rect.right()), row_by_y(rect.bottom())})
  };
}


range range::entire_column(column_index rel_index) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  column_index index = united_.left_column() + rel_index;
  if (united_.contains_column(index)) {
    return {
      *sheet_,
      area({index, united_.top_row()}, {index, united_.bottom_row()})
    };
  }

  return {};
}


range range::entire_row(row_index rel_index) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  row_index index = united_.top_row() + rel_index;
  if (united_.contains_row(index)) {
    return {
      *sheet_,
      area({united_.left_column(), index}, {united_.right_column(), index})
    };
  }

  return {};
}


range range::row_above() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  if (united_.top_row() == 0) {
    return {};
  }

  return resize(std::nullopt, 1).offset(0, -1);
}


range range::row_below() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  if (united_.bottom_row() >= cell_addr::max_row_count - 1) {
    return {};
  }

  return resize(std::nullopt, 1).offset(0, united_.rows_count());
}


range range::column_at_left() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  if (united_.left_column() == 0) {
    return {};
  }

  return resize(1, std::nullopt).offset(-1, 0);
}


range range::column_at_right() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  if (united_.right_column() >= cell_addr::max_column_count - 1) {
    return {};
  }

  return resize(1, std::nullopt).offset(united_.columns_count(), 0);
}


range range::unite(const range& rng) const noexcept {
  if (sheet_ && sheet_ == rng.sheet_) {
    return {*sheet_, united_.unite(rng.united_)};
  }
  return {};
}


range range::united() const noexcept {
  if (sheet_) {
    return {*sheet_, united_};
  }
  return {};
}


range range::join(const range& rng) const {
  if (empty()) {
    return rng;
  }

  if (sheet_ != rng.sheet_) {
    return {};
  }

  area::list result_areas(areas_.begin(), areas_.end());

  for (auto& rng_ar : rng.areas_) {
    area::list spl_areas = {rng_ar};
    for (auto& ar : areas_) {
      if (auto inter = rng_ar.intersect(ar)) {
        area::list new_spl_areas;
        for (auto& spl_ar : spl_areas) {
          if (auto spl_inter = spl_ar.intersect(ar)) {
            auto disjoined = spl_ar.disjoin(*spl_inter);
            new_spl_areas.insert(new_spl_areas.end(), disjoined.begin(), disjoined.end());
          } else {
            new_spl_areas.push_back(spl_ar);
          }
        }
        spl_areas = std::move(new_spl_areas);
      }
    }
    result_areas.insert(result_areas.end(), spl_areas.begin(), spl_areas.end());
  }

  std::size_t i = 0;
  while (i < result_areas.size()) {
    std::size_t j = 0;
    while (j < result_areas.size()) {
      if (i != j) {
        if (auto join = result_areas[i].join(result_areas[j]); join.size() == 1) {
          result_areas[i] = join.front();
          result_areas.erase(result_areas.begin() + j);
          i = 0;
          break;
        }
      }
      ++j;
    }
    ++i;
  }

  return {*sheet_, std::move(result_areas)};
}


range range::merge_area() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  range result = *this;

  apply(cell_nodes_visitor_op([&result, this](cell_node::it node) {
    const cell_node* merged = nullptr;

    if (node->row_span > 1 || node->column_span > 1) {
      merged = &*(node);
    } else if (node->merged_with) {
      if (!node->merged_with_node) {
        node->merged_with_node = &*sheet_->find_cell(*node->merged_with).value();
      }
      merged = node->merged_with_node;
    }

    if (merged) {
      cell_addr tl = merged->index;
      cell_addr br(tl.column() + merged->column_span - 1, tl.row() + merged->row_span - 1);
      result = result.join({*sheet_, area(tl, br)});
    }

    return true;
  }));

  return result;
}


range range::right_trimmed() const {
  if (!sheet_) {
    return {};
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  std::optional<column_index> biggest;
  apply(cell_nodes_visitor_op([&biggest](cell_node::it node) {
    cell_addr addr(node->index);
    if (biggest) {
      biggest = std::max(*biggest, addr.column());
    } else {
      biggest = addr.column();
    }
    return true;
  }));

  if (biggest) {
    return {*sheet_, area(united_.top_left(), {*biggest, united_.bottom_row()})};
  } else {
    return {};
  }
}


range range::bottom_trimmed() const {
  if (!sheet_) {
    return {};
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  std::optional<row_index> biggest;
  apply(cell_nodes_visitor_op([&biggest](cell_node::it node) {
    cell_addr addr(node->index);
    if (biggest) {
      biggest = std::max(*biggest, addr.row());
    } else {
      biggest = addr.row();
    }
    return true;
  }));

  if (biggest) {
    return {*sheet_, area(united_.top_left(), {united_.right_column(), *biggest})};
  } else {
    return {};
  }
}


void range::for_each_area(const area_fn& fn) const {
  ED_ASSERT(fn);

  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  for (auto& ar : areas_) {
    fn(range(*sheet_, ar));
  }
}


void range::for_each_column(const column_fn& fn) const {
  ED_ASSERT(fn);

  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  auto columns = sheet_->book().forest().get<column_node>(sheet_->sheet_node_);
  auto i = std::lower_bound(columns.begin(), columns.end(), column_node{united_.left_column()});
  column_info info;

  for (auto index = united_.left_column(); index <= united_.right_column(); ++index) {
    info.index = index;

    if (i != columns.end() && i->index < index) {
      ++i;
    }

    if (i != columns.end() && i->index == index) {
      info.width = ed::pixels<long long>(i->width);
      info.node = i;
    } else {
      info.width = ed::pixels<long long>(sheet_->default_column_width());
      info.node = std::nullopt;
    }

    fn(info);

    info.left += info.width;
  }
}


void range::for_each_row(const row_fn& fn) const {
  ED_ASSERT(fn);

  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  auto rows = sheet_->book().forest().get<row_node>(sheet_->sheet_node_);
  auto i = std::lower_bound(rows.begin(), rows.end(), row_node{united_.top_row()});
  row_info info;

  for (auto index = united_.top_row(); index <= united_.bottom_row(); ++index) {
    info.index = index;

    if (i != rows.end() && i->index < index) {
      ++i;
    }

    if (i != rows.end() && i->index == index) {
      info.height = ed::pixels<long long>(i->height);
      info.node = i;
    } else {
      info.height = ed::pixels<long long>(sheet_->default_row_height());
      info.node = std::nullopt;
    }

    fn(info);

    info.top += info.height;
  }
}


void range::for_each_cell(const cell_fn& fn) const {
  ED_ASSERT(fn);

  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  for (auto&& ar : areas_) {
    for(auto row = ar.top_row(); row <= ar.bottom_row(); ++row) {
      for (auto col = ar.left_column(); col <= ar.right_column(); ++col) {
        fn(range{*sheet_, area{cell_addr{col, row}}});
      }
    }
  }
}


void range::for_existing_cells(const cell_fn& fn) const {
  ED_ASSERT(fn);

  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  apply(cell_nodes_visitor_op([&fn, this](cell_node::it node) {
    fn(range{*sheet_, area{node->index}});
    return true;
  }));
}


ed::twips<double> range::left() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (united_.left_column() == 0) {
    return 0._tw;
  }
  return sheet_->columns_width(0, united_.left_column() - 1);
}


ed::twips<double> range::top() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (united_.top_row() == 0) {
    return 0._tw;
  }
  return sheet_->rows_height(0, united_.top_row() - 1);
}


ed::twips<double> range::width() const {
  if (sheet_) {
    return sheet_->columns_width(united_.left_column(), united_.right_column());
  }
  return 0._tw;
}


ed::twips<double> range::height() const {
  if (sheet_) {
    return sheet_->rows_height(united_.top_row(), united_.bottom_row());
  }
  return 0._tw;
}


cell_format range::format() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  cell_format cols_fmt;
  apply(get_column_format_op(cols_fmt));

  cell_format rows_fmt;
  apply(get_row_format_op(rows_fmt));

  cell_format cells_fmt;
  apply(get_cell_format_op(cells_fmt));

  cell_format result = cells_fmt.unite(rows_fmt).unite(cols_fmt);
  if (sheet_->node()->format) {
    result = result.unite(*sheet_->node()->format);
  }

  return result;
}


cell_value range::value() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  cell_value val;
  apply(get_cell_value_op(val));
  return val;
}


cell_value::list range::values() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  cell_value::list vals;
  apply(get_cell_values_op(vals));
  return vals;
}


cell_value::matrix range::values_matrix() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  cell_value::matrix vals;
  apply(get_cell_values_matrix_op(vals));
  return vals;
}


std::wstring range::text() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  std::wstring formula;
  apply(get_cell_formula_op(formula));

  if (!formula.empty()) {
    return formula;
  }

  cell_value val;
  apply(get_cell_value_op(val));
  return val.to<std::wstring>();
}


std::wstring range::text_for_edit() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_cell()) {
    ED_THROW_EXCEPTION(too_many_cells_in_range());
  }

  /// Если активна формула, то она отображается с наивысшим приоритетом.
  std::wstring text;
  apply(get_cell_formula_op(text));

  if (!text.empty()) {
    return text;
  }

  /// Если формула не активна, то часть форматов меняет строку для редактирования.
  /// Это поведение полностью повторяет поведение Microsoft Excel/Google Sheet.
  cell_format fmt;
  cell_value cell_val;
  auto& loc = sheet_->book().get_locale();

  apply(get_cell_format_op(fmt));
  apply(get_cell_value_op(cell_val));

  const auto format_error = [](const value_format::result& result) noexcept {
    return result.fill_positions.has_value() && result.text == L"#";
  };

  value_format v_fmt(fmt.get_or_default<number_format>(), loc);
  const auto result = v_fmt(cell_val);

  if (cell_val.type() == cell_value_type::number && !format_error(result)) {
    const auto value = cell_val.to<double>();
    if (v_fmt.has_date_time_section(value)) {
      if (v_fmt.has_date_section(value)) {
        text = value_format(static_cast<int>(value) == value ? L"dd.mm.yyyy" : L"dd.mm.yyyy h:mm:ss")(cell_val).text;
      } else if (v_fmt.has_time_section(value)) {
        double int_part = 0.;
        std::modf(value, &int_part);
        text = value_format(int_part ? L"dd.mm.yyyy h:mm:ss" : L"h:mm:ss")(cell_val).text;
      } else {
        text = value_format(L"dd.mm.yyyy h:mm:ss")(cell_val).text;
      }
    } else if (v_fmt.has_percent_section(value)) {
      text = value_format(number_format::default_value, loc)(value * 100.).text + L"%";
    } else if (v_fmt.has_duration_section(value)) { // Для продолжительности всегда выводятся миллисекунды.
      text = value_format(L"[h]:mm:ss.000")(cell_val).text;
    } else {
      text = value_format(number_format::default_value, loc)(cell_val).text;
    }
  } else {
    text = cell_val.to<std::wstring>();
  }
  return text;
}


range& range::merge() {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    auto area_f = [](const auto& r) {
      const auto merge_with = r.addr().index();
      r.apply(merge_cells_op() |
        clear_cell_value_op({merge_with}) |
        clear_cell_formula_op({merge_with}) |
        clear_cell_format_op({merge_with}));
    };

    for_each_area(area_f);
  }
  tr.commit();

  return *this;
}


range& range::unmerge() {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    for_each_area([](const auto& r) {
      r.apply(unmerge_cells_op());
    });
  }
  tr.commit();

  return *this;
}


range& range::set_format(cell_format::changes&& changes) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    if (contains_entire_sheet()) {
      if (sheet_->node()->format_key) {
        ED_ASSERT(sheet_->node()->format);
        auto new_fmt = sheet_->node()->format->apply(changes);
        if (new_fmt != *sheet_->node()->format) {
          auto format_node = sheet_->book().ensure_format(std::move(new_fmt));
          ED_ENSURES(format_node->format);

          worksheet_node n = *sheet_->node();
          n.format_key = forest_t::key_of(format_node);
          n.format = format_node->format;
          sheet_->book().forest().modify(sheet_->node()) = n;
        }
      } else {
        auto fmt = cell_format().apply(changes);
        auto format_node = sheet_->book().ensure_format(std::move(fmt));
        ED_ENSURES(format_node->format);

        worksheet_node n = *sheet_->node();
        n.format_key = forest_t::key_of(format_node);
        n.format = format_node->format;
        sheet_->book().forest().modify(sheet_->node()) = n;
      }

      apply(change_existing_column_format_op(changes));
      apply(change_existing_row_format_op(changes));
      apply(change_existing_cell_format_op(std::move(changes)));
    } else if (contains_entire_columns()) {
      apply(change_column_format_op(changes));
      apply(change_existing_cell_format_op(changes));

      // Надо создать ячейки при пересечении со строками, имеющими формат
      for (auto& row : sheet_->book().forest().get<row_node>(sheet_->node())) {
        if (row.format_key) {
          ED_ASSERT(row.format);
          ED_ASSERT(areas_.size() == 1);
          auto& ar = areas_.front();
          range row_range(*sheet_, area({ar.left_column(), row.index}, {ar.right_column(), row.index}));
          row_range.apply(change_cell_format_op(changes));
        }
      }
    } else if (contains_entire_rows()) {
      apply(change_row_format_op(changes));
      apply(change_existing_cell_format_op(changes));

      // Надо создать ячейки при пересечении c колонками, имеющими формат
      for (auto& column : sheet_->book().forest().get<column_node>(sheet_->node())) {
        if (column.format_key) {
          ED_ASSERT(column.format);
          ED_ASSERT(areas_.size() == 1);
          auto& ar = areas_.front();
          range column_range(*sheet_, area({column.index, ar.top_row()}, {column.index, ar.bottom_row()}));
          column_range.apply(change_cell_format_op(changes.unite(column.format->as_changes())));
        }
      }
    } else {
      apply(change_cell_format_op(std::move(changes)));
    }
  }
  tr.commit();

  return *this;
}


range& range::clear_format() {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    if (contains_entire_sheet()) {
      if (sheet_->node()->format_key) {
        ED_ASSERT(sheet_->node()->format);
        worksheet_node n = *sheet_->node();
        n.format_key = std::nullopt;
        n.format = nullptr;
        sheet_->book().forest().modify(sheet_->node()) = n;
      }
    }

    if (contains_entire_columns()) {
      apply(clear_column_format_op());
    }

    if (contains_entire_rows()) {
      apply(clear_row_format_op());
    }

    apply(clear_cell_format_op());
  }
  tr.commit();

  return *this;
}


void range::prepare_and_apply_number_format(const number_format::value_type& format) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }
  // 1) Конвертируем ячейки под текущий формат
  // 2) Применяем формат для выбранного диапазона

  cell_fn fn;
  cell_format::changes changes;
  const auto& loc = sheet_->book().get_locale();

  if (format.empty()) { // Атоматический формат.
    fn = [&loc](range range) {
      const auto& v = range.value();
      if (v.type() == cell_value_type::string) {
        auto result = value_parser(loc)(v.as<std::wstring>());
        range.set_value(std::move(result.first));
        if(result.first.is<double>()) {
          range.set_format<text_horizontal_alignment>(horizontal_alignment::right);
        }
      }
    };
    changes.set<number_format>(std::nullopt);
  } else if (format.find(L"@") != std::wstring::npos) { // Текстовый формат.
    fn = [&loc](range range) {
      const auto& v = range.value();
      if (v.type() == cell_value_type::number) {
        auto result = value_format(number_format::default_value, loc)(v.as<double>());
        range.set_value(std::move(result.text));
        range.set_format<text_horizontal_alignment>(horizontal_alignment::left);
      }
    };
    changes.set<number_format>(format);
  } else { // Другие числовые форматы.
    fn = [&loc](range range) {
      const auto& v = range.value();
      if (v.type() == cell_value_type::string) {
        auto p = value_parser(loc)(v.as<std::wstring>());
        if (p.first.is<double>()) {
          range.set_format<text_horizontal_alignment>(horizontal_alignment::right);
          range.set_value(std::move(p.first));
        }
      }
    };
    changes.set<number_format>(format);
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    for_existing_cells(fn);
    set_format(std::move(changes));
  }
  tr.commit();
}


void range::fill(range& out) const {
  using namespace std::placeholders;
  // 1 - Определяем где расположен @out. Сверху/снизу, слева/справа.
  // 2 - Разбиваем на поддиапазоны разных типов и форматов.
  // 3 - Определяем формулы.
  // 4 - Применяем формулы на @out.
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  // В Ms Excel @out должен включать исходный range.
  if (!out.contains(*this)) {
    ED_THROW_EXCEPTION(does_not_contains_range());
  }

  // В Мs Excel работет только для single area.
  if (!single_area() || !out.single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  const auto split_by_columns = [](worksheet& sheet, const area united) {
    std::vector<range> out;
    out.reserve(united.columns_count());
    for (std::size_t col_index = 0; col_index < united.columns_count(); ++col_index) {
      out.push_back(range{sheet, united.entire_column(col_index)}); // private constructor
    }
    return out;
  };

  const auto split_by_rows = [](worksheet& sheet, const area united) {
    std::vector<range> out;
    out.reserve(united.rows_count());
    for (std::size_t row_index = 0; row_index < united.rows_count(); ++row_index) {
      out.push_back(range{sheet, united.entire_row(row_index)});
    }
    return out;
  };

  // Когда растягиваем сверху вниз, слева на право. Нужно:
  // - Взять верхнюю/левую границу.
  // - Добавить стартовый индекс ячейки. Так как ячейки в массиве идут подряд, но в range они могут быть разряжены(между ними могут быть пустые).
  // - Добавить отступ current_range. Поскольку нет смысла перезаписывать уже созданные ячейки. Т.е нужно начать после current_range(indent).
  const auto make_start_index_down_right = [](const auto starting_border, const auto value_index, const auto indent) noexcept {
    return starting_border + value_index + indent;
  };

  // Когда растягиваем снизу вверх, справо налево. Нужно:
  // - Так же взять стартовую границу. Только идём снизу/справа.
  // - Из отступа отнять индекс первой ячейки с конца массива. Так как обход с конца.
  // - Дополнительно отнимаем отсутп indent. Чтобы не перезаписывать current_range.
  const auto make_start_index_up_left = [](const auto starting_border, const auto first_step, const auto indent) noexcept -> std::int64_t  {
    return starting_border + 1 - 2 * indent + first_step; // starting_border + 1 - (indent - first_step) - indent
  };

  const auto downward_direction = [](const area first, const area second) noexcept {
    return first.top_row() == second.top_row() && first.bottom_row() < second.bottom_row();
  };

  const auto upward_direction = [](const area first, const area second) noexcept {
    return first.bottom_row() == second.bottom_row() && first.top_row() > second.top_row();
  };

  const auto left_direction = [](const area first, const area second) noexcept {
    return first.right_column() == second.right_column() && first.left_column() > second.left_column();
  };

  const auto right_direction = [](const area first, const area second) noexcept {
    return first.left_column() == second.left_column() && first.right_column() < second.right_column();
  };

  // Функции использующие дополнительные вычисления.
  _::type_functor<ast_subrange>          ast_func;
  _::type_functor<const double_subrange> double_func;

  // Функции копирующие значения.
  _::type_functor<const double_subrange>    copy_double_func;
  _::type_functor<const rich_text_subrange> rich_text_func;
  _::type_functor<const wstring_subrange>   wstring_func;
  _::type_functor<const bool_subrange>      bool_func;
  _::type_functor<const error_subrange>     error_func;

  const auto apply_generate_func_down_right = [](const area current,
                                                 const area dest,
                                                 auto&      subrange,
                                                 auto&&     generate_start_step_end,
                                                 auto&&     generate_range) {
    auto& [format, values] = subrange;
    const auto generated_function = _::generate_function_to_set_double(values.begin(), values.end());
    for (auto [beg, i] = std::make_pair(values.begin(), 0U); beg < values.end(); ++beg, ++i) {
      auto [start, end, step] = generate_start_step_end(current, dest, beg->first);
      for (auto relative_index = i + values.size(); start <= end; start += step, relative_index += values.size()) {
        auto range = generate_range(current, start);
        if (format) {
          range.set_format(format->as_changes());
        }
        generated_function(range, relative_index);
      }
    }
  };

  const auto apply_simple_func_down_right = [](const area current,
                                               const area dest,
                                               auto&      subrange,
                                               auto&&     generate_start_step_end,
                                               auto&&     generate_range,
                                               auto&&     set_value) {
    auto& [format, values] = subrange;
    for (auto& value : values) {
      for (auto [start, end, step] = generate_start_step_end(current, dest, value.first); start <= end; start += step) {
        auto range = generate_range(current, start);
        if (format) {
          range.set_format(format->as_changes());
        }
        set_value(range, value.second, current);
      }
    }
  };

  const auto apply_generate_func_up_left = [](const area current,
                                              const area dest,
                                              auto&      subrange,
                                              auto&&     generate_start_step_end,
                                              auto&&     generate_range) {
    auto& [format, values] = subrange;
    const auto generated_function = _::generate_function_to_set_double(values.rbegin(), values.rend());
    for (auto [beg, i] = std::make_pair(values.rbegin(), 0U); beg < values.rend(); ++beg, ++i) {
      auto [start, end, step] = generate_start_step_end(current, dest, beg->first);
      for (auto relative_index = i + values.size(); start >= end; start -= step, relative_index += values.size()) {
        auto range = generate_range(current, start);
        if (format) {
          range.set_format(format->as_changes());
        }
        generated_function(range, relative_index);
      }
    }
  };

  const auto apply_simple_func_up_left = [](const area current,
                                            const area dest,
                                            auto&      subrange,
                                            auto&&     generate_start_step_end,
                                            auto&&     generate_range,
                                            auto&&     set_value) {
    auto& [format, values] = subrange;
    for (auto beg = values.rbegin(); beg < values.rend(); ++beg) {
      for (auto [start, end, step] = generate_start_step_end(current, dest, beg->first); start >= end; start -= step) {
        auto range = generate_range(current, start);
        if (format) {
          range.set_format(format->as_changes());
        }
        set_value(range, beg->second, current);
      }
    }
  };

  const auto apply_value_to_cell = [](auto& range, auto& value, const auto) {
    range.set_value(value);
  };

  if (downward_direction(united_, out.united_)) {
    const auto generate_starting_index = [make_start_index_down_right](const area current, const area dest, const auto index) noexcept {
      return std::make_tuple(make_start_index_down_right(dest.top_row(), index, current.rows_count()), dest.bottom_row(), current.rows_count());
    };

    const auto generate_range = [this](const area current, const auto row) {
      return range{*sheet_, area{cell_addr{static_cast<column_index>(current.left_column()), static_cast<row_index>(row)}}};
    };

    const auto apply_formula_to_cell = [](auto& range, auto& value, const auto current) {
      for (auto& tok : value) {
        if (auto* ref = std::get_if<fx::ast::reference>(&tok); ref && !ref->row_abs) {
          ref->addr = cell_addr{ref->addr.column(), static_cast<row_index>(ref->addr.row() + current.rows_count())};
        }
      }
      range.set_text(L"=" + fx::ast_to_formula(value));
    };

    double_func      = std::bind(apply_generate_func_down_right, _1, _2, _3, generate_starting_index, generate_range);
    wstring_func     = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    copy_double_func = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    rich_text_func   = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    bool_func        = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    error_func       = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    ast_func         = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_formula_to_cell);

  } else if (upward_direction(united_, out.united_)) {
    const auto generate_starting_index = [make_start_index_up_left](const area current, const area dest, const auto index) noexcept {
      return std::make_tuple(make_start_index_up_left(dest.bottom_row(), index, current.rows_count()), dest.top_row(), current.rows_count());
    };

    const auto generate_range = [this](const area current, const auto row) {
      return range{*sheet_, area{cell_addr{static_cast<column_index>(current.left_column()), static_cast<row_index>(row)}}};
    };

    const auto apply_formula_to_cell = [](auto& range, auto& value, const auto current) {
      for (auto& tok : value) {
        if (auto* ref = std::get_if<fx::ast::reference>(&tok); ref && !ref->row_abs) {
          ref->addr = cell_addr{ref->addr.column(), static_cast<row_index>(ref->addr.row() - current.rows_count())};
        }
      }
      range.set_text(L"=" + fx::ast_to_formula(value));
    };

    double_func      = std::bind(apply_generate_func_up_left, _1, _2, _3, generate_starting_index, generate_range);
    wstring_func     = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    copy_double_func = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    rich_text_func   = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    bool_func        = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    error_func       = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    ast_func         = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_formula_to_cell);

  } else if (right_direction(united_, out.united_)) {
    const auto generate_starting_index = [make_start_index_down_right](const area current, const area dest, const auto index) noexcept {
      return std::make_tuple(make_start_index_down_right(dest.left_column(), index, current.columns_count()), dest.right_column(), current.columns_count());
    };

    const auto generate_range = [this](const area current, const auto col) {
      return range{*sheet_, area{cell_addr{static_cast<column_index>(col), static_cast<row_index>(current.top_row())}}};
    };

    const auto apply_formula_to_cell = [](auto& range, auto& value, const auto current) {
      for (auto& tok : value) {
        if (auto* ref = std::get_if<fx::ast::reference>(&tok); ref && !ref->col_abs) {
          ref->addr = cell_addr{static_cast<column_index>(ref->addr.column() + current.columns_count()), ref->addr.row()};
        }
      }
      range.set_text(L"=" + fx::ast_to_formula(value));
    };

    double_func      = std::bind(apply_generate_func_down_right, _1, _2, _3, generate_starting_index, generate_range);
    wstring_func     = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    copy_double_func = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    rich_text_func   = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    bool_func        = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    error_func       = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    ast_func         = std::bind(apply_simple_func_down_right, _1, _2, _3, generate_starting_index, generate_range, apply_formula_to_cell);

  } else if (left_direction(united_, out.united_)) {
    const auto generate_starting_index = [make_start_index_up_left](const area current, const area dest, const auto index) noexcept {
      return std::make_tuple(make_start_index_up_left(dest.right_column(), index, current.columns_count()), dest.left_column(), current.columns_count());
    };

    const auto generate_range = [this](const area current, const auto col) {
      return range{*sheet_, area{cell_addr{static_cast<column_index>(col), static_cast<row_index>(current.top_row())}}};
    };

    const auto apply_formula_to_cell = [](auto& range, auto& value, const auto current) {
      for (auto& tok : value) {
        if (auto* ref = std::get_if<fx::ast::reference>(&tok); ref && !ref->col_abs) {
          ref->addr = cell_addr{static_cast<column_index>(ref->addr.column() - current.columns_count()), ref->addr.row()};
        }
      }
      range.set_text(L"=" + fx::ast_to_formula(value));
    };

    double_func      = std::bind(apply_generate_func_up_left, _1, _2, _3, generate_starting_index, generate_range);
    wstring_func     = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    copy_double_func = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    rich_text_func   = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    bool_func        = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    error_func       = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_value_to_cell);
    ast_func         = std::bind(apply_simple_func_up_left, _1, _2, _3, generate_starting_index, generate_range, apply_formula_to_cell);
  }

  std::vector<range> current_ranges;
  std::vector<range> dest_ranges;
  // Если dest слева или справа. То делим на строки. Если dest снизу/сверху, то на столбцы.
  if (out.united_.left_column() == united_.left_column() && out.united_.right_column() == united_.right_column()) {
    current_ranges = split_by_columns(*sheet_, united_);
    dest_ranges    = split_by_columns(*sheet_, out.united_);
  } else if (out.united_.top_row() == united_.top_row() && out.united_.bottom_row() == united_.bottom_row()) {
    current_ranges = split_by_rows(*sheet_, united_);
    dest_ranges    = split_by_rows(*sheet_, out.united_);
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    for(auto&& p_ranges : _::zip_range(current_ranges, dest_ranges)) {
      value_ranges value_ranges;
      p_ranges.get<0>().apply(split_by_format_and_type(value_ranges));
      if (!value_ranges.empty()) {
        // Если в выбранном диапазоне только 1 массив с числами и он состоит из 1 элемента, то для этого числового массива не применяются формулы.
        if (const auto* double_ranges = std::get_if<double_subrange>(&value_ranges.front()); value_ranges.size() == 1 && double_ranges && double_ranges->second.size() == 1) {
          copy_double_func(p_ranges.get<0>().united_, p_ranges.get<1>().united_, *double_ranges);
        } else {
          for (auto&& variant_ranges : value_ranges) {
            if (const auto* double_ranges = std::get_if<double_subrange>(&variant_ranges)) {
              double_func(p_ranges.get<0>().united_, p_ranges.get<1>().united_, *double_ranges);
            } else if (const auto* wstring_ranges = std::get_if<wstring_subrange>(&variant_ranges)) {
              wstring_func(p_ranges.get<0>().united_, p_ranges.get<1>().united_, *wstring_ranges);
            } else if (const auto* rich_text_ranges = std::get_if<rich_text_subrange>(&variant_ranges)) {
              rich_text_func(p_ranges.get<0>().united_, p_ranges.get<1>().united_, *rich_text_ranges);
            } else if (auto* formula_ranges = std::get_if<ast_subrange>(&variant_ranges)) {
              ast_func(p_ranges.get<0>().united_, p_ranges.get<1>().united_, *formula_ranges);
            } else if (auto* bool_ranges = std::get_if<bool_subrange>(&variant_ranges)) {
              bool_func(p_ranges.get<0>().united_, p_ranges.get<1>().united_, *bool_ranges);
            } else if (auto* error_ranges = std::get_if<error_subrange>(&variant_ranges)) {
              error_func(p_ranges.get<0>().united_, p_ranges.get<1>().united_, *error_ranges);
            }
          }
        }
      }
    }
  }
  tr.commit();
}


range& range::update_column_width() {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    apply(change_column_width{});
  }
  tr.commit();

  return *this;
}


range& range::set_default_row_height() {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    apply(set_up_default_row_height());
  }
  tr.commit();

  return *this;
}


range& range::set_all_borders(const border& br) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    for (auto& ar : areas_) {
      range rng(*sheet_, ar);
      rng.set_format<top_border, bottom_border, left_border, right_border>(br, br, br, br);

      if (range above = rng.row_above(); !above.empty()) {
        above.set_format<bottom_border>(std::nullopt);
      }
      if (range below = rng.row_below(); !below.empty()) {
        below.set_format<top_border>(std::nullopt);
      }
      if (range left = rng.column_at_left(); !left.empty()) {
        left.set_format<right_border>(std::nullopt);
      }
      if (range right = rng.column_at_right(); !right.empty()) {
        right.set_format<left_border>(std::nullopt);
      }
    }
  }
  tr.commit();

  return *this;
}


range& range::set_inner_borders(const border& br) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    set_horizontal_borders(br);
    set_vertical_borders(br);
  }
  tr.commit();

  return *this;
}


range& range::set_horizontal_borders(const border& br) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    if (!single_merged_cell()) {
      for (auto& ar : areas_) {
        range rng = range(*sheet_, ar).resize(std::nullopt, 1);
        for (row_index row = ar.top_row(); row < ar.bottom_row(); ++row) {
          rng.set_format<bottom_border>(br);
          rng = rng.offset(0, 1);
        }
      }
    }
  }
  tr.commit();

  return *this;
}


range& range::set_vertical_borders(const border& br) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    if (!single_merged_cell()) {
      for (auto& ar : areas_) {
        range rng = range(*sheet_, ar).resize(1, std::nullopt);
        for (column_index col = ar.left_column(); col < ar.right_column(); ++col) {
          rng.set_format<right_border>(br);
          rng = rng.offset(1, 0);
        }
      }
    }
  }
  tr.commit();

  return *this;
}


range& range::set_outer_borders(const border& br) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    if (single_merged_cell()) {
      set_all_borders(br);
    } else {
      for (auto& ar : areas_) {
        range rng(*sheet_, ar);
        rng.resize(std::nullopt, 1).set_format<top_border>(br);
        rng.resize(std::nullopt, 1).offset(0, ar.rows_count() - 1).set_format<bottom_border>(br);
        rng.resize(1, std::nullopt).set_format<left_border>(br);
        rng.resize(1, std::nullopt).offset(ar.columns_count() - 1, 0).set_format<right_border>(br);

        if (range above = rng.row_above(); !above.empty()) {
          above.set_format<bottom_border>(std::nullopt);
        }
        if (range below = rng.row_below(); !below.empty()) {
          below.set_format<top_border>(std::nullopt);
        }
        if (range left = rng.column_at_left(); !left.empty()) {
          left.set_format<right_border>(std::nullopt);
        }
        if (range right = rng.column_at_right(); !right.empty()) {
          right.set_format<left_border>(std::nullopt);
        }
      }
    }
  }
  tr.commit();

  return *this;
}


range& range::set_top_borders(const border& br) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    for (auto& ar : areas_) {
      range rng(*sheet_, ar);
      rng.resize(std::nullopt, 1).set_format<top_border>(br);

      if (range above = rng.row_above(); !above.empty()) {
        above.set_format<bottom_border>(std::nullopt);
      }
    }
  }
  tr.commit();

  return *this;
}


range& range::set_bottom_borders(const border& br) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    for (auto& ar : areas_) {
      range rng(*sheet_, ar);

      if (rng.single_merged_cell()) {
        rng.set_format<bottom_border>(br);
      } else {
        rng.resize(std::nullopt, 1).offset(0, ar.rows_count() - 1).set_format<bottom_border>(br);
      }

      if (range below = rng.row_below(); !below.empty()) {
        below.set_format<top_border>(std::nullopt);
      }
    }
  }
  tr.commit();

  return *this;
}


range& range::set_left_borders(const border& br) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    for (auto& ar : areas_) {
      range rng(*sheet_, ar);
      rng.resize(1, std::nullopt).set_format<left_border>(br);

      if (range right = rng.column_at_right(); !right.empty()) {
        right.set_format<left_border>(std::nullopt);
      }
    }
  }
  tr.commit();

  return *this;
}


range& range::set_right_borders(const border& br) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
      for (auto& ar : areas_) {
        range rng(*sheet_, ar);

        if (rng.single_merged_cell()) {
          rng.set_format<right_border>(br);
        } else {
          rng.resize(1, std::nullopt).offset(ar.columns_count() - 1, 0).set_format<right_border>(br);
        }

        if (range right = rng.column_at_right(); !right.empty()) {
          right.set_format<left_border>(std::nullopt);
        }
      }
  }
  tr.commit();

  return *this;
}


range& range::clear_all_borders() {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    for (auto& ar : areas_) {
      range rng(*sheet_, ar);
      rng.set_format<top_border, bottom_border, left_border, right_border>(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt);

      if (range above = rng.row_above(); !above.empty()) {
        above.set_format<bottom_border>(std::nullopt);
      }
      if (range below = rng.row_below(); !below.empty()) {
        below.set_format<top_border>(std::nullopt);
      }
      if (range left = rng.column_at_left(); !left.empty()) {
        left.set_format<right_border>(std::nullopt);
      }
      if (range right = rng.column_at_right(); !right.empty()) {
        right.set_format<left_border>(std::nullopt);
      }
    }
  }
  tr.commit();

  return *this;
}


range& range::set_column_width(ed::twips<double> val) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    apply(set_column_width_op(val));
  }
  tr.commit();

  return *this;
}


range& range::set_row_height(ed::twips<double> val) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    apply(set_row_height_op(val));
  }
  tr.commit();

  return *this;
}


range& range::set_value(const cell_value& val) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    if (val.is_none()) {
      apply(clear_cell_value_op() | clear_cell_formula_op());
    } else {
      apply(set_cell_value_op(val) | clear_cell_formula_op());
    }
  }
  tr.commit();

  return *this;
}


range& range::set_text(const std::wstring& val) {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  scoped_transaction tr(sheet_->book().forest());
  {
    if (val.empty()) {
      apply(clear_cell_value_op() | clear_cell_formula_op());
    } else if (const auto v_format = format(); v_format.get_or_default<number_format>().find(L"@") != std::wstring::npos) {  // Если выбран текстовый формат.
      apply(set_cell_value_op(val) | clear_cell_formula_op());
    } else if (cell_format::changes changes; val.size() > 1 && val.front() == L'=') {
      apply(clear_cell_value_op() | set_cell_formula_op(val));
      if (fx::ast::tokens tokens; sheet_->book().formula_parser().parse_no_throw(val, tokens)) {
        value_format formatter(v_format.get_or_default<number_format>(), sheet_->book_.get_locale());
        // Если выставлен формат "Число", то не применяется формат от функций.
        if (auto format = fx::get_func_format(tokens); !format.empty() && !formatter.has_number_section(0.)) {
          apply(change_cell_format_op(changes.set<number_format>(std::move(format))));
        }
      }
    } else {
      const auto& loc = sheet_->book().get_locale();
      value_parser parser(loc);
      value_format formatter(v_format.get_or_default<number_format>(), loc);

      // Если был выставлен формат дробный, то с приоритетом ищется дробь, а не дата.
      auto value_with_format = parser(val, formatter.has_fraction_section(0.));
      // Если выставлен формат "Число", "Дата/время", то формат меняется только вручную.
      if (!value_with_format.second.empty() && !formatter.has_number_section(0.) && !formatter.has_date_time_section(0.)) {
        changes.set<number_format>(std::move(value_with_format.second));
      }
      apply(set_cell_value_op(std::move(value_with_format.first)) | change_cell_format_op(std::move(changes)) | clear_cell_formula_op());
    }
  }
  tr.commit();

  return *this;
}


void range::select() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  sheet_->selection_ = *this;
  sheet_->active_cell_ = range(*sheet_, areas_.back().top_left());
}


void range::activate() const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  range active(*sheet_, areas_.back().top_left());
  ED_EXPECTS(sheet_->selection_->contains(active));
  sheet_->active_cell_ = active;
}


void range::paint(ed::rasta::painter& pnt, const paint_options& opts) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  thread_local _::cell_info::matrix matrix;
  matrix.clear();
  matrix.resize(united_.rows_count(), united_.columns_count());

  auto rows = sheet_->book().forest().get<row_node>(sheet_->sheet_node_);
  auto columns = sheet_->book().forest().get<column_node>(sheet_->sheet_node_);
  auto row_it = std::lower_bound(rows.begin(), rows.end(), row_node{united_.top_row()});
  auto first_col_it = std::lower_bound(columns.begin(), columns.end(), column_node{united_.left_column()});

  ed::rect_px<long long> rect;
  std::size_t matrix_row = 0;
  std::size_t matrix_col = 0;

  for (auto row = united_.top_row(); row <= united_.bottom_row(); ++row) {
    if (row_it != rows.end() && row_it->index == row) { // Не пустая строка
      rect.height = row_it->height;
      matrix_col = 0;
      const _::cell_info* left_non_overlapping = nullptr;

      auto col_it = first_col_it;
      auto cells = sheet_->book().forest().get<cell_node>(row_it);
      auto cell_it = std::lower_bound(cells.begin(), cells.end(), cell_node{cell_addr(united_.left_column(), row).index()});

      for (auto col = united_.left_column(); col <= united_.right_column(); ++col) {
        cell_addr addr(col, row);
        auto& info = matrix[matrix_row][matrix_col];
        info.matrix_row = matrix_row;
        info.matrix_col = matrix_col;
        info.format = sheet_->node()->format;

        if (col_it != columns.end() && col_it->index == col) {
          rect.width = col_it->width;
          if (col_it->format) {
            info.format = col_it->format;
          }
          ++col_it;
        } else {
          rect.width = sheet_->default_column_width();
        }

        if (row_it->format) {
          info.format = row_it->format;
        }

        if (cell_it != cells.end() && cell_it->index == addr.index()) { // Не пустая ячейка
          // Для типа горизонтального выравнивания general нужно получить тип результата формулы. Поскольку результат формулы тоже должен быть выровнен.
          // Например: Если результат формулы bool. То он должен, по аналогии с Ms Excel, рисоваться по центру.
          const auto formula_node = sheet_->book().forest().get<cell_formula_node>(cell_it);
          if (!formula_node.empty()) {
            ED_ASSERT(formula_node.size() == 1);
            info.formula_type = formula_node.front().result.type();
          }

          const cell_node& cell = *cell_it;
          info.node = cell_it;
          info.rect = rect;
          info.merged_rect = rect;
          info.left_non_overlapping = left_non_overlapping;

          if (cell.merged_with) {
            if (!cell.merged_with_node) {
              cell_addr merged_addr(*cell.merged_with);
              if (united_.contains(merged_addr)) {
                cell.merged_with_node = &*matrix
                  [merged_addr.row() - united_.top_row()]
                  [merged_addr.column() - united_.left_column()].node.value();
              } else { // Объединяющая ячейка может находиться за пределами области
                cell.merged_with_node = &*sheet_->find_cell(merged_addr).value();
              }
            }
            info.merged_with = cell.merged_with_node;
            if (info.merged_with->format) {
              info.format = info.merged_with->format;
            }
          } else {
            if (cell.format) {
              info.format = cell.format;
            }

            if (cell.column_span > 1 || cell.row_span > 1) {
              info.merged_with = &cell;
            }

            if (cell.column_span > 1) {
              auto sp_col_it = col_it;
              for (column_index sp_col = col + 1; sp_col < col + cell.column_span; ++sp_col) {
                if (sp_col_it != columns.end() && sp_col_it->index == sp_col) {
                  info.merged_rect.width += sp_col_it->width;
                  ++sp_col_it;
                } else {
                  info.merged_rect.width += sheet_->default_column_width();
                }
              }
            }

            if (cell.row_span > 1) {
              auto sp_row_it = row_it + 1;
              for (row_index sp_row = row + 1; sp_row < row + cell.row_span; ++sp_row) {
                if (sp_row_it != rows.end() && sp_row_it->index == sp_row) {
                  info.merged_rect.height += sp_row_it->height;
                  ++sp_row_it;
                } else {
                  info.merged_rect.height += sheet_->default_row_height();
                }
              }
            }
          }

          if (cell.layout) {
            ED_ASSERT(!cell.merged_with);

            auto h_alignment = text_horizontal_alignment::default_value;
            auto v_alignment = text_vertical_alignment::default_value;
            auto t_rotation  = text_rotation::default_value;

            if (cell.format) {
              h_alignment = cell.format->get_or_default<text_horizontal_alignment>();
              v_alignment = cell.format->get_or_default<text_vertical_alignment>();
              t_rotation  = cell.format->get_or_default<text_rotation>();
            }

            if (t_rotation != text_rotation::default_value) { // Если задана ориентация.
              if (t_rotation > 0._deg) { // Поворот по часовой стрелке.
                const auto rect = _::map_rect(info.merged_rect.bottom_right(), t_rotation,
                  ed::rect_px<long long>{ed::point_px<long long>{}, ed::size_px<long long>{cell.layout->width(), cell.layout->height()}});
                info.layout_rect.x = info.merged_rect.right();
                info.layout_rect.y = info.merged_rect.top();
                info.layout_rect.width  = rect.width;
                info.layout_rect.height = rect.height;

                info.layout_rect.x -= rect.width;
                if (h_alignment == horizontal_alignment::left && rect.width < info.merged_rect.width) {
                  info.layout_rect.x = info.merged_rect.left();
                } else if (h_alignment == horizontal_alignment::center && rect.width < info.merged_rect.width) {
                  info.layout_rect.x -= (info.merged_rect.width - rect.width) / 2;
                }

                if (v_alignment == vertical_alignment::bottom && rect.height < info.merged_rect.height) {
                  info.layout_rect.y = info.merged_rect.bottom() - rect.height;
                } else if (v_alignment == vertical_alignment::center && rect.height < info.merged_rect.height) {
                  info.layout_rect.y += (info.merged_rect.height - rect.height) / 2;
                }

                info.rotate_matrix.translate(info.layout_rect.x + (info.merged_rect.right() - rect.left()), info.layout_rect.y);
                info.rotate_matrix.rotate(t_rotation);
              } else { // Против часовой стрелки.
                const auto rect = _::map_rect(info.merged_rect.bottom_left(), t_rotation,
                  ed::rect_px<long long>{ed::point_px<long long>{}, ed::size_px<long long>{cell.layout->width(), cell.layout->height()}});
                info.layout_rect.x      = info.merged_rect.left();
                info.layout_rect.y      = info.merged_rect.bottom();
                info.layout_rect.width  = rect.width;
                info.layout_rect.height = rect.height;

                info.layout_rect.y -= rect.bottom() - info.merged_rect.bottom();
                if (v_alignment == vertical_alignment::center && rect.height < info.merged_rect.height) {
                  info.layout_rect.y -= (info.merged_rect.height - rect.height) / 2;
                } else if (v_alignment == vertical_alignment::top && rect.height < info.merged_rect.height) {
                  info.layout_rect.y = info.merged_rect.top() + rect.height - (rect.bottom() - info.merged_rect.bottom());
                }

                // При повороте нужно дополнительно учесть, что размер квадрата может быть меньше, чем размер повёрнутого текста.
                if (h_alignment == horizontal_alignment::center && rect.width < info.merged_rect.width) { // По умолчанию слева.
                  info.layout_rect.x += (info.merged_rect.width - rect.width) / 2;
                } else if (h_alignment == horizontal_alignment::right && rect.width < info.merged_rect.width) {
                  info.layout_rect.x = info.merged_rect.right() - rect.width;
                }
                info.rotate_matrix.translate(info.layout_rect.top_left());
                info.rotate_matrix.rotate(t_rotation);
              }
            } else { // Если нет ориентации, то используется general_horizontal_alignment.
              info.layout_rect.top_left(info.merged_rect.top_left());
              info.layout_rect.width = cell.layout->width();
              info.layout_rect.height = cell.layout->height();

              const auto general_hor_alignment_right = h_alignment == horizontal_alignment::general && info.formula_type.value_or(cell.value_type) == cell_value_type::number;
              const auto general_hor_alignment_center = h_alignment == horizontal_alignment::general &&
                (info.formula_type.value_or(cell.value_type) == cell_value_type::error || info.formula_type.value_or(cell.value_type) == cell_value_type::boolean);

              // Выравнивание по правому краю:
              // - Если задано вручную.
              // - Выставлен тип general и тип данных double.
              // Выравнивание по левому краю:
              // - Если задано вручную.
              // - Выставлен тип general и тип данных string/rich_text.
              // Выравнивание по центру:
              // - Если задано вручную.
              // - Выставлен тип general и тип данных bool, ошибка.
              // По умолчанию стоит выравнивание по левому краю.
              // todo: Осталось 3 типа: distibuted, fill, justify.
              if (general_hor_alignment_right || h_alignment == horizontal_alignment::right) {
                info.layout_rect.x = info.merged_rect.right() - cell.layout->width() - 2_px;
              } else if (general_hor_alignment_center || h_alignment == horizontal_alignment::center) {
                info.layout_rect.x += (info.merged_rect.width - cell.layout->width()) / 2;
              }
              if (v_alignment == vertical_alignment::bottom) {
                info.layout_rect.y = info.merged_rect.bottom() - cell.layout->height() - 2_px;
              } else if (v_alignment == vertical_alignment::center) {
                info.layout_rect.y += (info.merged_rect.height - cell.layout->height()) / 2;
              }
            }

            // Заполняем #, если тип значения не std::wstring/rich_text и layout не помещается в ячейку.
            info.fill_sharp = info.layout_rect.width > info.merged_rect.width &&
              cell.value_type != cell_value_type::string                      &&
              cell.value_type != cell_value_type::rich_text;
          }
          if (cell.layout || cell.merged_with) {
            if (matrix_col > 0) {
              auto row_ref = matrix[matrix_row];
              for (int c = matrix_col - 1; c >= 0; --c) {
                if (!row_ref[c].right_non_overlapping) {
                  row_ref[c].right_non_overlapping = &info;
                } else {
                  break;
                }
              }
            }
            left_non_overlapping = &info;
          }

          ++cell_it;
        } else { // Пустая ячейка
          info.left_non_overlapping = left_non_overlapping;
          info.rect = rect;
          info.merged_rect = rect;
        }

        rect.x += rect.width;
        ++matrix_col;
      }

      ++row_it;
    } else { // Пустая строка
      rect.height = sheet_->default_row_height();
      auto col_it = first_col_it;
      matrix_col = 0;

      for (auto col = united_.left_column(); col <= united_.right_column(); ++col) {
        auto& info = matrix[matrix_row][matrix_col];
        info.matrix_row = matrix_row;
        info.matrix_col = matrix_col;

        if (col_it != columns.end() && col_it->index == col) {
          rect.width = col_it->width;
          info.format = col_it->format;
          ++col_it;
        } else {
          info.format = sheet_->node()->format;
          rect.width = sheet_->default_column_width();
        }

        info.rect = rect;
        info.merged_rect = rect;

        rect.x += rect.width;
        ++matrix_col;
      }
    }

    rect.x = 0._tw;
    rect.y += rect.height;
    ++matrix_row;
  }

  for (auto& info : matrix) {
    if (info.merged_with) {
      ED_ASSERT(info.node);

      cell_addr addr(info.node.value()->index);
      cell_addr merged_addr(info.merged_with->index);

      if (addr.row() != merged_addr.row()) {
        info.draw_top_border = false;
      }
      if (addr.row() != merged_addr.row() + info.merged_with->row_span - 1) {
        info.draw_bottom_border = false;
      }
      if (addr.column() != merged_addr.column()) {
        info.draw_left_border = false;
      }
      if (addr.column() != merged_addr.column() + info.merged_with->column_span - 1) {
        info.draw_right_border = false;
      }
    } else {
      if (info.node && info.node.value()->layout) { // Не пустая ячейка
        if (info.node.value()->row_span == 1 && info.node.value()->column_span == 1) {
          if (info.layout_rect.left() < info.rect.left()) { // Слева не помещается в ячейку
            if (!info.left_non_overlapping || info.left_non_overlapping->matrix_col < info.matrix_col - 1) { // Ячейка слева может перекрываться
              info.draw_left_border = false;
            }
          }
          if (info.layout_rect.right() > info.rect.right()) { // Справа не помещается в ячейку
            if (!info.right_non_overlapping || info.right_non_overlapping->matrix_col > info.matrix_col + 1) { // Ячейка справа может перекрываться
              info.draw_right_border = false;
            }
          }
        }
      } else { // Пустая ячейка
        if (info.left_non_overlapping && !info.left_non_overlapping->merged_with) {
          if (info.left_non_overlapping->node.value()->layout) {
            if (info.left_non_overlapping->layout_rect.right() > info.rect.left()) {
              info.draw_left_border = false;
            }
            if (info.left_non_overlapping->layout_rect.right() > info.rect.right()) {
              info.draw_right_border = false;
            }
          }
        }
        if (info.right_non_overlapping && !info.right_non_overlapping->merged_with) {
          if (info.right_non_overlapping->node.value()->layout) {
            if (info.right_non_overlapping->layout_rect.left() < info.rect.left()) {
              info.draw_left_border = false;
            }
            if (info.right_non_overlapping->layout_rect.left() < info.rect.right()) {
              info.draw_right_border = false;
            }
          }
        }
      }
    }

    if (info.draw_top_border) {
      if (const _::cell_info* above = info.matrix_row > 0 ? &matrix[info.matrix_row - 1][info.matrix_col] : nullptr) {
        if (!_::is_more_power<top_border, bottom_border>(info.format, above->format)) {
          info.draw_top_border = false;
        }
      }
    }
    if (info.draw_bottom_border) {
      if (const _::cell_info* below = info.matrix_row < matrix.rows_count() - 1 ? &matrix[info.matrix_row + 1][info.matrix_col] : nullptr) {
        if (!_::is_more_power_eq<bottom_border, top_border>(info.format, below->format)) {
          info.draw_bottom_border = false;
        }
      }
    }
    if (info.draw_left_border) {
      if (const _::cell_info* left = info.matrix_col > 0 ? &matrix[info.matrix_row][info.matrix_col - 1] : nullptr) {
        if (!_::is_more_power<left_border, right_border>(info.format, left->format)) {
          info.draw_left_border = false;
        }
      }
    }
    if (info.draw_right_border) {
      if (const _::cell_info* right = info.matrix_col < matrix.columns_count() - 1 ? &matrix[info.matrix_row][info.matrix_col + 1] : nullptr) {
        if (!_::is_more_power_eq<right_border, left_border>(info.format, right->format)) {
          info.draw_right_border = false;
        }
      }
    }
  }

  for (auto& info : matrix) {
    if (!info.rect.empty()) {
      _::draw_grid_lines(info, opts, pnt);
    }
  }

  for (auto& info : matrix) {
    if (info.node && opts.hidden_cell == info.node.value()->index) {
      continue;
    }
    if (!info.rect.empty()) {
      _::draw_cell_content(info, pnt);
    }
  }

  for (auto& info : matrix) {
    if (info.node && opts.hidden_cell == info.node.value()->index) {
      continue;
    }
    if (!info.rect.empty()) {
      _::draw_border(info, pnt);
    }
  }
}


ed::buffer range::copy(const ed::mime_type& format) const {
  if (!sheet_) {
    ED_THROW_EXCEPTION(range_is_empty());
  }

  if (!single_area()) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }

  auto cbw_it = sheet_->book().clipboard_writers_.find(format);
  if (cbw_it == sheet_->book().clipboard_writers_.end()) {
    ED_THROW_EXCEPTION(unsupported_clipboard_format());
  }

  ed::buffer data;
  boost::iostreams::stream<boost::iostreams::back_insert_device<ed::buffer>> os(data);
  cbw_it->second(os, *this);
  os.flush();

  return data;
}


column_index range::column_by_x(ed::twips<double> x) const noexcept {
  ED_ASSERT(sheet_);

  auto lower = united_.left_column();
  auto upper = united_.right_column();

  if (x <= 0._tw) {
    return lower;
  }

  auto columns = sheet_->book().forest().get<column_node>(sheet_->sheet_node_);
  auto i = std::lower_bound(columns.begin(), columns.end(), column_node{lower});

  if (i == columns.end()) {
    return lower + static_cast<column_index>(
      ed::pixels<long long>(x).value() / ed::pixels<long long>(sheet_->default_column_width()).value());
  } else {
    // TODO: оптимизировать
    ed::pixels<long long> offs = 0_px;
    for (auto index = lower; index <= upper; ++index) {
      if (i != columns.end() && i->index == index) {
        offs += i->width;
        ++i;
      } else {
        offs += sheet_->default_column_width();
      }
      if (x < offs) {
        return index;
      }
    }
    return upper;
  }
}


row_index range::row_by_y(ed::twips<double> y) const noexcept {
  ED_ASSERT(sheet_);

  auto lower = united_.top_row();
  auto upper = united_.bottom_row();

  if (y < 0._tw) {
    return lower;
  }

  auto rows = sheet_->book().forest().get<row_node>(sheet_->sheet_node_);
  auto i = std::lower_bound(rows.begin(), rows.end(), row_node{lower});

  if (i == rows.end()) {
    return lower + static_cast<row_index>(
      ed::pixels<long long>(y).value() / ed::pixels<long long>(sheet_->default_row_height()).value());
  } else {
    // TODO: оптимизировать
    ed::pixels<long long> offs = 0_px;
    for (auto index = lower; index <= upper; ++index) {
      if (i != rows.end() && i->index == index) {
        offs += i->height;
        ++i;
      } else {
        offs += sheet_->default_row_height();
      }
      if (y < offs) {
        return index;
      }
    }
    return upper;
  }
}


} // namespace lde::cellfy::boox
