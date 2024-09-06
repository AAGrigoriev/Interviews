#pragma once


#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

#include <boost/functional/hash.hpp>
#include <boost/mp11.hpp>
#include <boost/operators.hpp>

#include <ed/core/color.h>
#include <ed/core/color_rw.h>
#include <ed/core/exception.h>
#include <ed/core/quantity.h>
#include <ed/core/rw.h>
#include <ed/core/type_traits.h>

#include <ed/rasta/enums.h>
#include <ed/rasta/text_layout.h>

#include <lde/cellfy/boox/enums.h>


namespace lde::cellfy::boox {

inline const ed::color default_line_color = "#d3d3d3"_argb;

struct border final {
  border_style style      = border_style::none;
  ed::color        line_color = default_line_color;

  bool operator==(const border& rhs) const noexcept {
    return style == rhs.style && line_color == rhs.line_color;
  }

  friend std::size_t hash_value(const border& bs) noexcept {
    std::size_t seed = 0;
    boost::hash_combine(seed, bs.style);
    boost::hash_combine(seed, bs.line_color);
    return seed;
  }

  template<typename OStream>
  friend void write(OStream& os, const border& br) {
    // TODO: version
    using ed::write;
    write(os, br.style);
    write(os, br.line_color);
  }

  template<typename IStream>
  friend void read(IStream& is, border& br) {
    // TODO: version
    using ed::read;
    read(is, br.style);
    read(is, br.line_color);
  }
};


struct font_name {
  using value_type = std::string;
  static inline const value_type default_value = "Roboto";
};


struct font_size {
  using value_type = ed::points<double>;
  static inline const value_type default_value = 9._pt;
};


struct font_weight {
  using value_type = ed::rasta::font_weight;
  static inline const value_type default_value = ed::rasta::font_weight::normal;
};


struct font_italic {
  using value_type = bool;
  static inline const value_type default_value = false;
};


struct font_underline {
  using value_type = bool;
  static inline const value_type default_value = false;
};


struct font_strike_out {
  using value_type = bool;
  static inline const value_type default_value = false;
};


struct font_script {
  using value_type = script;
  static inline const value_type default_value = script::normal;
};


struct font_background {
  using value_type = ed::color;
  static inline const value_type default_value = ed::color::known::transparent;
};


struct font_foreground {
  using value_type = ed::color;
  static inline const value_type default_value = ed::color::known::black;
};


struct fill_pattern_type {
  using value_type = pattern_type;
  static inline const value_type default_value = pattern_type::none;
};


struct fill_background {
  using value_type = ed::color;
  static inline const value_type default_value = ed::color::known::transparent;
};


struct fill_foreground {
  using value_type = ed::color;
  static inline const value_type default_value = ed::color::known::black;
};


struct top_border {
  using value_type = border;
};


struct bottom_border {
  using value_type = border;
};


struct left_border {
  using value_type = border;
};


struct right_border {
  using value_type = border;
};


//https://docs.microsoft.com/ru-ru/office/vba/api/excel.xlhalign
struct text_horizontal_alignment {
  using value_type = horizontal_alignment;
  static inline const value_type default_value = horizontal_alignment::general;
};


struct text_vertical_alignment {
  using value_type = vertical_alignment;
  static inline const value_type default_value = vertical_alignment::bottom;
};


struct text_rotation {
  using value_type = ed::degrees<double>;
  static inline const value_type default_value = 0._deg;
};


struct text_wrap {
  using value_type = bool;
  static inline const value_type default_value = false;
};


struct text_indent {
  using value_type = unsigned int;
  static inline const value_type default_value = 0;
};


/// Числовые форматы.
/// https://support.microsoft.com/en-us/office/available-number-formats-in-excel-0afe8f52-97db-41f1-b972-4b46e9f1e8d2#ID0EBBH=Windows
/// Отсутвие number_format - "Автоматический формат".
/// "@"                    - "Текстовый формат".
struct number_format {
  using value_type = std::wstring;
  static inline const value_type default_value = {};
};


template<typename T, typename = void>
struct has_default_value : std::false_type {};

template<typename T>
struct has_default_value<T, std::void_t<decltype(T::default_value)>> : std::true_type {};

template<typename T>
constexpr bool has_default_value_v = has_default_value<T>::value;


template<typename ... T>
class format_changes final {
  template<typename ...>
  friend class format;

public:
  using fields = boost::mp11::mp_list<T ...>;

public:
  format_changes() noexcept {
    flags_.fill(false);
  }

  template<typename F>
  format_changes& set(std::optional<typename F::value_type>&& value) noexcept {
    std::get<boost::mp11::mp_find<fields, F>::value>(values_) = std::move(value);
    flags_[boost::mp11::mp_find<fields, F>::value] = true;
    return *this;
  }

  bool empty() const noexcept {
    return std::count(flags_.begin(), flags_.end(), true) == 0;
  }

  /// Возвращает объединенные изменения текущего с changes. При конфликте, приоритет у текущего.
  template<typename ... C>
  format_changes unite(const format_changes<C ...>& changes) const {
    format_changes result;
    boost::mp11::mp_for_each<fields>([&result, &changes, this](auto field) {
      using field_type = decltype(field);
      constexpr std::size_t result_index = boost::mp11::mp_find<fields, field_type>::value;
      if (flags_.at(result_index)) {
        result.flags_.at(result_index) = true;
        std::get<result_index>(result.values_) = std::get<result_index>(values_);
      } else {
        if constexpr (boost::mp11::mp_contains<typename format_changes<C ...>::fields , field_type>::value) {
          constexpr std::size_t changes_index = boost::mp11::mp_find<typename format_changes<C ...>::fields, field_type>::value;
          if (changes.flags_.at(changes_index)) {
            result.flags_.at(result_index) = true;
            std::get<result_index>(result.values_) = std::get<changes_index>(changes.values_);
          }
        }
      }
    });
    return result;
  }

private:
  using opt_values = std::tuple<std::optional<typename T::value_type> ...>;
  using flags      = std::array<bool, sizeof...(T)>;

  opt_values values_;
  flags      flags_;
};


/// Формат, состоящий из полей T
template<typename ... T>
class format final : boost::equality_comparable<format<T ...>> {
  template<typename ...>
  friend class format;

public:
  using fields  = boost::mp11::mp_list<T ...>;
  using changes = format_changes<T ...>;
  using ptr     = std::shared_ptr<format>;

public:
  /// Возможно ли хранение поля F в текущем формате
  template<typename F>
  static constexpr bool can_hold() noexcept {
    return boost::mp11::mp_contains<fields, F>::value;
  }

public:
  format() = default;

  bool operator==(const format& rhs) const noexcept {
    return values_ == rhs.values_;
  }

  /// Наличие поля F.
  template<typename F>
  bool holds() const noexcept {
    constexpr std::size_t field_index = boost::mp11::mp_find<fields, F>::value;
    return std::get<field_index>(values_).has_value();
  }

  /// Наличие поля F. Если есть default_value, то всегда будет true.
  template<typename F>
  bool holds_or_default() const noexcept {
    return holds<F>() || has_default_value_v<F>;
  }

  /// Получить значение поля F. Если в формате поле nullopt, то исключение.
  template<typename F>
  const typename F::value_type& get() const {
    constexpr std::size_t field_index = boost::mp11::mp_find<fields, F>::value;
    if (auto& v = std::get<field_index>(values_)) {
      return *v;
    }
    ED_THROW_EXCEPTION(ed::runtime_error("no value"));
  }

  /// Получить значение поля F. Если в формате поле nullopt, то вернется default_value.
  /// Если nullopt и нет default_value, то исключение.
  template<typename F>
  const typename F::value_type& get_or_default() const {
    constexpr std::size_t field_index = boost::mp11::mp_find<fields, F>::value;
    if (auto& v = std::get<field_index>(values_)) {
      return *v;
    }
    if constexpr (has_default_value_v<F>) {
      return F::default_value;
    }
    ED_THROW_EXCEPTION(ed::runtime_error("no value"));
  }

  template<typename F>
  std::optional<typename F::value_type> get_optional() const {
    constexpr std::size_t field_index = boost::mp11::mp_find<fields, F>::value;
    return std::get<field_index>(values_);
  }

  /// Задать значение поля F.
  template<typename F>
  format& set(std::optional<typename F::value_type>&& value) noexcept {
    constexpr std::size_t field_index = boost::mp11::mp_find<fields, F>::value;
    std::get<field_index>(values_) = std::move(value);
    return *this;
  }

  /// Проверка на пустоту всех значений.
  bool empty() const noexcept {
    return std::apply([](auto& ... opt_value) {
      return (!opt_value.has_value() && ...);
    }, values_);
  }

  /// Проверка на то, что все значения либо пусты, либо содержат default_value.
  bool is_default() const noexcept {
    bool result = true;
    boost::mp11::mp_for_each<fields>([&result, this](auto field) {
      using field_type = decltype(field);
      constexpr std::size_t field_index = boost::mp11::mp_find<fields, field_type>::value;
      auto& v = std::get<field_index>(values_);
      if constexpr (has_default_value_v<field_type>) {
        if (v && v != field_type::default_value) {
          result = false;
        }
      } else {
        if (v.has_value()) {
          result = false;
        }
      }
    });
    return result;
  }

  /// Возвращает формат у которого установлены только пересекающиется поля.
  format intersect(const format& fmt) const {
    format result;
    boost::mp11::mp_for_each<fields>([&result, &fmt, this](auto field) {
      using field_type = decltype(field);
      constexpr std::size_t field_index = boost::mp11::mp_find<fields, field_type>::value;
      if (holds<field_type>() && fmt.holds<field_type>()) {
        auto& v = get<field_type>();
        auto& fmt_v = fmt.get<field_type>();
        if (v == fmt_v) {
          std::get<field_index>(result.values_) = v;
        }
      }
    });
    return result;
  }

  /// Возвращает объединенный формат текущего с fmt. При конфликте, приоритет у текущего.
  template<typename ... F>
  format unite(const format<F ...>& fmt) const {
    format result;
    boost::mp11::mp_for_each<fields>([&result, &fmt, this](auto field) {
      using field_type = decltype(field);
      constexpr std::size_t result_index = boost::mp11::mp_find<fields, field_type>::value;
      if (auto& v = std::get<result_index>(values_)) {
        std::get<result_index>(result.values_) = v;
      } else {
        if constexpr (format<F ...>::template can_hold<field_type>()) {
          constexpr std::size_t fmt_index = boost::mp11::mp_find<typename format<F ...>::fields, field_type>::value;
          std::get<result_index>(result.values_) = std::get<fmt_index>(fmt.values_);
        }
      }
    });
    return result;
  }

  /// Возвращает формат с примененными изменениями.
  format apply(const changes& ch) const {
    format result;
    boost::mp11::mp_for_each<fields>([&ch, &result, this](auto field) {
      using field_type = decltype(field);
      constexpr std::size_t field_index = boost::mp11::mp_find<fields, field_type>::value;
      if (ch.flags_[field_index]) {
        std::get<field_index>(result.values_) = std::get<field_index>(ch.values_);
      } else {
        std::get<field_index>(result.values_) = std::get<field_index>(values_);
      }
    });
    return result;
  }

  changes as_changes() const {
    changes result;
    boost::mp11::mp_for_each<fields>([&result, this](auto field) {
      using field_type = decltype(field);
      constexpr std::size_t result_index = boost::mp11::mp_find<fields, field_type>::value;
      if (auto& v = std::get<result_index>(values_)) {
        std::get<result_index>(result.values_) = v;
        result.flags_.at(result_index) = true;
      }
    });
    return result;
  }

  friend std::size_t hash_value(const format& fmt) noexcept {
    return boost::hash_value(fmt.values_);
  }

  template<typename OStream>
  friend void write(OStream& os, const format& fmt) {
    using ed::write;
    write(os, fmt.values_);
  }

  template<typename IStream>
  friend void read(IStream& is, format& fmt) {
    using ed::read;
    read(is, fmt.values_);
  }

private:
  using opt_values = std::tuple<std::optional<typename T::value_type> ...>;

  opt_values values_;
};


using cell_format = format<
  number_format,
  text_horizontal_alignment,
  text_vertical_alignment,
  text_rotation,
  text_indent,
  text_wrap,
  font_name,
  font_size,
  font_weight,
  font_italic,
  font_underline,
  font_strike_out,
  font_script,
  font_background,
  font_foreground,
  fill_pattern_type,
  fill_background,
  fill_foreground,
  top_border,
  bottom_border,
  left_border,
  right_border
>;


using font_format = format<
  font_name,
  font_size,
  font_weight,
  font_italic,
  font_underline,
  font_strike_out,
  font_script,
  font_background,
  font_foreground
>;


using fill_format = format<
  fill_pattern_type,
  fill_background,
  fill_foreground
>;


using border_format = format<
  top_border,
  bottom_border,
  left_border,
  right_border
>;


template<typename Format>
ed::rasta::font make_font(const Format& fmt) {
  ed::rasta::font fnt;
  fnt.name(fmt.template get_or_default<font_name>());
  fnt.size(fmt.template get_or_default<font_size>());
  fnt.weight(fmt.template get_or_default<font_weight>());
  fnt.italic(fmt.template get_or_default<font_italic>());
  fnt.underline(fmt.template get_or_default<font_underline>());
  fnt.strike_out(fmt.template get_or_default<font_strike_out>());
  return fnt;
}


template<typename Format>
ed::rasta::text_layout::text_format make_text_format(const Format& fmt) {
  ed::rasta::text_layout::text_format tf;
  tf.fnt = make_font(fmt);
  tf.frg = fmt.template get_or_default<font_foreground>();
  tf.bkg = fmt.template get_or_default<font_background>();
  return tf;
}


template<typename Format>
Format make_format(const ed::rasta::font& fnt) {
  Format fmt;
  fmt.template set<font_name>(std::string(fnt.name()));
  fmt.template set<font_size>(fnt.size());
  fmt.template set<font_weight>(fnt.weight());
  fmt.template set<font_italic>(fnt.is_italic());
  fmt.template set<font_underline>(fnt.is_underline());
  fmt.template set<font_strike_out>(fnt.is_strike_out());
  return fmt;
}


template<typename Format>
Format make_format(const ed::rasta::text_layout::text_format& tf) {
  Format fmt = make_format<Format>(tf.fnt);
  fmt.template set<font_foreground>(tf.frg);
  fmt.template set<font_background>(tf.bkg);
  return fmt;
}


} // namespace lde::cellfy::boox
