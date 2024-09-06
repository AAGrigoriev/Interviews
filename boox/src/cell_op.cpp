#include <lde/cellfy/boox/src/cell_op.h>

#include <algorithm>
#include <type_traits>

#include <ed/core/aggregate_equal.h>
#include <ed/core/assert.h>
#include <ed/core/exception.h>
#include <ed/core/math.h>
#include <ed/core/quantity.h>
#include <ed/core/scoped_assign.h>

#include <ed/rasta/dpi.h>
#include <ed/rasta/physical_font.h>
#include <ed/rasta/text_layout.h>

#include <lde/cellfy/boox/exception.h>
#include <lde/cellfy/boox/format.h>
#include <lde/cellfy/boox/fx_engine.h>
#include <lde/cellfy/boox/value_format.h>
#include <lde/cellfy/boox/workbook.h>
#include <lde/cellfy/boox/worksheet.h>


namespace lde::cellfy::boox {
namespace _ {
namespace {


cell_value get_cell_value(range_op_ctx& ctx, cell_node::it node) {
  ED_ASSERT(!node->merged_with);
  ED_ASSERT(!node->has_formula);

  if (node->value_type != cell_value_type::none) {
    if (node->value_type == cell_value_type::rich_text) {
      rich_text rt;
      for (auto& child : ctx.forest().get<text_run_node>(node)) {
        rt.emplace_back();
        rt.back().text = child.text;
        if (node->format) {
          rt.back().format = child.format.unite(*node->format);
        } else {
          rt.back().format = child.format;
        }
      }
      return rt;
    } else {
      auto children = ctx.forest().get<cell_data_node>(node);
      ED_ASSERT(children.size() == 1);
      if (node->value_type == cell_value_type::boolean) {
        return std::get<bool>(children.front().data);
      } else if (node->value_type == cell_value_type::number) {
        return std::get<double>(children.front().data);
      } else if (node->value_type == cell_value_type::string) {
        return std::get<std::wstring>(children.front().data);
      } else if (node->value_type == cell_value_type::error) {
        return std::get<cell_value_error>(children.front().data);
      }
    }
  }
  return {};
}


const cell_value& get_formula_result(range_op_ctx& ctx, cell_node::it node) {
  ED_ASSERT(node->has_formula);
  auto children = ctx.forest().get<cell_formula_node>(node);
  ED_EXPECTS(children.size() == 1);
  auto& formula_node = children.front();
  ED_ASSERT(formula_node.is_parsed);

  if (formula_node.is_result_dirty) {
    ED_ASSERT(!formula_node.ast.empty());
    ED_ASSERT(formula_node.is_parsed);

    cell_value result;
    if (formula_node.in_calculating) {
      result = cell_value_error::ref; // В Google Sheets так.
    } else {
      ed::scoped_assign _(formula_node.in_calculating, true);
      result = fx::engine(ctx.sheet()).evaluate(formula_node.ast);
    }

    if (result != formula_node.result) {
      formula_node.result = std::move(result);
      node->is_layout_dirty = true;
    }
    formula_node.is_result_dirty = false;
  }
  return formula_node.result;
}


ed::rasta::text_alignment h_alignment_to_rasta_text_alignment(const cell_value_type type, const horizontal_alignment h_alignment) {
  // По умолчанию стоит тип горизонтального выравнивания general. Его свойства:
  // - Если тип данных строка(wstring или rich_text) - horizontal_alignment::left.
  // - Если тип данных число - horizontal_alignment::right.
  // - Если тип данных bool - horizontal_alignment::center.
  // - Если тип данных ошибка - horizontal_alignment::center.
  // - Если тип данных none - todo : не совсем понятно, что ставить в этом случае. Пока оставил horizontal_alignment::left.
  // todo: Осталось 3 типа: distibuted, fill, justify.
  switch (h_alignment) {
    case horizontal_alignment::general:
      switch (type) {
        case cell_value_type::number:
          return ed::rasta::text_alignment::right;
          break;

        case cell_value_type::none:
        case cell_value_type::string:
        case cell_value_type::rich_text:
          return ed::rasta::text_alignment::left;
          break;

        case cell_value_type::boolean:
        case cell_value_type::error:
          return ed::rasta::text_alignment::center;
          break;
      }
      break;

    case horizontal_alignment::center:
      return ed::rasta::text_alignment::center;
      break;

    case horizontal_alignment::left:
      return ed::rasta::text_alignment::left;
      break;

    case horizontal_alignment::right:
      return ed::rasta::text_alignment::right;
      break;
  };
}


/// Получить cell_value и тип из cell_node::it
/// В ячейке может лежать и формула.
auto get_cell_value_from_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->has_formula) {
    // get_formula_result может изменить is_layout_dirty
    return get_formula_result(ctx, node);
  } else {
    return get_cell_value(ctx, node);
  }
}


///  Получить ширину столбца.
auto get_columns_width(const worksheet& sheet, cell_node::it node) {
  ED_ASSERT(node->column_span >= 1);
  const auto left_column = cell_addr(node->index).column();
  const auto right_column = node->column_span > 1 ? left_column + node->column_span - 1 : left_column;
  return sheet.columns_width(left_column, right_column);
}


/// Сформировать и вернуть text_format и value_format_result.
auto formatter_result(const cell_value& val, const std::locale& loc, const cell_format& fmt, ed::pixels<double> columns_width) {
  value_format::result result;
  value_format formatter(fmt.get_or_default<number_format>(), loc);
  auto text_fmt = make_text_format(fmt);

  if (val.type() == cell_value_type::rich_text) {
    result = formatter(val.to<std::wstring>());
  } else {
    // Если выставлен default в number_format и тип данных double, то большие числа представляем в виде експоненты.
    // todo: Временное решение. Сейчас экспонента не подбирается под размер столбца.
    if (val.type() == cell_value_type::number && !formatter.has_any_section() && static_cast<int>(std::log10(val.as<double>()) + 1) > std::numeric_limits<double>::digits10) {
      result = value_format(L"0.00E+00")(val.as<double>());
    } else {
      result = formatter(val);
      // layout хранит ширину текста.
      // Но для обработки ошибки или для вставки заполнителя нужно расширить layout на ширину столбца, чтобы заполнить его определённым символом.
      // Если ячейки объединены, то берётся ширина нескольких столбцов [column, column + node->column_span - 1].
      if (result.fill_positions) {
        const auto text_width   = text_fmt.fnt.text_extents(result.text).width;
        const auto glyph_width  = text_fmt.fnt.text_extents(result.text.substr(*result.fill_positions, 1)).width;
        const auto fill_count   = static_cast<std::size_t>(std::max((columns_width - text_width) / glyph_width, {}));

        result.text.reserve(result.text.size() + fill_count);
        result.text.insert(*result.fill_positions, fill_count, result.text.at(*result.fill_positions));
      }
    }
  }

  if (result.text_color) {
    text_fmt.frg = *result.text_color;
  }

  return std::pair{std::move(result.text), std::move(text_fmt)};
}

}} // namespace _


get_cell_format_op::get_cell_format_op(cell_format& format) noexcept
  : format_(&format) {
}


void get_cell_format_op::on_start(range_op_ctx& ctx) {
  ED_ASSERT(format_);
  *format_ = {};
  processed_count_ = 0;
}


bool get_cell_format_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  ED_ASSERT(format_);
  if (!node->merged_with && node->format_key) {
    ED_ASSERT(node->format);
    if (processed_count_ == 0) {
      *format_ = *node->format;
    } else {
      *format_ = format_->intersect(*node->format);
    }
    ++processed_count_;
  }
  return true;
}


clear_cell_format_op::clear_cell_format_op(cell_index_set&& except) noexcept
  : except_(std::move(except)) {
}


bool clear_cell_format_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->format_key && except_.count(node->index) == 0) {
    ED_ASSERT(node->format);
    cell_node n = *node;
    n.format_key = std::nullopt;
    n.format = nullptr;
    ctx.forest().modify(node) = n;
  }
  return true;
}


change_existing_cell_format_op::change_existing_cell_format_op(cell_format::changes changes) noexcept
  : changes_(std::move(changes)) {
}


bool change_existing_cell_format_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (!node->merged_with) {
    if (node->format_key) {
      ED_ASSERT(node->format);
      auto new_fmt = node->format->apply(changes_);
      if (new_fmt != *node->format) {
        auto format_node = ctx.sheet().book().ensure_format(std::move(new_fmt));
        ED_ENSURES(format_node->format);

        cell_node n = *node;
        n.format_key = forest_t::key_of(format_node);
        n.format = format_node->format;
        ctx.forest().modify(node) = n;
      }
    } else {
      cell_addr addr(node->index);
      cell_format fmt;

      if (auto row_i = ctx.sheet().find_row(addr.row()); row_i && row_i.value()->format) {
        fmt = *row_i.value()->format;
      }

      if (auto col_i = ctx.sheet().find_column(addr.column()); col_i && col_i.value()->format) {
        fmt = fmt.unite(*col_i.value()->format);
      }

      if (ctx.sheet().node()->format) {
        fmt = fmt.unite(*ctx.sheet().node()->format);
      }

      fmt = fmt.apply(changes_);
      auto format_node = ctx.sheet().book().ensure_format(std::move(fmt));
      ED_ENSURES(format_node->format);

      cell_node n = *node;
      n.format_key = forest_t::key_of(format_node);
      n.format = format_node->format;
      ctx.forest().modify(node) = n;
    }
  }
  return true;
}


change_cell_format_op::change_cell_format_op(cell_format::changes changes) noexcept
  : change_existing_cell_format_op(std::move(changes)) {
}


bool change_cell_format_op::on_new_node(range_op_ctx& ctx, cell_node& node) {
  cell_addr addr(node.index);
  cell_format fmt;

  if (auto row_i = ctx.sheet().find_row(addr.row()); row_i && row_i.value()->format) {
    fmt = *row_i.value()->format;
  }

  if (auto col_i = ctx.sheet().find_column(addr.column()); col_i && col_i.value()->format) {
    fmt = fmt.unite(*col_i.value()->format);
  }

  if (ctx.sheet().node()->format) {
    fmt = fmt.unite(*ctx.sheet().node()->format);
  }

  fmt = fmt.apply(changes_);
  auto format_node = ctx.sheet().book().ensure_format(std::move(fmt));
  ED_ENSURES(format_node->format);

  node.format_key = forest_t::key_of(format_node);
  node.format = format_node->format;
  return true;
}


get_cell_value_op::get_cell_value_op(cell_value& value) noexcept
  : value_(&value) {
}


void get_cell_value_op::on_start(range_op_ctx& ctx) {
  ED_ASSERT(value_);
  *value_ = {};
  processed_count_ = 0;
  existing_count_ = 0;
}


void get_cell_value_op::on_finish(range_op_ctx& ctx) {
  ED_ASSERT(value_);

  std::uint64_t cells_count = 0;
  for (auto& ar : ctx.areas()) {
    cells_count += ar.cells_count();
  }

  if (existing_count_ != cells_count) {
    *value_ = {};
  }
}


bool get_cell_value_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  ED_ASSERT(value_);
  if (!node->merged_with) {
    cell_value val;

    if (node->has_formula) {
      val = _::get_formula_result(ctx, node);
    } else {
      val = _::get_cell_value(ctx, node);
    }

    if (processed_count_ == 0) {
      *value_ = std::move(val);
    } else {
      if (*value_ != val) {
        *value_ = {};
        return false;
      }
    }

    ++processed_count_;
  }
  ++existing_count_;
  return true;
}


get_cell_values_op::get_cell_values_op(cell_value::list& values) noexcept
  : values_(&values) {
}


void get_cell_values_op::on_start(range_op_ctx& ctx) {
  ED_ASSERT(values_);
  values_->clear();
}


bool get_cell_values_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  ED_ASSERT(values_);
  if (!node->merged_with) {
    if (node->has_formula) {
      values_->push_back(_::get_formula_result(ctx, node));
    } else {
      values_->push_back(_::get_cell_value(ctx, node));
    }
  }
  return true;
}


get_cell_values_matrix_op::get_cell_values_matrix_op(cell_value::matrix& values) noexcept
  : values_(&values) {
}


void get_cell_values_matrix_op::on_start(range_op_ctx& ctx) {
  ED_ASSERT(values_);
  ED_ASSERT(ctx.areas().size() == 1);
  values_->resize(ctx.areas().front().rows_count(), ctx.areas().front().columns_count());
}


bool get_cell_values_matrix_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  ED_ASSERT(values_);
  if (!node->merged_with) {
    cell_addr addr(node->index);
    row_index row = addr.row() - ctx.areas().front().top_row();
    column_index column = addr.column() - ctx.areas().front().left_column();

    if (node->has_formula) {
      values_->at(row, column) = _::get_formula_result(ctx, node);
    } else {
      values_->at(row, column) = _::get_cell_value(ctx, node);
    }
  }
  return true;
}


split_by_format_and_type::split_by_format_and_type(value_ranges& val_ranges) noexcept
  : value_ranges_(val_ranges) {
}


void split_by_format_and_type::on_start(range_op_ctx& ctx) {
  ED_EXPECTS(ctx.areas().size() == 1);
  start_column_index_ = ctx.areas().front().left_column();
  start_row_index_ = ctx.areas().front().top_row();
  columns_count_ = ctx.areas().front().columns_count();
}


bool split_by_format_and_type::on_existing_node(range_op_ctx& ctx, cell_node::it it) {
  if (!it->merged_with) {
    // Сохранив стартовые значения строк/столбцов. Можем получить относительный индекс (относительно области).
    const auto current_col_index = it->index % cell_addr::max_column_count - start_column_index_;
    const auto current_row_index = it->index / cell_addr::max_column_count - start_row_index_;
    const auto current_cell_index = current_row_index * columns_count_ + current_col_index;
    // В первый раз ининциализируем format_type_ типом и по возможности форматом.
    if (current_cell_index == 0) {
      format_type_.type = it->value_type;
      if (it->format) {
        format_type_.format = *it->format;
      }
      create_new_subrange(ctx, it, current_cell_index);
     } else {
      // Если 2 формата отсутствовали или были, но равны и тип ячейки одинаковый был.
      // При сравнении учитываются number_format.
      if (static_cast<bool>(format_type_.format) == static_cast<bool>(it->format) &&
         (!format_type_.format || format_type_.format->get_or_default<number_format>() == it->format->get_or_default<number_format>()) &&
         format_type_.type == it->value_type) {
        expand_current_subrange(ctx, it, current_cell_index);
      } else {
        format_type_.format = it->format ? std::make_optional(*it->format) : std::nullopt;
        format_type_.type = it->value_type;
        create_new_subrange(ctx, it, current_cell_index);
      }
    }
  }
  return true;
}


auto split_by_format_and_type::get_value_data_from_node(range_op_ctx& ctx, cell_node::it it) {
  value_data result;

  const auto fill_result_from_cell_value = [](auto&& cell_value, auto& result) {
    if (cell_value.type() == cell_value_type::number) {
      result = cell_value.template as<double>();
    } else if (cell_value.type() == cell_value_type::rich_text) {
      result = cell_value.template as<rich_text>();
    } else if (cell_value.type() == cell_value_type::string) {
      result = cell_value.template as<std::wstring>();
    } else if (cell_value.type() == cell_value_type::boolean) {
      result = cell_value.template as<bool>();
    } else if (cell_value.type() == cell_value_type::error) {
      result = cell_value.template as<cell_value_error>();
    }
  };

  if (it->has_formula) {
    const auto formula_node = ctx.forest().get<cell_formula_node>(it);
    ED_EXPECTS(formula_node.size() == 1);
    if (formula_node.front().result.type() != cell_value_type::error) {
      result = formula_node.front().ast;
    } else {
      fill_result_from_cell_value(formula_node.front().result, result);
    }
  } else {
    fill_result_from_cell_value(_::get_cell_value(ctx, it), result);
  }
  return result;
}


void split_by_format_and_type::create_new_subrange(range_op_ctx& ctx, cell_node::it it, cell_index current_cell_index) {
  std::visit([this, current_cell_index](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, fx::ast::tokens>) {
      value_ranges_.emplace_back(ast_subrange{format_type_.format, ast_table{{current_cell_index, std::move(arg)}}});
    } else if constexpr (std::is_same_v<T, double>) {
      value_ranges_.emplace_back(double_subrange{format_type_.format, double_table{{current_cell_index, arg}}});
    } else if constexpr (std::is_same_v<T, std::wstring>) {
      value_ranges_.emplace_back(wstring_subrange{format_type_.format, wstring_table{{current_cell_index, std::move(arg)}}});
    } else if constexpr (std::is_same_v<T, rich_text>) {
      value_ranges_.emplace_back(rich_text_subrange{format_type_.format, rich_text_table{{current_cell_index, std::move(arg)}}});
    } else if constexpr (std::is_same_v<T, bool>) {
      value_ranges_.emplace_back(bool_subrange{format_type_.format, bool_table{{current_cell_index, arg}}});
    } else if constexpr(std::is_same_v<T, cell_value_error>) {
      value_ranges_.emplace_back(error_subrange{format_type_.format, error_table{{current_cell_index, arg}}});
    }
  }, get_value_data_from_node(ctx, it));
}


void split_by_format_and_type::expand_current_subrange(range_op_ctx& ctx, cell_node::it it, cell_index current_cell_index) {
  std::visit([this, current_cell_index](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, fx::ast::tokens>) {
      auto& ast_pair = std::get<ast_subrange>(value_ranges_.back());
      ast_pair.second.emplace_back(current_cell_index, std::move(arg));
    } else if constexpr (std::is_same_v<T, double>) {
      auto& double_pair = std::get<double_subrange>(value_ranges_.back());
      double_pair.second.emplace_back(current_cell_index, arg);
    } else if constexpr (std::is_same_v<T, std::wstring>) {
      auto& wstring_pair = std::get<wstring_subrange>(value_ranges_.back());
      wstring_pair.second.emplace_back(current_cell_index, std::move(arg));
    } else if constexpr (std::is_same_v<T, rich_text>) {
      auto& rich_text_pair = std::get<rich_text_subrange>(value_ranges_.back());
      rich_text_pair.second.emplace_back(current_cell_index, std::move(arg));
    } else if constexpr (std::is_same_v<T, bool>) {
      auto& bool_pair = std::get<bool_subrange>(value_ranges_.back());
      bool_pair.second.emplace_back(current_cell_index, arg);
    } else if constexpr (std::is_same_v<T, cell_value_error>) {
      auto& error_pair = std::get<error_subrange>(value_ranges_.back());
      error_pair.second.emplace_back(current_cell_index, arg);
    }
  }, get_value_data_from_node(ctx, it));
}


clear_cell_value_op::clear_cell_value_op(cell_index_set&& except) noexcept
  : except_(std::move(except)) {
}


bool clear_cell_value_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (except_.count(node->index) == 0) {
    if (node->value_type != cell_value_type::none) {
      cell_node n = *node;
      n.value_type = cell_value_type::none;

      if (node->value_type == cell_value_type::rich_text) {
        auto children = ctx.forest().get<text_run_node>(node);
        ctx.forest().erase(children.begin(), children.end());
      } else {
        auto children = ctx.forest().get<cell_data_node>(node);
        ctx.forest().erase(children.begin(), children.end());
      }

      if (n != *node) {
        ctx.forest().modify(node) = n;
      }
    }
  }

  return true;
}


set_cell_value_op::set_cell_value_op(const cell_value& value) noexcept
  : value_(&value) {
  ED_ASSERT(!value.is_nil());
}


bool set_cell_value_op::on_new_node(range_op_ctx& ctx, cell_node& node) {
  ED_ASSERT(value_);
  node.value_type = value_->type();
  return true;
}


bool set_cell_value_op::on_new_node(range_op_ctx& ctx, cell_node::it node) {
  ED_ASSERT(value_);
  if (node->value_type != cell_value_type::none) {
    if (node->value_type == cell_value_type::rich_text) {
      for (auto& run : value_->as<rich_text>()) {
        text_run_node child;
        child.text = run.text;
        child.format = run.format;
        ctx.forest().push_back(node, std::move(child));
      }
    } else {
      cell_data_node child;
      if (node->value_type == cell_value_type::boolean) {
        child.data = value_->as<bool>();
      } else if (node->value_type == cell_value_type::number) {
        child.data = value_->as<double>();
      } else if (node->value_type == cell_value_type::string) {
        child.data = value_->as<std::wstring>();
      } else if (node->value_type == cell_value_type::error) {
        child.data = value_->as<cell_value_error>();
      }
      ctx.forest().push_back(node, std::move(child));
    }
  }
  return true;
}


bool set_cell_value_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  ED_ASSERT(value_);
  cell_value_type type = value_->type();

  if (node->value_type != cell_value_type::none && node->value_type != type) {
    if (node->value_type == cell_value_type::rich_text) {
      auto children = ctx.forest().get<text_run_node>(node);
      ctx.forest().erase(children.begin(), children.end());
    } else if (type == cell_value_type::rich_text) {
      auto children = ctx.forest().get<cell_data_node>(node);
      ctx.forest().erase(children.begin(), children.end());
    }
  }

  if (type == cell_value_type::rich_text) {
    auto children = ctx.forest().get<text_run_node>(node);
    auto child_it = children.begin();

    for (auto& run : value_->as<rich_text>()) {
      if (child_it == children.end()) {
        text_run_node child;
        child.text = run.text;
        child.format = run.format;
        child_it = ctx.forest().push_back(node, std::move(child));
      } else {
        text_run_node child;
        child.text = run.text;
        child.format = run.format;
        if (!ed::aggregate_equal(child, *child_it)) {
          ctx.forest().modify(child_it) = std::move(child);
        }
      }
      ++child_it;
    }

    ctx.forest().erase(child_it, children.end());
  } else {
    cell_data_node child;
    if (type == cell_value_type::boolean) {
      child.data = value_->as<bool>();
    } else if (type == cell_value_type::number) {
      child.data = value_->as<double>();
    } else if (type == cell_value_type::string) {
      child.data = value_->as<std::wstring>();
    } else if (type == cell_value_type::error) {
      child.data = value_->as<cell_value_error>();
    }

    auto children = ctx.forest().get<cell_data_node>(node);
    auto child_it = children.begin();

    if (child_it == children.end()) {
      ctx.forest().push_back(node, std::move(child));
    } else if (child_it->data != child.data) {
      ctx.forest().modify(child_it) = std::move(child);
    }
  }

  if (node->value_type != type) {
    cell_node n = *node;
    n.value_type = type;
    ctx.forest().modify(node) = n;
  }

  return true;
}


get_cell_formula_op::get_cell_formula_op(std::wstring& formula) noexcept
  : formula_(&formula) {
}


void get_cell_formula_op::on_start(range_op_ctx& ctx) {
  ED_ASSERT(formula_);
  formula_->clear();
  processed_count_ = 0;
  existing_count_ = 0;
}


void get_cell_formula_op::on_finish(range_op_ctx& ctx) {
  ED_ASSERT(formula_);

  std::uint64_t cells_count = 0;
  for (auto& ar : ctx.areas()) {
    cells_count += ar.cells_count();
  }

  if (existing_count_ != cells_count) {
    formula_->clear();
  }
}


bool get_cell_formula_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  ED_ASSERT(formula_);
  if (!node->merged_with) {
    if (node->has_formula) {
      auto children = ctx.forest().get<cell_formula_node>(node);
      ED_ASSERT(children.size() == 1);
      if (processed_count_ == 0) {
        *formula_ = children.front().formula;
      } else {
        if (*formula_ != children.front().formula) {
          formula_->clear();
          return false;
        }
      }
    } else {
      formula_->clear();
      return false;
    }
    ++processed_count_;
  }
  ++existing_count_;
  return true;
}


clear_cell_formula_op::clear_cell_formula_op(cell_index_set&& except) noexcept
  : except_(std::move(except)) {
}


bool clear_cell_formula_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->has_formula && except_.count(node->index) == 0) {
    auto children = ctx.forest().get<cell_formula_node>(node);
    ctx.forest().erase(children.begin(), children.end());

    cell_node n = *node;
    n.has_formula = false;
    ctx.forest().modify(node) = n;
  }
  return true;
}


set_cell_formula_op::set_cell_formula_op(const std::wstring& formula)
  : formula_(formula) {
}


bool set_cell_formula_op::on_new_node(range_op_ctx& ctx, cell_node& node) {
  node.has_formula = true;
  return true;
}


bool set_cell_formula_op::on_new_node(range_op_ctx& ctx, cell_node::it node) {
  cell_formula_node child;
  child.formula = formula_;
  ctx.forest().push_back(node, std::move(child));
  return true;
}


bool set_cell_formula_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->has_formula) {
    auto children = ctx.forest().get<cell_formula_node>(node);
    auto child_it = children.begin();

    if (child_it == children.end()) {
      cell_formula_node child;
      child.formula = formula_;
      ctx.forest().push_back(node, std::move(child));
    } else {
      if (child_it->formula != formula_) {
        auto child = *child_it;
        child.formula = formula_;
        ctx.forest().modify(child_it) = std::move(child);
      }
    }
  } else {
    cell_node n = *node;
    n.has_formula = true;
    ctx.forest().modify(node) = n;

    cell_formula_node child;
    child.formula = formula_;
    ctx.forest().push_back(node, std::move(child));
  }
  return true;
}


void merge_cells_op::on_start(range_op_ctx& ctx) {
  if (ctx.areas().size() > 1) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }
  merged_with_ = ctx.areas().front().top_left().index();
}


bool merge_cells_op::on_new_node(range_op_ctx& ctx, cell_node& node) {
  if (node.index == merged_with_) {
    node.column_span = ctx.areas().front().columns_count();
    node.row_span = ctx.areas().front().rows_count();
  } else {
    node.merged_with = merged_with_;
  }
  return true;
}


bool merge_cells_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->index == merged_with_) {
    cell_node n = *node;
    n.column_span = ctx.areas().front().columns_count();
    n.row_span = ctx.areas().front().rows_count();
    n.merged_with = std::nullopt;
    n.merged_with_node = nullptr;
    ctx.forest().modify(node) = n;
  } else {
    cell_node n = *node;
    n.merged_with = merged_with_;
    n.merged_with_node = nullptr;
    ctx.forest().modify(node) = n;
  }
  return true;
}


void unmerge_cells_op::on_start(range_op_ctx& ctx) {
  if (ctx.areas().size() > 1) {
    ED_THROW_EXCEPTION(too_many_areas_in_range());
  }
}


bool unmerge_cells_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->column_span > 1 || node->row_span > 1) {
    ED_ASSERT(!node->merged_with);
    cell_node n = *node;
    n.column_span = 1;
    n.row_span = 1;
    ctx.forest().modify(node) = n;
  } else if (node->merged_with) {
    ctx.forest().erase(node);
  }
  return true;
}


bool erase_empty_cells_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->row_span == 1 &&
      node->column_span == 1 &&
      node->value_type == cell_value_type::none &&
      !node->has_formula &&
      !node->merged_with &&
      !node->format_key) {

    ctx.forest().erase(node);
  }
  return true;
}


bool actualize_cell_format_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->format_key) {
    node->format = ctx.forest().find<cell_format_node>(*node->format_key)->format;
    ED_ASSERT(node->format);
  } else {
    node->format = nullptr;
  }
  return true;
}


bool actualize_layout_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->value_type == cell_value_type::none && !node->has_formula) {
    node->layout = nullptr;
  } else {
    const auto value = _::get_cell_value_from_node(ctx, node);
    const auto& loc = ctx.sheet().book().get_locale();

    if (node->is_layout_dirty) {
      if (!node->layout) {
        node->layout = std::make_shared<ed::rasta::text_layout>();
      } else {
        node->layout->clear();
      }

      cell_format fmt;
      if (node->format_key) {
        ED_ASSERT(node->format);
        fmt = *node->format;
      }

      node->layout->alignment(_::h_alignment_to_rasta_text_alignment(value.type(), fmt.get_or_default<text_horizontal_alignment>()));
      node->layout->enable_soft_breaks(fmt.get_or_default<text_wrap>());

      if (fmt.get_or_default<text_wrap>()) {
        column_index column = cell_addr(node->index).column();
        node->layout->max_line_width(ctx.sheet().columns_width(column, column));
      }

      const auto result = _::formatter_result(value, loc, fmt, _::get_columns_width(ctx.sheet(), node));
      node->layout->add(result.first, result.second);
    }
  }

  node->is_layout_dirty = false;
  return true;
}


bool invalidate_layout_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (!node->is_layout_dirty && node->value_type != cell_value_type::none) {
    node->is_layout_dirty = true;
  }
  return true;
}


bool invalidate_layout_width_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (!node->is_layout_dirty && node->value_type != cell_value_type::none) {
    if (node->format_key) {
      ED_ASSERT(node->format);
      if (node->format->get_or_default<text_wrap>()) {
        node->is_layout_dirty = true;
      }
    } else if (text_wrap::default_value) {
      node->is_layout_dirty = true;
    }
  }
  return true;
}


bool parse_formulas_op::on_existing_node(range_op_ctx& ctx, cell_node::it node) {
  if (node->has_formula) {
    ctx.sheet().parse_formula(node);
  }
  return true;
}

} // namespace lde::cellfy::boox
