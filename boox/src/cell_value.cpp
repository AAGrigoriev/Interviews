#include <lde/cellfy/boox/cell_value.h>

#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_map>

#include <boost/lexical_cast.hpp>

#include <ed/core/aggregate_equal.h>
#include <ed/core/assert.h>
#include <ed/core/math.h>
#include <ed/core/unicode.h>


namespace lde::cellfy::boox {

namespace _ {
namespace {


// https://www.ablebits.com/office-addins-blog/2015/03/11/change-date-format-excel/#excel-date-format
const boost::gregorian::date epoch(1900, boost::date_time::Jan, 1);


double ptime_to_double(boost::posix_time::ptime v) noexcept {
  double result = static_cast<double>((v.date() - epoch).days() + 2);
  result += static_cast<double>(v.time_of_day().total_milliseconds()) /
    static_cast<double>(boost::posix_time::time_duration(24, 0, 0).total_milliseconds());
  return result;
}


// Приоритет типов.
// Ошибки -> bool -> строки(rich_text, string) -> числа -> пустые.
auto& get_type_precedence() {
  const static std::unordered_map<cell_value_type, std::size_t> type_precedence {
    {cell_value_type::none,      0},
    {cell_value_type::number,    1},
    {cell_value_type::string,    2},
    {cell_value_type::rich_text, 2},
    {cell_value_type::boolean,   3},
    {cell_value_type::error,     4}
  };
  return type_precedence;
}

}} // namespace _


bool text_run::operator==(const text_run& rhs) const noexcept {
  if (text == rhs.text) {
    return ed::aggregate_equal(format, rhs.format);
  }
  return false;
}


cell_value::cell_value() noexcept
  : data_(nullptr) {
}


cell_value::cell_value(bool v) noexcept
  : data_(v) {
}


cell_value::cell_value(double v) noexcept
  : data_(v) {
}


cell_value::cell_value(boost::posix_time::ptime v) noexcept
  : data_(_::ptime_to_double(v)) {
}


cell_value::cell_value(boost::gregorian::date v) noexcept
  : data_(_::ptime_to_double(boost::posix_time::ptime(v))) {
}


cell_value::cell_value(boost::posix_time::time_duration v) noexcept
  : data_(static_cast<double>(v.total_milliseconds()) / boost::posix_time::time_duration(24, 0, 0).total_milliseconds()) {
}


cell_value::cell_value(std::wstring v) noexcept
  : data_(std::move(v)) {
}


cell_value::cell_value(const wchar_t* v) noexcept {
  if (v) {
    data_ = std::wstring(v);
  }
}


cell_value::cell_value(std::string v) noexcept
  : data_(ed::from_utf8(v)) {
}


cell_value::cell_value(const char* v) noexcept {
  if (v) {
    data_ = ed::from_utf8(v);
  }
}


cell_value::cell_value(rich_text v) noexcept
  : data_(std::move(v)) {
}


cell_value::cell_value(cell_value_error v) noexcept
  : data_(v) {
}


bool cell_value::operator==(const cell_value& rhs) const noexcept {
  if (auto l = std::get_if<double>(&data_), r = std::get_if<double>(&rhs.data_); l && r) {
    return ed::fuzzy_equal(*l, *r);
  }
  return data_ == rhs.data_;
}


bool cell_value::operator<(const cell_value& rhs) const {
  const auto lhs_priority = _::get_type_precedence().at(type());
  const auto rhs_priority = _::get_type_precedence().at(rhs.type());

  // Сначала проверяются приоритеты, потом значения.
  if (lhs_priority < rhs_priority) {
    return true;
  } else if (lhs_priority > rhs_priority) {
    return false;
  } else { // Если одинаковые приоритеты.
    return std::visit([&rhs, this](auto&& l_data, auto&& r_data) {
      using lt = std::decay_t<decltype(l_data)>;
      using rt = std::decay_t<decltype(r_data)>;
      if constexpr (std::is_same_v<lt, std::nullptr_t> && std::is_same_v<rt, std::nullptr_t>) {
        return false;
      } else if constexpr (std::is_same_v<lt, double> && std::is_same_v<rt, double>) {
        return l_data < r_data;
      } else if constexpr (std::is_same_v<lt, std::wstring> && std::is_same_v<rt, std::wstring>) {
        return l_data < r_data;
      } else if constexpr (std::is_same_v<lt, std::wstring> && std::is_same_v<rt, rich_text>) {
        return l_data < rhs.to<std::wstring>();
      } else if constexpr (std::is_same_v<lt, rich_text> && std::is_same_v<rt, std::wstring>) {
        return to<std::wstring>() < r_data;
      } else if constexpr (std::is_same_v<lt, rich_text> && std::is_same_v<rt, rich_text>) {
        return to<std::wstring>() < rhs.to<std::wstring>();
      } else if constexpr (std::is_same_v<lt, bool> && std::is_same_v<rt, bool>) {
        return l_data != r_data && !l_data; // "Истина" по приоритетам выше чем "Ложь".
      } else if constexpr (std::is_same_v<lt, cell_value_error> && std::is_same_v<rt, cell_value_error>) {
        return false;
      } else {
        ED_ASSERT(false); // Когда приоритеты разные. Должны отработать условия выше.
        return false;
      }
    }, data_, rhs.data_);
  }
}


cell_value_type cell_value::type() const noexcept {
  cell_value_type result = cell_value_type::none;

  if (is<double>()) {
    result = cell_value_type::number;
  } else if (is<bool>()) {
    result = cell_value_type::boolean;
  } else if (is<std::wstring>()) {
    result = cell_value_type::string;
  } else if (is<rich_text>()) {
    if (!as<rich_text>().empty()) {
      result = cell_value_type::rich_text;
    }
  } else if (is<cell_value_error>()) {
    result = cell_value_type::error;
  }

  return result;
}


bool cell_value::is_none() const noexcept {
  return type() == cell_value_type::none;
}


bool cell_value::is_nil() const noexcept {
  return is<std::nullptr_t>();
}


template<>
bool cell_value::to<bool>() const {
  if (is<std::nullptr_t>()) {
    return false;
  } else if (is<bool>()) {
    return as<bool>();
  } else if (is<double>()) {
    return !ed::fuzzy_is_null(as<double>());
  } else if (is<std::wstring>()) {
    // TODO: Это не точно.
    return !ed::fuzzy_is_null(std::stod(as<std::wstring>()));
  } else if (is<rich_text>()) {
    // TODO: Это не точно.
    return !ed::fuzzy_is_null(std::stod(to<std::wstring>()));
  } else if (is<cell_value_error>()) {
    ED_THROW_EXCEPTION(bad_value_cast() << cell_value_error_info(as<cell_value_error>()));
  }
  ED_THROW_EXCEPTION(bad_value_cast());
}


template<>
double cell_value::to<double>() const {
  if (is<std::nullptr_t>()) {
    return 0.;
  } else if (is<bool>()) {
    return as<bool>() ? 1. : 0.;
  } else if (is<double>()) {
    return as<double>();
  } else if (is<std::wstring>()) {
    return std::stod(as<std::wstring>());
  } else if (is<rich_text>()) {
    return std::stod(to<std::wstring>());
  } else if (is<cell_value_error>()) {
    ED_THROW_EXCEPTION(bad_value_cast() << cell_value_error_info(as<cell_value_error>()));
  }
  ED_THROW_EXCEPTION(bad_value_cast());
}


template<>
boost::posix_time::ptime cell_value::to<boost::posix_time::ptime>() const {
  double integral = 0;
  double decimal = std::modf(to<double>(), &integral);
  return {
    _::epoch + boost::gregorian::date_duration(static_cast<long long>(integral) - 2),
    boost::posix_time::milliseconds(
      static_cast<long long>(decimal * boost::posix_time::time_duration(24, 0, 0).total_milliseconds()))
  };
}


template<>
boost::gregorian::date cell_value::to<boost::gregorian::date>() const {
  return to<boost::posix_time::ptime>().date();
}


template<>
boost::posix_time::time_duration cell_value::to<boost::posix_time::time_duration>() const {
  return boost::posix_time::milliseconds(
    static_cast<long long>(to<double>() * boost::posix_time::time_duration(24, 0, 0).total_milliseconds()));
}


template<>
std::wstring cell_value::to<std::wstring>() const {
  if (is<std::nullptr_t>()) {
    return std::wstring();
  } else if (is<bool>()) {
    return as<bool>() ? L"TRUE" : L"FALSE";
  } else if (is<double>()) {
    std::wostringstream stream;
    stream << std::fixed << as<double>();
    auto str = stream.str();
    if (auto pos = str.find('.'); pos != std::wstring::npos) {
      str.erase(str.find_last_not_of('0') + 1);
      if (pos == str.size() - 1) {
        str.pop_back();
      }
    }
    return str;
  } else if (is<std::wstring>()) {
    return as<std::wstring>();
  } else if (is<rich_text>()) {
    std::size_t size = 0;
    for (auto& run : as<rich_text>()) {
      size += run.text.size();
    }
    std::wstring str;
    str.reserve(size);
    for (auto& run : as<rich_text>()) {
      str += run.text;
    }
    return str;
  } else if (is<cell_value_error>()) {
    switch (as<cell_value_error>()) {
      case cell_value_error::div0:  return L"#DIV/0!";
      case cell_value_error::na:    return L"#N/A";
      case cell_value_error::name:  return L"#NAME?";
      case cell_value_error::null:  return L"#NULL!";
      case cell_value_error::num:   return L"#NUM!";
      case cell_value_error::ref:   return L"#REF!";
      case cell_value_error::value: return L"#VALUE!";
    }
  }
  ED_THROW_EXCEPTION(bad_value_cast());
}


template<>
std::string cell_value::to<std::string>() const {
  return ed::to_utf8<std::string>(to<std::wstring>());
}


} // namespace lde::cellfy::boox
