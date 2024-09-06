#pragma once


#include <optional>
#include <tuple>

#include <boost/uuid/string_generator.hpp>

#include <ed/core/assert.h>
#include <ed/core/color.h>
#include <ed/core/quantity.h>
#include <ed/core/quantity_rw.h>
#include <ed/core/rw.h>
#include <ed/core/uuid.h>

#include <ed/rasta/fwd.h>

#include <lde/cellfy/boox/cell_value.h>
#include <lde/cellfy/boox/forest.h>
#include <lde/cellfy/boox/format.h>
#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/fx_ast.h>


namespace lde::cellfy::boox {


// Корневой узел всего документа.
// Будет полезно при встраивании документа в любой другой лес.
struct workbook_node final {
  using it     = forest_iterator<workbook_node>;
  using opt_it = std::optional<it>;

  constexpr static node_version version = 1;

  // TODO: данные по всей книге

  template<typename OStream>
  friend void write(OStream& os, const workbook_node& n) {
    using ed::write;
    write(os, version);
  }

  template<typename IStream>
  friend void read(IStream& is, workbook_node& n) {
    using ed::read;
    node_version v;
    read(is, v);
    ED_EXPECTS(v == version);
  }
};


struct worksheet_node final {
  using it     = forest_iterator<worksheet_node>;
  using opt_it = std::optional<it>;

  constexpr static node_version version = 2;

  std::wstring                 name;
  bool                         hidden = false;
  std::optional<ed::color>     tab_color;
  std::optional<node_key_type> format_key; // Ключ cell_format_node
  mutable worksheet*           sheet = nullptr;
  mutable cell_format::ptr     format;

  template<typename OStream>
  friend void write(OStream& os, const worksheet_node& n) {
    using ed::write;
    write(os, version);
    write(os, n.name);
    write(os, n.hidden);
    write(os, n.tab_color);
    write(os, n.format_key);
  }

  template<typename IStream>
  friend void read(IStream& is, worksheet_node& n) {
    using ed::read;
    node_version v;
    read(is, v);
    read(is, n.name);
    read(is, n.hidden);
    read(is, n.tab_color);
    if (v > 1) {
      read(is, n.format_key);
    }
  }
};


struct cell_format_node final {
  using it     = forest_iterator<cell_format_node>;
  using opt_it = std::optional<it>;

  constexpr static node_version version = 1;

  typename cell_format::ptr format;

  template<typename OStream>
  friend void write(OStream& os, const cell_format_node& n) {
    using ed::write;
    write(os, version);
    write(os, n.format);
  }

  template<typename IStream>
  friend void read(IStream& is, cell_format_node& n) {
    using ed::read;
    node_version v;
    read(is, v);
    ED_EXPECTS(v == version);
    read(is, n.format);
  }
};


struct column_node final {
  using it     = forest_iterator<column_node>;
  using opt_it = std::optional<it>;

  constexpr static node_version version = 2;

  column_index                 index        = 0;
  ed::twips<double>            width        = 0._tw;
  bool                         custom_width = false;
  bool                         hidden       = false;
  bool                         collapsed    = false;
  std::optional<node_key_type> format_key; // Ключ cell_format_node
  mutable cell_format::ptr     format;

  bool operator<(const column_node& rhs) const noexcept {
    return index < rhs.index;
  }

  template<typename OStream>
  friend void write(OStream& os, const column_node& n) {
    using ed::write;
    write(os, version);
    write(os, n.index);
    write(os, n.width);
    write(os, n.custom_width);
    write(os, n.hidden);
    write(os, n.collapsed);
    write(os, n.format_key);
  }

  template<typename IStream>
  friend void read(IStream& is, column_node& n) {
    using ed::read;
    node_version v;
    read(is, v);
    read(is, n.index);
    read(is, n.width);
    read(is, n.custom_width);
    read(is, n.hidden);
    read(is, n.collapsed);
    if (v > 1) {
      read(is, n.format_key);
    }
  }
};


struct row_node final {
  using it     = forest_iterator<row_node>;
  using opt_it = std::optional<it>;

  constexpr static node_version version = 2;

  row_index                    index         = 0;
  ed::twips<double>            height        = 0._tw;
  bool                         custom_height = false;
  bool                         hidden        = false;
  bool                         collapsed     = false;
  std::optional<node_key_type> format_key; // Ключ cell_format_node
  mutable cell_format::ptr     format;

  bool operator<(const row_node& rhs) const noexcept {
    return index < rhs.index;
  }

  template<typename OStream>
  friend void write(OStream& os, const row_node& n) {
    using ed::write;
    write(os, version);
    write(os, n.index);
    write(os, n.height);
    write(os, n.custom_height);
    write(os, n.hidden);
    write(os, n.collapsed);
    write(os, n.format_key);
  }

  template<typename IStream>
  friend void read(IStream& is, row_node& n) {
    using ed::read;
    node_version v;
    read(is, v);
    read(is, n.index);
    read(is, n.height);
    read(is, n.custom_height);
    read(is, n.hidden);
    read(is, n.collapsed);
    if (v > 1) {
      read(is, n.format_key);
    }
  }
};


struct cell_node final {
  using it              = forest_iterator<cell_node>;
  using opt_it          = std::optional<it>;
  using text_layout_ptr = std::shared_ptr<ed::rasta::text_layout>;

  constexpr static node_version version = 1;

  cell_index                   index            = 0;
  std::uint32_t                column_span      = 1;
  std::uint32_t                row_span         = 1;
  cell_value_type              value_type       = cell_value_type::none;
  bool                         has_formula      = false;
  std::optional<cell_index>    merged_with;
  std::optional<node_key_type> format_key;                 // Ключ cell_format_node
  mutable const cell_node*     merged_with_node = nullptr; // Кэш указателя на ячейку по адресу merged_with
  mutable cell_format::ptr     format;
  mutable text_layout_ptr      layout;
  mutable bool                 is_layout_dirty  = true;

  bool operator<(const cell_node& rhs) const noexcept {
    return index < rhs.index;
  }

  bool operator==(const cell_node& rhs) const noexcept {
    return std::tie(index, column_span, row_span, value_type, has_formula, merged_with, format_key) ==
           std::tie(rhs.index, rhs.column_span, rhs.row_span, rhs.value_type, rhs.has_formula, rhs.merged_with, rhs.format_key);
  }

  bool operator!=(const cell_node& rhs) const noexcept {
    return std::tie(index, column_span, row_span, value_type, has_formula, merged_with, format_key) !=
           std::tie(rhs.index, rhs.column_span, rhs.row_span, rhs.value_type, rhs.has_formula, rhs.merged_with, rhs.format_key);
  }

  template<typename OStream>
  friend void write(OStream& os, const cell_node& n) {
    using ed::write;
    write(os, version);
    write(os, n.index);
    write(os, n.column_span);
    write(os, n.row_span);
    write(os, n.value_type);
    write(os, n.has_formula);
    write(os, n.merged_with);
    write(os, n.format_key);
  }

  template<typename IStream>
  friend void read(IStream& is, cell_node& n) {
    using ed::read;
    node_version v;
    read(is, v);
    ED_EXPECTS(v == version);
    read(is, n.index);
    read(is, n.column_span);
    read(is, n.row_span);
    read(is, n.value_type);
    read(is, n.has_formula);
    read(is, n.merged_with);
    read(is, n.format_key);
  }
};


struct cell_data_node final {
  using it     = forest_iterator<cell_data_node>;
  using opt_it = std::optional<it>;

  constexpr static node_version version = 1;

  std::variant<
    bool,
    double,
    std::wstring,
    cell_value_error
  > data;

  template<typename OStream>
  friend void write(OStream& os, const cell_data_node& n) {
    using ed::write;
    write(os, version);
    write(os, n.data);
  }

  template<typename IStream>
  friend void read(IStream& is, cell_data_node& n) {
    using ed::read;
    node_version v;
    read(is, v);
    ED_EXPECTS(v == version);
    read(is, n.data);
  }
};


struct text_run_node final {
  using it     = forest_iterator<text_run_node>;
  using opt_it = std::optional<it>;

  using font_format_1 = format<
    font_name,
    font_size,
    font_weight,
    font_italic,
    font_underline,
    font_strike_out,
    font_background,
    font_foreground
  >;

  constexpr static node_version version = 2;

  std::wstring text;
  font_format  format;

  template<typename OStream>
  friend void write(OStream& os, const text_run_node& n) {
    using ed::write;
    write(os, version);
    write(os, n.text);
    write(os, n.format);
  }

  template<typename IStream>
  friend void read(IStream& is, text_run_node& n) {
    using ed::read;
    node_version v;
    read(is, v);
    read(is, n.text);
    switch (v) {
      case 1: {
          font_format_1 v1_format;
          read(is, v1_format);
          n.format.set<font_script>(font_script::default_value);
          n.format.unite(v1_format);
        }
        break;
      case version:
        read(is, n.format);
        break;
    }
  }
};


struct cell_formula_node final {
  using it     = forest_iterator<cell_formula_node>;
  using opt_it = std::optional<it>;

  constexpr static node_version version = 1;

  std::wstring            formula;
  mutable fx::ast::tokens ast;
  mutable bool            is_parsed       = false;
  mutable bool            is_volatile     = true;
  mutable bool            is_result_dirty = true;
  mutable bool            in_calculating  = false; // Для определения циклических зависимостей
  mutable cell_value      result;

  template<typename OStream>
  friend void write(OStream& os, const cell_formula_node& n) {
    using ed::write;
    write(os, version);
    write(os, n.formula);
  }

  template<typename IStream>
  friend void read(IStream& is, cell_formula_node& n) {
    using ed::read;
    node_version v;
    read(is, v);
    ED_EXPECTS(v == version);
    read(is, n.formula);
  }
};


} // namespace lde::cellfy::boox


namespace lde::forest {


template<>
struct type_id<cellfy::boox::worksheet_node> {
  static const type_id_t& value() noexcept {
    static type_id_t id = "00000000-0000-0000-0000-000000000001"_uuid;
    return id;
  }
};


template<>
struct type_id<cellfy::boox::cell_format_node> {
  static const type_id_t& value() noexcept {
    static type_id_t id = "00000000-0000-0000-0000-000000000002"_uuid;
    return id;
  }
};


template<>
struct type_id<cellfy::boox::column_node> {
  static const type_id_t& value() noexcept {
    static type_id_t id = "00000000-0000-0000-0000-000000000003"_uuid;
    return id;
  }
};


template<>
struct type_id<cellfy::boox::row_node> {
  static const type_id_t& value() noexcept {
    static type_id_t id = "00000000-0000-0000-0000-000000000004"_uuid;
    return id;
  }
};


template<>
struct type_id<cellfy::boox::cell_node> {
  static const type_id_t& value() noexcept {
    static type_id_t id = "00000000-0000-0000-0000-000000000005"_uuid;
    return id;
  }
};


template<>
struct type_id<cellfy::boox::cell_data_node> {
  static const type_id_t& value() noexcept {
    static type_id_t id = "00000000-0000-0000-0000-000000000006"_uuid;
    return id;
  }
};


template<>
struct type_id<cellfy::boox::text_run_node> {
  static const type_id_t& value() noexcept {
    static type_id_t id = "00000000-0000-0000-0000-000000000007"_uuid;
    return id;
  }
};


template<>
struct type_id<cellfy::boox::cell_formula_node> {
  static const type_id_t& value() noexcept {
    static type_id_t id = "00000000-0000-0000-0000-000000000008"_uuid;
    return id;
  }
};


} // namespace lde::forest
