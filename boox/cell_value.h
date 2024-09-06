#pragma once


#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/operators.hpp>

#include <lde/cellfy/boox/enums.h>
#include <lde/cellfy/boox/exception.h>
#include <lde/cellfy/boox/format.h>
#include <lde/cellfy/boox/vector_2d.h>


namespace lde::cellfy::boox {


struct text_run :
  public boost::equality_comparable<text_run> {

  std::wstring text;
  font_format  format;

  bool operator==(const text_run& rhs) const noexcept;
};

using rich_text = std::vector<text_run>;


class cell_value final :
  public boost::equality_comparable<cell_value>,
  public boost::less_than_comparable<cell_value> {

public:
  using list   = boost::container::small_vector<cell_value, 10>;
  using matrix = vector_2d<cell_value>;

public:
  cell_value() noexcept;
  cell_value(bool v) noexcept;
  cell_value(double v) noexcept;
  cell_value(boost::posix_time::ptime v) noexcept;
  cell_value(boost::gregorian::date v) noexcept;
  cell_value(boost::posix_time::time_duration v) noexcept;
  cell_value(std::wstring v) noexcept;
  cell_value(const wchar_t* v) noexcept;
  cell_value(std::string v) noexcept;
  cell_value(const char* v) noexcept;
  cell_value(rich_text v) noexcept;
  cell_value(cell_value_error v) noexcept;

  bool operator==(const cell_value& rhs) const noexcept;

  bool operator<(const cell_value& rhs) const;

  /// Тип значения.
  cell_value_type type() const noexcept;

  /// Проверка типа на cell_value_type::none.
  bool is_none() const noexcept;

  /// Проверка на nullptr.
  bool is_nil() const noexcept;

  /// Проверка типа содержащегося в data_.
  template<typename T>
  bool is() const noexcept;

  /// Получение данных требуемого типа.
  template<typename T>
  const T& as() const;

  /// Получение данных требуемого типа.
  template<typename T>
  T& as();

  /// Преобразование к требуемому типу.
  template<typename T>
  T to() const;

private:
  std::variant<
    std::nullptr_t,
    bool,
    double,
    std::wstring,
    rich_text,
    cell_value_error
  > data_;
};


template<typename T>
bool cell_value::is() const noexcept {
  return std::holds_alternative<T>(data_);
}


template<typename T>
const T& cell_value::as() const {
  return std::get<T>(data_);
}


template<typename T>
T& cell_value::as() {
  return std::get<T>(data_);
}


template<typename T>
T cell_value::to() const {
  if constexpr (std::is_arithmetic_v<T>) {
    return static_cast<T>(to<double>());
  }
  ED_THROW_EXCEPTION(bad_value_cast());
}


template<>
bool cell_value::to<bool>() const;

template<>
double cell_value::to<double>() const;

template<>
boost::posix_time::ptime cell_value::to<boost::posix_time::ptime>() const;

template<>
boost::gregorian::date cell_value::to<boost::gregorian::date>() const;

template<>
boost::posix_time::time_duration cell_value::to<boost::posix_time::time_duration>() const;

template<>
std::wstring cell_value::to<std::wstring>() const;

template<>
std::string cell_value::to<std::string>() const;


} // namespace lde::cellfy::boox
