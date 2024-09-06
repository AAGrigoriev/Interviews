#include <lde/cellfy/boox/src/value_parser.h>

#include <cmath>
#include <ctime>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <boost/date_time.hpp>
#include <boost/format.hpp>
#include <boost/spirit/home/x3.hpp>

#include <ed/core/assert.h>


namespace lde::cellfy::boox  {

namespace _ {
namespace {

struct date_time final {
  using time_p = std::pair<int, std::wstring>;
  time_p year;
  time_p month;
  time_p day;
  time_p hours;
  time_p minute;
  time_p sec;
  wchar_t date_separator;


  date_time()
    : year{static_cast<int>(boost::date_time::day_clock<boost::gregorian::date>::local_day_ymd().year) - 1900, {}}
    , month{0, {}}
    , day{1, {}}
    , hours{0, {}}
    , minute{0, {}}
    , sec{0, {}}
    , date_separator{L'.'} {
  }

  void clear_format() {
    year.second.clear();
    month.second.clear();
    day.second.clear();
    hours.second.clear();
    minute.second.clear();
    sec.second.clear();
  }
};


struct duration final {
  std::optional<unsigned> hours;
  std::optional<unsigned> min;
  std::optional<unsigned> sec;
  std::optional<unsigned> milliseconds;
};


template <typename T>
struct delimeter_real_polices final : boost::spirit::x3::real_policies<T> {
  template <typename Iterator>
  bool parse_dot(Iterator& first, const Iterator& last) const {
    if (first == last || *first != dot) {
      return false;
    }
    ++first;
    return true;
  }

  template <typename Iterator>
  bool parse_exp(Iterator&, const Iterator&) const {
    return false;
  }

  template <typename Iterator, typename Attribute>
  bool parse_exp_n(Iterator&, const Iterator&, Attribute&) const {
    return false;
  }

  wchar_t dot = L',';
};


// Определяем по десятичной дроби с определённой точностью обыкновенную дробь.
// Сначала находим дробь между [Числ/знам;Числ/знам - 1], если удовлетворяет точности, то завершаем.
// Иначе, повторяем до подходящей двухзначной дроби.
std::pair<std::size_t, std::size_t> brut_frac(double desired_fraction, double eps) {
  std::size_t numerator = 1;
  std::size_t denominator = 1;
  std::size_t multiplier = 2;

  auto calc_approx_frac = [desired_fraction, eps](auto&& num, auto&& den) {
    double temp_frac = 0.;
    do {
      ++den;
      temp_frac = static_cast<double>(num) / den;
    } while ((desired_fraction - temp_frac) < 0);

    if ((desired_fraction - temp_frac) < eps) {
      return true;
    }

    --den;
    temp_frac = static_cast<double>(num) / den;
    if ((desired_fraction - temp_frac) > -eps) {
      return true;
    }

    return false;
  };

  // Первый поиск
  if (calc_approx_frac(numerator, denominator)) {
    return {numerator, denominator};
  } else { // Уточнение
    std::size_t more_precise_num = 0;
    std::size_t more_precise_den = 0;

    while (more_precise_num <= 99 && more_precise_den <= 99) {
      more_precise_num = numerator * multiplier;
      more_precise_den = denominator * multiplier;
      if (calc_approx_frac(more_precise_num, more_precise_den)) {
        break;
      }
      ++multiplier;
    }
    return {more_precise_num, more_precise_den};
  }
}


bool validate_dmy(const date_time& date) {
  std::tm t = {
    .tm_mday = date.day.first,
    .tm_mon  = date.month.first,
    .tm_year = date.year.first
  };
  std::tm copy = t;
  std::mktime(&copy);
  return t.tm_mday == copy.tm_mday &&
         t.tm_mon  == copy.tm_mon &&
         t.tm_year == copy.tm_year;
}


std::pair<cell_value, std::wstring> cell_value_from_time(const date_time& time) {
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime ptime{
    date{
      static_cast<date::year_type>(time.year.first + 1900),
      static_cast<date::month_type>(time.month.first + 1),
      static_cast<date::day_type>(time.day.first)
    },
    time_duration{
      static_cast<time_duration::hour_type>(time.hours.first),
      static_cast<time_duration::min_type>(time.minute.first),
      static_cast<time_duration::sec_type>(time.sec.first)
    }
  };

  std::wstring result;

  if (!time.day.second.empty()) {
    result += time.day.second + time.date_separator;
  }

  // Месяц всегда должен присутсвовать. Т.е не может быть строки dd.yyyy, dd, yyyy
  ED_ASSERT(!time.month.second.empty());
  result += time.month.second;

  if (!time.year.second.empty()) {
    result += time.date_separator + time.year.second;
  }

  auto time_of_day = ptime.time_of_day();
  if (time_of_day.seconds() != 0) {
    result += L' ' + time.hours.second + L":mm:" + time.sec.second;
  } else if (time_of_day.minutes() != 0) {
    result += L' ' + time.hours.second + L":" + time.minute.second + L":ss";
  } else if (time_of_day.hours() != 0) {
    result += L' ' + time.hours.second + L":mm";
  }

  return {ptime, std::move(result)};
}


std::pair<cell_value, std::wstring> cell_value_from_duration(const duration& time) {
  using namespace boost::posix_time;

  time_duration dur_out{
    static_cast<time_duration::hour_type>(time.hours.value_or(0)),
    static_cast<time_duration::min_type>(time.min.value_or(0)),
    static_cast<time_duration::sec_type>(time.sec.value_or(0)),
    static_cast<time_duration::fractional_seconds_type>(time.milliseconds.value_or(0) * 1000)
  };

  std::wstring result;
  std::wstring hours_format = dur_out.hours() < 10 ? L"h:"
                                                   : dur_out.hours() < 24 ? L"hh:"
                                                                          : L"[h]:";
  if (time.sec) {
    result += hours_format + L"mm:ss";
  } else if (time.min) {
    result += hours_format + L"mm:ss";
  } else if (time.hours) {
    result += hours_format + L"mm";
  }

  return {dur_out, std::move(result)};
}


auto create_date_rule() {
  using namespace boost::spirit::x3;

  // День и месяц имеют формат XX. (День 1 - 31, Месяц 1 - 12).
  uint_parser<unsigned int, 10, 1, 2> day_month_uint_parser;
  // Год имеет формат XX-XXXX. (Год от 1900, до 9999).
  uint_parser<unsigned int, 10, 2, 4> year_uint_parser;
  // Час от 0 - 23, Минуты от 0 до 59, Секунды от 0 до 59.
  uint_parser<unsigned int, 10, 1, 2> hour_min_sec_parser;

  // Длинные названия месяцев имеют Р.П/И.П и строку форматирования mmmm.
  const auto long_m_name_rule = no_case[standard_wide::symbols<unsigned int>({
    {L"Январь",   0},
    {L"Января",   0},
    {L"Февраль",  1},
    {L"Февраля",  1},
    {L"Март",     2},
    {L"Марта",    2},
    {L"Апрель",   3},
    {L"Апреля",   3},
    {L"Май",      4},
    {L"Мая",      4},
    {L"Июнь",     5},
    {L"Июль",     6},
    {L"Июня",     5},
    {L"Июля",     6},
    {L"Август",   7},
    {L"Августа",  7},
    {L"Сентябрь", 8},
    {L"Сентября", 8},
    {L"Октябрь",  9},
    {L"Октября",  9},
    {L"Ноябрь",   10},
    {L"Ноября",   10},
    {L"Декабрь",  11},
    {L"Декабря",  11}
  })];

  // Короткие названия месяцев имеют строку форматирования mmm.
  const auto short_m_name_rule = no_case[standard_wide::symbols<unsigned int>({
    {L"Янв",      0},
    {L"Фев",      1},
    {L"Февр",     1},
    {L"Мар",      2},
    {L"Апр",      3},
    {L"Июн",      5},
    {L"Июл",      6},
    {L"Авг",      7},
    {L"Сент",     8},
    {L"Сен",      8},
    {L"Окт",      9},
    {L"Ноя",      10},
    {L"Нояб",     10},
    {L"Дек",      11}
  })];

  const auto d_rule =
    day_month_uint_parser[([](auto& ctx) {
      auto& time = get<date_time>(ctx);
      time.clear_format();
      const auto attr = _attr(ctx);
      if (attr > 0 && attr < 32) {
        time.day.first = attr;
        time.day.second = L"dd";
      } else {
        _pass(ctx) = false;
      }
    })];

  const auto m_rule =
    day_month_uint_parser[([](auto& ctx) {
      auto& time = get<date_time>(ctx);
      const auto attr = _attr(ctx);
      if (attr > 0 && attr < 13) {
        time.month.first = attr - 1;
        time.month.second = L"mm";
        time.date_separator = L'.';
      } else {
        _pass(ctx) = false;
      }
    })]
    |
    long_m_name_rule[([](auto& ctx) {
      auto& time = get<date_time>(ctx);
      time.month.first = _attr(ctx);
      time.month.second = L"mmmm";
      time.date_separator = L' ';
    })]
    |
    short_m_name_rule[([](auto& ctx) {
      auto& time = get<date_time>(ctx);
      time.month.first = _attr(ctx);
      time.month.second = L"mmm";
      time.date_separator = L' ';
    })];

  const auto year_rule =
    year_uint_parser[([](auto& ctx) {
      auto& time = get<date_time>(ctx);
      const auto attr = _attr(ctx);
      time.year.second = L"yyyy";
      if (attr < 30) {
        time.year.first = 100 + attr; // 10.10.20 -> 10.10.2020
      } else if (attr < 100) {
        time.year.first = attr;       // 10.10.70 -> 10.10.1970
      } else if (attr >= 1900 && attr <= 9999) {
        time.year.first = attr - 1900;
      } else {
        _pass(ctx) = false;
      }
    })];

  const auto long_year_rule =
    year_uint_parser[([](auto& ctx) {
      auto& time = get<date_time>(ctx);
      const auto attr = _attr(ctx);
      if (attr >= 1900 && attr <= 9999) {
        time.year.first = attr - 1900;
        time.year.second = L"yyyy";
      } else {
        _pass(ctx) = false;
      }
    })];

  const auto hour_callback = [](auto& ctx) {
    auto& time = get<date_time>(ctx);
    auto attr = _attr(ctx);
    time.hours.first = attr;
    time.hours.second = attr < 10 ? L"h" : L"hh";
  };

  const auto min_callback = [](auto& ctx) {
    auto& time = get<date_time>(ctx);
    time.minute.first = _attr(ctx);
    time.minute.second = L"mm";
  };

  const auto sec_callback = [](auto& ctx) {
    auto& time = get<date_time>(ctx);
    time.sec.first = _attr(ctx);
    time.sec.second = L"ss";
  };

  const auto sep = (char_(L"./"));

  const auto time_rule = hour_min_sec_parser[hour_callback] >> char_(L':') >>
    -(hour_min_sec_parser[min_callback] >> -(char_(L":") >>
    -hour_min_sec_parser[sec_callback]));
  // Жадный поиск. TODO: ускорить.
  return (m_rule >> sep >> long_year_rule >> -(space >> time_rule)) |
         (d_rule >> sep >> m_rule >> -(sep >> year_rule >> -(space >> time_rule)));
}


auto create_duration_rule() {
  using namespace boost::spirit::x3;

  // Час от 0 - xxxx, Минуты от 0 до xxxx, Секунды от 0 до xxxx.
  uint_parser<unsigned int, 10, 1, 8> hour_min_sec_parser;
  // Миллисекунды от 000 - 999.
  uint_parser<unsigned int, 10, 1, 3> millisecond_parser;

  const auto hour_callback = [](auto& ctx) {
    get<duration>(ctx).hours = _attr(ctx);
  };

  const auto min_callback = [](auto& ctx) {
    get<duration>(ctx).min = _attr(ctx);
  };

  const auto sec_callback = [](auto& ctx) {
    get<duration>(ctx).sec = _attr(ctx);
  };

  const auto millisecond_callback = [](auto& ctx) {
    get<duration>(ctx).milliseconds = _attr(ctx);
  };

  return hour_min_sec_parser[hour_callback] >> char_(L':') >>
         -(hour_min_sec_parser[min_callback] >> -(char_(L":") >>
         -hour_min_sec_parser[sec_callback] >> -(char_(L".") >>
          millisecond_parser[millisecond_callback])));
};


auto create_number_rule(std::wstring& number_format, const std::locale& loc) {
  using namespace boost::spirit::x3;

  real_parser<double, _::delimeter_real_polices<double>> ts_real;
  ts_real.policies.dot = std::use_facet<std::numpunct<wchar_t>>(loc).decimal_point();

  const auto number_callback  = [](auto& ctx) {
    get<double>(ctx) = _attr(ctx);
  };

  const auto percent_callback = [&number_format](auto& ctx) {
    number_format = L"0.00%";
    get<double>(ctx) /= 100;
  };

  const auto post_currency_callback = [&number_format](auto& ctx) {
    number_format = L"#,##0.00 ";
    number_format += _attr(ctx);
  };

  const auto pref_currency_callback = [&number_format](auto& ctx) {
    number_format = _attr(ctx);
    number_format += L" #,##0.00";
  };

  const auto exponent_callback = [&number_format](auto& ctx) {
    number_format = L"0.00E+00";
    get<double>(ctx) *= std::pow(10, _attr(ctx));
  };

  // Форматы :
  // - Екcпоненциальный double[eE]int
  // - Валюта double [£$€¥₽CHF] // TODO: €¥£CHF
  // - Процент double [%]
  return skip(space)[char_(L"$")[pref_currency_callback] >> ts_real[number_callback] |
         ts_real[number_callback] >> -((no_case[char_(L"e")] >> int_[exponent_callback]) | char_(L"%")[percent_callback] | char_(L"$")[post_currency_callback])];
}


value_parser::value_with_format concat_fraction(int integer_part, unsigned numerator, unsigned denominator) {
  const double result_value = static_cast<double>(numerator) / denominator;
  std::wstring result_format;

  result_format = L"#\" \"";
  double iptr = 0.;
  if (double frac_part = std::modf(result_value, &iptr); frac_part > 0) {
    const auto res = brut_frac(frac_part, 0.0001);
    result_format += (boost::wformat(L"%1%/%2%") %
      std::wstring(static_cast<int>(std::log10(res.first)) + 1, L'?') %
      std::wstring(static_cast<int>(std::log10(res.second)) + 1, L'?')).str();
  } else {
    result_format += L"?/?";
  }

  return {{integer_part < 0 ? -result_value + integer_part : result_value + integer_part}, std::move(result_format)};
}


auto check_fraction(int integer_part, unsigned numerator, unsigned denominator) {
  return numerator <= 99 && denominator <= 99 ? std::make_optional(concat_fraction(integer_part, numerator, denominator))
                                              : std::nullopt;
}


auto create_bool_rule() {
  using namespace boost::spirit::x3;

  return no_case[standard_wide::symbols<unsigned int>({{L"true", 1}, {L"false", 0}})] >> eoi;
}


namespace x3_rule {
using namespace boost::spirit::x3;

struct result final {
  std::optional<int> integer_part;
  unsigned           numerator;
  unsigned           denominator;
};

const auto add_integer_part = [](auto& ctx) {
  _val(ctx).integer_part = _attr(ctx);
};

const auto add_fraction_part = [](auto& ctx) {
  _val(ctx).numerator = boost::fusion::at_c<0>(_attr(ctx));
  _val(ctx).denominator = boost::fusion::at_c<1>(_attr(ctx));
};

const auto integer_part = int_ >> omit[space];

const auto fraction =  uint_ >> omit[char_(L"/")] >> uint_;

const rule<class mixed_fraction, result> mixed_fraction = "mixed_fraction_rule";

const auto mixed_fraction_def = integer_part[add_integer_part] >> fraction[add_fraction_part] >> eoi;

BOOST_SPIRIT_DEFINE(mixed_fraction);


const rule<class opt_fraction, result> opt_mixed_fraction = "opt_fraction_rule";

const auto opt_mixed_fraction_def = fraction[add_fraction_part] >> eoi | integer_part[add_integer_part] >> fraction[add_fraction_part] >> eoi;

BOOST_SPIRIT_DEFINE(opt_mixed_fraction);

} // namespace x3_rule


// Не все данные, которые находит парсер имеют форматы.
// Например: Число, строка.
template<typename T>
using pair = std::pair<T, std::wstring>;

// Может хранить : дату, процент, строку.
using variant = std::variant<date_time, duration, pair<double>, std::wstring, x3_rule::result, bool>;


template <typename Pair_type, typename F, typename S>
auto create_variant_from_pair(F&& f, S&& s) {
  return variant{std::in_place_type<Pair_type>,
    std::piecewise_construct,
    std::forward_as_tuple(std::forward<F>(f)),
    std::forward_as_tuple(std::forward<S>(s))};
}


/// @brief Парсим строку и получаем дату, процент c форматом, если он есть.
/// Ищем dd mm yyyy, dd mmmm yyyy, dd mm, dd mmmm, mmmm yyyy, mm yyyy, double%.
/// Если ничего не найдено, возвращаем std::variant с пустой строкой.
variant x3_parser(const std::wstring& text, bool mixed_fraction_search, const std::locale& loc) {
  using namespace boost::spirit::x3;

  std::wstring number_format;
  const auto date_rule     = create_date_rule();
  const auto duration_rule = create_duration_rule();
  const auto number_rule   = create_number_rule(number_format, loc);
  const auto bool_rule     = create_bool_rule();

  _::date_time time;
  _::duration dur;
  double number;
  bool bool_result;
  x3_rule::result fraction_result;
  decltype (text.begin()) beg;

  // Ищем дробь. Есть два паттерна поиска. 1 - Ищем целую часть и дробную. 2 - Целая часть опциональна, но дробную так же ищем.
  if (mixed_fraction_search ? parse(beg = text.begin(), text.end(), x3_rule::opt_mixed_fraction, fraction_result)
                            : parse(beg = text.begin(), text.end(), x3_rule::mixed_fraction, fraction_result) && beg == text.end()) {
    return fraction_result;
  // Ищем дату.
  } else if (parse(beg = text.begin(), text.end(), with<_::date_time>(time)[date_rule]) && beg == text.end()) {
    return time;
  // Ищем продолжительность.
  } else if (parse(beg = text.begin(), text.end(), with<_::duration>(dur)[duration_rule]) && beg == text.end()) {
    return dur;
  // Ищем число/процент.
  } else if (parse(beg = text.begin(), text.end(), with<double>(number)[number_rule]) && beg == text.end()) {
    return create_variant_from_pair<pair<double>>(number, std::move(number_format));
  } else if (parse(beg = text.begin(), text.end(), bool_rule, bool_result)) {
    return bool_result;
  }
  return std::wstring{};
}

}} // namespace _::



value_parser::value_parser(const std::locale& loc)
 : loc_(loc) {
}


value_parser::value_with_format value_parser::operator()(const std::wstring& text, bool mixed_fraction_search) const {
  return std::visit (
    [&text](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      using boost::gregorian::date;
      if constexpr (std::is_same_v<T, _::date_time>) {
        return _::validate_dmy(arg) ? _::cell_value_from_time(arg)
                                    : value_with_format{text, {}};
        return value_with_format{text, {}};
      } else if constexpr (std::is_same_v<T, _::duration>) {
        return _::cell_value_from_duration(arg);
      } else if constexpr (std::is_same_v<T, _::pair<double>>) {
        return value_with_format{arg.first, std::move(arg.second)};
      } else if constexpr (std::is_same_v<T, std::wstring>) {
        return value_with_format{text, {}};
      } else if constexpr (std::is_same_v<T, _::x3_rule::result>) {
        return _::check_fraction(arg.integer_part.value_or(0), arg.numerator, arg.denominator).value_or(value_with_format{text, {}});
      } else if constexpr (std::is_same_v<T, bool>) {
        return value_with_format(arg, {});
      }
    }, _::x3_parser(text, mixed_fraction_search, loc_));
}

} // namespace lde::cellfy::boox
