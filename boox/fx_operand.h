#pragma once


#include <boost/container/small_vector.hpp>

#include <lde/cellfy/boox/cell_value.h>
#include <lde/cellfy/boox/exception.h>
#include <lde/cellfy/boox/range.h>


namespace lde::cellfy::boox::fx {


using range_list = boost::container::small_vector<range, 10>;


class operand final {
public:
  using list = boost::container::small_vector<operand, 10>;

public:
  operand() = default;

  template<typename T>
  operand(T&& v);

  template<typename T>
  bool is() const noexcept;

  template<typename T>
  const T& as() const;

  template<typename T>
  T& as();

  template<typename T>
  T to() const;

private:
  std::variant<
    cell_value,
    range_list
  > data_;
};


template<typename T>
operand::operand(T&& v)
  : data_(std::forward<T>(v)) {
}


template<typename T>
bool operand::is() const noexcept {
  if constexpr (std::is_same_v<T, cell_value> || std::is_same_v<T, range_list>) {
    return std::holds_alternative<T>(data_);
  } else {
    if (std::holds_alternative<cell_value>(data_)) {
      return std::get<cell_value>(data_).is<T>();
    } else {
      return false;
    }
  }
}


template<typename T>
const T& operand::as() const {
  return std::get<T>(data_);
}


template<typename T>
T& operand::as() {
  return std::get<T>(data_);
}


template<typename T>
T operand::to() const {
  if (is<cell_value>()) {
    if constexpr (std::is_same_v<T, cell_value>) {
      return as<cell_value>();
    } else {
      return as<cell_value>().to<T>();
    }
  } else if (is<range_list>()) {
    if constexpr (std::is_same_v<T, range_list>) {
      return as<range_list>();
    } else {
      ED_EXPECTS(!as<range_list>().empty());
      cell_value v = as<range_list>().front().top_left().value();
      if constexpr (std::is_same_v<T, cell_value>) {
        return v;
      } else {
        return v.to<T>();
      }
    }
  }
  ED_THROW_EXCEPTION(bad_value_cast());
}


} // namespace lde::cellfy::boox::fx
