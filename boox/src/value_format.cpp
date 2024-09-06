#include <lde/cellfy/boox/value_format.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <type_traits>

#include <boost/algorithm/find_backward.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/spirit/home/x3.hpp>

#include <ed/core/assert.h>
#include <ed/core/math.h>
#include <ed/core/unicode.h>

#include <lde/cellfy/boox/cell_value.h>


using namespace boost::spirit;
using x3::_attr;
using x3::_val;
using x3::_where;
using x3::ascii::string;
using x3::double_;
using x3::inf;
using x3::lexeme;
using x3::lit;
using x3::no_case;
using x3::raw;
using x3::repeat;
using x3::skip;
using x3::unicode::char_;
using x3::unicode::space;
using x3::with;
using x3::hex;


namespace lde::cellfy::boox {
namespace _ {
namespace {


enum class token_type {
  string,        // \char "..." $ + - / ( ) : ! ^ & ' ~ { } < > = space
  number_sign,   // #
  zero,          // 0
  question_mark, // ?
  at_sign,       // @
  period,        // .
  comma,         // ,
  percent,       // %
  e,             // E+ e+ E- e-
  slash,         // /
  asterisk,      // *
  underscore,    // _
  yy,            // y yy
  yyyy,          // yyy yyyy+
  m,             // m
  mm,            // mm
  mmm,           // mmm
  mmmm,          // mmmm mmmmmm+
  mmmmm,         // mmmmm
  d,             // d
  dd,            // dd
  ddd,           // ddd
  dddd,          // dddd+
  h,             // h
  hh,            // hh+
  s,             // s
  ss,            // ss
  h_sqb,         // [h+]
  m_sqb,         // [m+]
  s_sqb,         // [s+]
  am_pm          // a/p am/pm
};

struct token {
  token_type   type;
  std::wstring text;
};


using token_vector = boost::container::small_vector<token, 32>;
using token_range  = boost::iterator_range<token_vector::const_iterator>;


bool is_date_part(const token& tok) noexcept {
  return tok.type == token_type::yy ||
         tok.type == token_type::yyyy ||
         tok.type == token_type::m ||
         tok.type == token_type::mm ||
         tok.type == token_type::mmm ||
         tok.type == token_type::mmmm ||
         tok.type == token_type::mmmmm ||
         tok.type == token_type::d ||
         tok.type == token_type::dd ||
         tok.type == token_type::ddd ||
         tok.type == token_type::dddd ||
         tok.type == token_type::h ||
         tok.type == token_type::hh ||
         tok.type == token_type::s ||
         tok.type == token_type::ss ||
         tok.type == token_type::am_pm;
}


bool is_duration_part(const token& tok) noexcept {
  return tok.type == token_type::h_sqb ||
         tok.type == token_type::m_sqb ||
         tok.type == token_type::s_sqb;
}


bool is_digit_placeholder(const token& tok) noexcept {
  return tok.type == token_type::number_sign ||
         tok.type == token_type::zero ||
         tok.type == token_type::question_mark;
}


bool not_digit_placeholder(const token& tok) noexcept {
  return !is_digit_placeholder(tok);
}


bool is_text_placeholder(const token& tok) noexcept {
  return tok.type == token_type::at_sign;
}


bool is_exponent(const token& tok) noexcept {
  return tok.type == token_type::e;
}


bool is_fraction(const token& tok) noexcept {
  return tok.type == token_type::slash;
}


bool is_thousand_separator(const token& tok) noexcept {
  return tok.type == token_type::comma;
}


bool is_decimal_separator(const token& tok) noexcept {
  return tok.type == token_type::period;
}


bool is_percent(const token& tok) noexcept {
  return tok.type == token_type::percent;
}


bool is_am_pm(const token& tok) noexcept {
  return tok.type == token_type::am_pm;
}


struct condition {
  enum class operation {
    less,          // <
    less_equal,    // <=
    greater,       // >
    greater_equal, // >=
    equal,         // =
    not_equal      // <>
  };

  operation op;
  double    value;

  bool check(double v) const noexcept {
    switch (op) {
      case operation::less:          return (v < value);
      case operation::less_equal:    return ed::fuzzy_less_equal(v, value);
      case operation::greater:       return (v > value);
      case operation::greater_equal: return ed::fuzzy_greater_equal(v, value);
      case operation::equal:         return ed::fuzzy_equal(v, value);
      case operation::not_equal:     return ed::fuzzy_not_equal(v, value);
    }
    ED_ASSERT(false);
    return false;
  }
};


// Ecma Open Office page 1791.
struct system_information {
  std::wstring currency_string; // F, USD and etc.
  // 0, 1 byte - Language ID (LID).
  // 2 byte    - Calendar type.
  // 3 byte    - Number system type.
  unsigned language_info;
};


enum class section_type {
  number,
  fraction,
  exponential,
  date,
  duration,
  text
};


enum class section_number {
  positive,
  negative,
  zero,
  text,
  undefined_section
};


struct section {
  section_number                    number = section_number::undefined_section;
  section_type                      type = section_type::text;
  std::optional<condition>          condition;
  std::optional<system_information> system_information;
  std::optional<ed::color>          text_color;
  token_vector                      tokens;
};


using section_vector = boost::container::small_vector<section, 5>;


struct parser_state {
  section_vector parsed_sections;
  section        curr_section;
};


template<token_type Type>
struct process_string final {
  template<typename Context>
  void operator()(Context& ctx) const {
    auto& state = x3::get<parser_state>(ctx);
    state.curr_section.tokens.push_back(token{Type, std::wstring(_attr(ctx).begin(), _attr(ctx).end())});
  }
};


template<>
struct process_string<token_type::string> final {
  template<typename Context>
  void operator()(Context& ctx) const {
    auto& state = x3::get<parser_state>(ctx);
    if (!state.curr_section.tokens.empty() &&
        state.curr_section.tokens.back().type == token_type::string) {
      state.curr_section.tokens.back().text.append(_attr(ctx).begin(), _attr(ctx).end());
    } else {
      state.curr_section.tokens.push_back(token{token_type::string, std::wstring(_attr(ctx).begin(), _attr(ctx).end())});
    }
  }
};


template<token_type Type>
struct process_char {
  template<typename Context>
  void operator()(Context& ctx) const {
    auto& state = x3::get<parser_state>(ctx);
    state.curr_section.tokens.push_back(token{Type, std::wstring(1, _attr(ctx))});
  }
};


template<token_type Type, std::size_t ... I>
struct process_chars {
  template<typename Context>
  void operator()(Context& ctx) const {
    auto& state = x3::get<parser_state>(ctx);
    std::wstring text;
    (text.push_back(boost::fusion::at_c<I>(_attr(ctx))), ...);
    state.curr_section.tokens.push_back(token{Type, std::move(text)});
  }
};


template<>
struct process_char<token_type::string> {
  template<typename Context>
  void operator()(Context& ctx) const {
    auto& state = x3::get<parser_state>(ctx);
    if (!state.curr_section.tokens.empty() &&
        state.curr_section.tokens.back().type == token_type::string) {
      state.curr_section.tokens.back().text.append(1, _attr(ctx));
    } else {
      state.curr_section.tokens.push_back(token{token_type::string, std::wstring(1, _attr(ctx))});
    }
  }
};


const x3::rule<struct format_rule>                             format_rule             = "format_rule";
const x3::rule<struct section_rule>                            section_rule            = "section_rule";
const x3::rule<struct color_name_rule, ed::color>              color_name_rule         = "color_name_rule";
const x3::rule<struct color_rule>                              color_rule              = "color_rule";
const x3::rule<struct condition_op_rule, condition::operation> condition_op_rule       = "condition_op_rule";
const x3::rule<struct condition_rule>                          condition_rule          = "condition_rule";
const x3::rule<struct system_information_rule>                 system_information_rule = "system_information_rule";
const x3::rule<struct token_rule>                              token_rule              = "token_rule";


const auto format_rule_def = (
  section_rule[([](auto& ctx) {
    auto& state = x3::get<parser_state>(ctx);

    bool has_date_part = false;
    bool has_duration_part = false;
    bool has_digit_placeholder = false;
    bool has_text_placeholder = false;
    bool has_exponent = false;
    bool has_fraction = false;

    for (auto& tok : state.curr_section.tokens) {
      if (is_date_part(tok)) {
        has_date_part = true;
      } else if (is_duration_part(tok)) {
        has_duration_part = true;
      } else if (is_digit_placeholder(tok)) {
        has_digit_placeholder = true;
      } else if (is_text_placeholder(tok)) {
        has_text_placeholder = true;
      } else if (is_exponent(tok)) {
        has_exponent = true;
      } else if (is_fraction(tok)) {
        has_fraction = true;
      }
    }

    if (has_duration_part) {
      state.curr_section.type = section_type::duration;
    } else if (has_date_part) {
      state.curr_section.type = section_type::date;
    } else if (has_text_placeholder && !has_digit_placeholder) {
      state.curr_section.type = section_type::text;
    } else if (has_fraction) {
      state.curr_section.type = section_type::fraction;
    } else if (has_exponent) {
      state.curr_section.type = section_type::exponential;
    } else if (has_digit_placeholder) {
      state.curr_section.type = section_type::number;
    }

    if (static_cast<std::size_t>(section_number::undefined_section) > state.parsed_sections.size()) {
      state.curr_section.number = static_cast<section_number>(state.parsed_sections.size());
    }
    state.parsed_sections.push_back(std::move(state.curr_section));
    state.curr_section = {};
  })] % L";"
);

BOOST_SPIRIT_DEFINE(format_rule);


const auto section_rule_def = -color_rule >> -condition_rule >>  -system_information_rule >> *token_rule;

BOOST_SPIRIT_DEFINE(section_rule);


const auto color_name_rule_def = no_case[x3::standard_wide::symbols<ed::color>({
  {L"black",   ed::color::known::black},
  {L"blue",    ed::color::known::blue},
  {L"cyan",    ed::color::known::cyan},
  {L"green",   ed::color::known::green},
  {L"magenta", ed::color::known::magenta},
  {L"red",     ed::color::known::red},
  {L"white",   ed::color::known::white},
  {L"yellow",  ed::color::known::yellow}
})];

BOOST_SPIRIT_DEFINE(color_name_rule);


const auto color_rule_def = skip(space)[(L"[" >> color_name_rule >> L"]")[([](auto& ctx) {
  auto& state = x3::get<parser_state>(ctx);
  state.curr_section.text_color = _attr(ctx);
})]];

BOOST_SPIRIT_DEFINE(color_rule);


const auto condition_op_rule_def = x3::standard_wide::symbols<condition::operation>({
  {L"<",  condition::operation::less},
  {L"<=", condition::operation::less_equal},
  {L">",  condition::operation::greater},
  {L">=", condition::operation::greater_equal},
  {L"=",  condition::operation::equal},
  {L"<>", condition::operation::not_equal}
});

BOOST_SPIRIT_DEFINE(condition_op_rule);


const auto condition_rule_def = skip(space)[(L"[" >> condition_op_rule >> double_ >> L"]")[([](auto& ctx) {
  auto& state = x3::get<parser_state>(ctx);
  state.curr_section.condition = condition{
    boost::fusion::at_c<0>(_attr(ctx)),
    boost::fusion::at_c<1>(_attr(ctx))
  };
})]];

BOOST_SPIRIT_DEFINE(condition_rule);


const auto system_information_rule_def = skip(space)[(L"[$" >> *(char_ - L'-') >> L'-'  >> hex >> L"]")[([](auto& ctx) {
  auto& state = x3::get<parser_state>(ctx);
  const auto& vec = boost::fusion::at_c<0>(_attr(ctx));
  state.curr_section.system_information = system_information{
    {vec.begin(), vec.end()},
    boost::fusion::at_c<1>(_attr(ctx))
  };
})]];

BOOST_SPIRIT_DEFINE(system_information_rule);


const auto token_rule_def =
  char_(L"#")[process_char<token_type::number_sign>()] |
  char_(L"0")[process_char<token_type::zero>()] |
  char_(L"?")[process_char<token_type::question_mark>()] |
  char_(L"@")[process_char<token_type::at_sign>()] |
  char_(L".")[process_char<token_type::period>()] |
  char_(L",")[process_char<token_type::comma>()] |
  char_(L"%")[process_char<token_type::percent>()] |
  char_(L"/")[process_char<token_type::slash>()] |
  (char_(L"*") >> char_)[process_chars<token_type::asterisk, 0, 1>()] |
  (char_(L"_") >> char_)[process_chars<token_type::underscore, 0, 1>()] |
  (no_case[string(L"e+")[process_string<token_type::e>()]]) |
  (no_case[string(L"e-")[process_string<token_type::e>()]]) |
  (no_case[repeat(3, inf)[char_(L"y")][process_string<token_type::yyyy>()]]) |
  (no_case[repeat(1, 2)[char_(L"y")][process_string<token_type::yy>()]]) |
  (no_case[repeat(6, inf)[char_(L"m")][process_string<token_type::mmmm>()]]) |
  (no_case[string(L"mmmmm")[process_string<token_type::mmmmm>()]]) |
  (no_case[string(L"mmmm")[process_string<token_type::mmmm>()]]) |
  (no_case[string(L"mmm")[process_string<token_type::mmm>()]]) |
  (no_case[string(L"mm")[process_string<token_type::mm>()]]) |
  (no_case[char_(L"m")[process_char<token_type::m>()]]) |
  (no_case[repeat(4, inf)[char_(L"d")][process_string<token_type::dddd>()]]) |
  (no_case[string(L"ddd")[process_string<token_type::ddd>()]]) |
  (no_case[string(L"dd")[process_string<token_type::dd>()]]) |
  (no_case[char_(L"d")[process_char<token_type::d>()]]) |
  (no_case[repeat(2, inf)[char_(L"h")][process_string<token_type::hh>()]]) |
  (no_case[char_(L"h")[process_char<token_type::h>()]]) |
  (no_case[string(L"ss")[process_string<token_type::ss>()]]) |
  (no_case[char_(L"s")[process_char<token_type::s>()]]) |
  (no_case[L"[" >> repeat(1, inf)[char_(L"h")][process_string<token_type::h_sqb>()] >> L"]"]) |
  (no_case[L"[" >> repeat(1, inf)[char_(L"m")][process_string<token_type::m_sqb>()] >> L"]"]) |
  (no_case[L"[" >> repeat(1, inf)[char_(L"s")][process_string<token_type::s_sqb>()] >> L"]"]) |
  (no_case[string(L"a/p")[process_string<token_type::am_pm>()]]) |
  (no_case[string(L"am/pm")[process_string<token_type::am_pm>()]]) |
  (L'\\' >> char_)[process_char<token_type::string>()] |
  char_(L"$+-/():!^&'~{}<>= ")[process_char<token_type::string>()] |
  (raw[lexeme[L'"' >> (+(char_ - L'"')) >> L'"'][process_string<token_type::string>()]]);

BOOST_SPIRIT_DEFINE(token_rule);


const section* section_for_number(const section_vector& sections, double value) noexcept {
  if (sections.empty()) {
    return nullptr;
  }

  // Логика спижжена с https://github.com/andersnm/ExcelNumberFormat/blob/master/src/ExcelNumberFormat/Evaluator.cs
  // ибо нигде не задокументированна.
  const section* result = &sections.front();

  if (result->condition) {
    if (result->condition->check(value)) {
      return result;
    }
  } else if (sections.size() == 1 || (sections.size() == 2 && ed::fuzzy_greater_equal(value, 0.)) || (sections.size() >= 2 && value > 0.)) {
    return result;
  }

  if (sections.size() < 2) {
    return nullptr;
  }

  result = &sections[1];

  if (result->condition) {
    if (result->condition->check(value)) {
      return result;
    }
  } else if (value < 0 || (sections.size() == 2 && sections.front().condition)) {
    return result;
  }

  if (sections.size() < 3) {
    return nullptr;
  }

  return &sections[2];
}


const section* section_for_string(const section_vector& sections) noexcept {
  if (sections.empty()) {
    return nullptr;
  }

  if (sections.size() >= 4) {
    return &sections[3];
  } else {
    for (auto& sect : sections) {
      if (std::any_of(sect.tokens.begin(), sect.tokens.end(), is_text_placeholder)) {
        return &sect;
      }
    }
    return nullptr;
  }
}


template<typename T>
std::wstring to_string(T value, std::optional<int> width, std::optional<int> precision) {
  static_assert(std::is_arithmetic_v<T>);

  std::wstringstream stream;
  stream << std::fixed;
  if (width) {
    stream << std::setw(*width) << std::setfill(L'0');
  }
  if (precision) {
    stream << std::setprecision(*precision);
  }
  stream << value;

  std::wstring symbols = stream.str();
  if (symbols.find(L'.') != std::wstring::npos) {
    while (!symbols.empty()) {
      if (symbols.back() == L'0') {
        symbols.pop_back();
      } else {
        if (symbols.back() == L'.') {
          symbols.pop_back();
        }
        break;
      }
    }
  }
  return symbols;
}


std::pair<token_range, token_range> split_tokens(token_range tokens, token_type by_type) noexcept {
  auto i = std::find_if(tokens.begin(), tokens.end(), [by_type](const token& tok) {
    return tok.type == by_type;
  });

  if (i != tokens.end()) {
    return {
      {tokens.begin(), i},
      {std::next(i), tokens.end()}
    };
  } else {
    return {tokens, {}};
  }
}


std::pair<std::wstring_view, std::wstring_view> split_string(const std::wstring_view& str, wchar_t by_char) {
  if (auto p = str.find(by_char); p != std::wstring_view::npos) {
    return {str.substr(0, p), str.substr(p + 1, str.size() - p - 1)};
  } else {
    return {str, {}};
  }
}


void format_literal(const token& tok, value_format::result& result) {
  if (tok.type == token_type::comma) {
    return;
  }

  if (tok.type == token_type::asterisk) {
    ED_ASSERT(tok.text.size() == 2);
    result.fill_positions = result.text.size();
    result.text += tok.text[1];
  } else if (tok.type == token_type::underscore) {
    ED_ASSERT(tok.text.size() == 2);
    // Underscore означает добавление пробела,
    // по ширине соответствующего следующему за ним символу
    result.white_positions.push_back(result.text.size());
  } else {
    result.text += tok.text;
  }
}


void format_literals(token_range tokens, value_format::result& result) {
  for (auto& tok : tokens) {
    format_literal(tok, result);
  }
}


void format_digit_placeholder(wchar_t symbol, const token& tok, bool significant, value_format::result& result) {
  ED_ASSERT(symbol != 0);
  if (significant) {
    result.text += symbol;
  } else {
    if (tok.type == token_type::zero) { // 0
      result.text += L'0';
    } else if (tok.type == token_type::question_mark) { // ?
      result.text += L' ';
    }
  }
}


void format_integer_symbols(std::wstring_view symbols, token_range tokens, std::optional<wchar_t> thousand_sep, value_format::result& result) {
  auto tok_it = std::find_if(tokens.begin(), tokens.end(), is_digit_placeholder);
  ED_ASSERT(tok_it != tokens.end());
  format_literals({tokens.begin(), tok_it}, result);

  std::size_t placeholders_count = std::count_if(tok_it, tokens.end(), is_digit_placeholder);
  auto symbol_it = symbols.begin();

  // для thousand_sep
  int rest_symbols = symbols.size();
  bool can_thousand_sep  = false;

  if (symbols.size() < placeholders_count) {
    std::size_t leading_zeros_count = placeholders_count - symbols.size();
    rest_symbols += leading_zeros_count;
    while (tok_it < tokens.end()) {
      if (is_digit_placeholder(*tok_it)) {
        if (thousand_sep && rest_symbols % 3 == 0 && can_thousand_sep) {
          result.text += *thousand_sep;
        }
        format_digit_placeholder(L'0', *tok_it, false, result);
        --leading_zeros_count;
        --rest_symbols;
        if (tok_it->type == token_type::zero || tok_it->type == token_type::question_mark) {
          can_thousand_sep = true;
        }
      } else {
        format_literal(*tok_it, result);
      }

      ++tok_it;
      if (leading_zeros_count == 0) {
        break;
      }
    }
  } else if (symbols.size() > placeholders_count) {
    // Надо заменить первый placeholder на первые невлезающие цифры
    if (tok_it != tokens.end()) {
      ED_ASSERT(is_digit_placeholder(*tok_it));
      for (std::size_t i = 0; i < symbols.size() - placeholders_count + 1; ++i) {
        if (thousand_sep && rest_symbols % 3 == 0 && can_thousand_sep && symbol_it != symbols.end()) {
          result.text += *thousand_sep;
        }
        format_digit_placeholder(*symbol_it, *tok_it, true, result);
        ++symbol_it;
        --rest_symbols;
        can_thousand_sep = true;
      }
      ++tok_it;
    }
  }

  while (tok_it < tokens.end()) {
    if (is_digit_placeholder(*tok_it)) {
      ED_ASSERT(symbol_it != symbols.end());
      if (thousand_sep && rest_symbols % 3 == 0 && can_thousand_sep) {
        result.text += *thousand_sep;
      }
      format_digit_placeholder(*symbol_it, *tok_it, true, result);
      ++symbol_it;
      --rest_symbols;
      can_thousand_sep = true;
    } else {
      format_literal(*tok_it, result);
    }

    ++tok_it;
  }
}


void format_decimal_symbols(std::wstring_view symbols, token_range tokens, value_format::result& result) {
  auto symbol_it = symbols.begin();

  for (auto& tok : tokens) {
    if (is_digit_placeholder(tok)) {
      if (symbol_it == symbols.end()) {
        format_digit_placeholder(L'0', tok, false, result);
      } else {
        format_digit_placeholder(*symbol_it, tok, true, result);
        ++symbol_it;
      }
    } else {
      format_literal(tok, result);
    }
  }
}


void format_denominator(double value, token_range tokens, value_format::result& result, const std::locale& loc) {
  auto placeholder_count = std::count_if(tokens.begin(), tokens.end(), is_digit_placeholder);
  auto symbols = to_string(value, placeholder_count, 0);
  auto symbol_it = symbols.begin();
  bool significant = false;

  for (auto& tok : tokens) {
    if (is_digit_placeholder(tok)) {
      wchar_t symbol = 0;

      if (symbol_it == symbols.end()) {
        symbol = L'0';
        significant = false;
      } else {
        if (tok.type == token_type::question_mark && !significant) {
          while (symbol_it != symbols.end()) {
            symbol = *symbol_it;
            ++symbol_it;
            if (symbol != L'0') {
              significant = true;
              break;
            }
          }
        } else {
          symbol = *symbol_it;
          if (symbol != L'0') {
            significant = true;
          }
          ++symbol_it;
        }
      }

      format_digit_placeholder(symbol, tok, significant, result);
    } else {
      format_literal(tok, result);
    }
  }
}


void format_number_by_default(double value, const std::locale& loc, value_format::result& result) {
  // Для дефолного форматирования числа, не нужны разделители между тысячными разрядами.
  // Поэтому в поток не задаётся локаль, а только заменяется разделитель.
  result.text = to_string(value, std::nullopt, std::nullopt);
  if (auto pos = result.text.find(L'.'); pos != std::wstring::npos) {
    result.text[pos] = std::use_facet<std::numpunct<wchar_t>>(loc).decimal_point();
  }
}


void format_number(double value, token_range tokens, section_number s_number, const std::locale& loc, value_format::result& result) {
  // Преобразование value c учетом '%' и ','
  auto last_placeholder_it = boost::algorithm::find_if_backward(tokens, is_digit_placeholder);
  ED_ASSERT(last_placeholder_it != tokens.end());
  value /= std::pow(1000., std::count_if(last_placeholder_it, tokens.end(), is_thousand_separator));
  value *= std::any_of(tokens.begin(), tokens.end(), is_percent) ? 100. : 1.;

  auto [integer_tokens, decimal_tokens] = split_tokens(tokens, token_type::period);
  int precision = std::count_if(decimal_tokens.begin(), decimal_tokens.end(), is_digit_placeholder);

  // Округление
  if (precision > 0) {
    // TODO: Переделать округление
    double tmp = std::pow(10., precision);
    value = static_cast<double>(static_cast<long long>(value * tmp + (value < 0. ? -0.5 : 0.5)));
    value /= tmp;
  }

  // Преобразование в строку без знака
  std::wstring symbols = to_string(std::abs(value), std::nullopt, precision);
  auto [integer_symbols, decimal_symbols] = split_string(symbols, L'.');

  // Если функция вызывается для негативной секции, то знак минуса по умолчанию не рисуется.
  if (value < 0 && s_number != section_number::negative) {
    result.text += L'-';
  }

  if (!integer_tokens.empty()) {
    std::optional<wchar_t> thousand_sep;
    if (std::any_of(tokens.begin(), last_placeholder_it, is_thousand_separator)) {
      thousand_sep = std::use_facet<std::numpunct<wchar_t>>(loc).thousands_sep();
    }
    format_integer_symbols(integer_symbols, integer_tokens, thousand_sep, result);
  }

  if (!decimal_tokens.empty()) {
    result.text += std::use_facet<std::numpunct<wchar_t>>(loc).decimal_point();
    format_decimal_symbols(decimal_symbols, decimal_tokens, result);
  }
}


void frac(double x, int d, int& nom, int& den) {
  double sgn = x < 0 ? -1. : 1.;
  double b = x * sgn;
  double p_2 = 0.;
  double p_1 = 1.;
  double p = 0.;
  double q_2 = 1.;
  double q_1 = 0.;
  double q = 0.;
  double a = std::floor(b);

  while (q_1 < d) {
    a = std::floor(b);
    p = a * p_1 + p_2;
    q = a * q_1 + q_2;

    if ((b - a) < 0.00000005) {
      break;
    }

    b = 1 / (b - a);
    p_2 = p_1; p_1 = p;
    q_2 = q_1; q_1 = q;
  }

  if (q > d) {
    if (q_1 > d) {
      q = q_2;
      p = p_2;
    } else {
      q = q_1;
      p = p_1;
    }
  }

  nom = static_cast<int>(sgn * p);
  den = static_cast<int>(q);
}


auto number_of_token(const token_range tokens, token_type token) {
  return std::count_if(tokens.begin(), tokens.end(), [token](const auto& tok) {
    return tok.type == token;
  });
}


void format_error(value_format::result& result) {
  result.text = L"#";
  result.fill_positions = 0;
}


void format_fraction(double value, token_range tokens, section_number s_number, const std::locale& loc, value_format::result& result) {
  double tmp;
  if (ed::fuzzy_is_null(std::modf(value, &tmp))) {
    format_number_by_default(value, loc, result);
    return;
  }

  token_range integral_tokens;
  token_range numerator_tokens;
  token_range denominator_tokens;

  {
    std::tie(numerator_tokens, denominator_tokens) = split_tokens(tokens, token_type::slash);
    if (numerator_tokens.empty() || denominator_tokens.empty()) {
      format_number_by_default(value, loc, result);
      return;
    }

    auto last_placeholder_it = boost::algorithm::find_if_backward(numerator_tokens, is_digit_placeholder);
    auto integral_end = boost::algorithm::find_if_backward(numerator_tokens.begin(), last_placeholder_it, not_digit_placeholder);
    if (integral_end != numerator_tokens.end()) {
      integral_tokens = {numerator_tokens.begin(), std::next(integral_end)};
      numerator_tokens = {std::next(integral_end), numerator_tokens.end()};
    }
  }

  if (value < 0. && s_number != section_number::negative) {
    result.text += L'-';
  }

  value = std::abs(value);
  double integral = 0;

  if (!integral_tokens.empty()) {
    value = std::modf(value, &integral);
  }

  // TODO: Знаменатель может быть явно задан в токенах (если нет placeholder-a)
  int denominator_digits = std::min(
    static_cast<int>(std::count_if(denominator_tokens.begin(), denominator_tokens.end(), is_digit_placeholder)), 7);

  int numerator = 0;
  int denominator = 0;

  frac(value, std::pow(10, denominator_digits) - 1, numerator, denominator);

  if (numerator == denominator) {
    integral += 1.;
  }

  // 0 ->     0 X   integer =  0, num =  0   fuzzy_is_null(std::modf(value, &tmp)) = true
  // 0,1 ->   0 X   integer =  0, num =  0   fuzzy_is_null(std::modf(value, &tmp)) = false
  // 10 ->   10 X   integer != 0, num =  0
  // 0,5 ->   X 1/2 integer =  0, num != 0
  // 10,2 -> 10 1/5 integer != 0, num != 0
  if (!integral_tokens.empty() && (integral != 0 || numerator == 0)) {
    format_integer_symbols(to_string(integral, std::nullopt, 0), integral_tokens, std::nullopt, result);
  }

  if (numerator != denominator && numerator) {
    format_integer_symbols(to_string(numerator, std::nullopt, 0), numerator_tokens, std::nullopt, result);
    result.text += L'/';
    format_denominator(denominator, denominator_tokens, result, loc);
  }
}


void format_exponential(double value, token_range tokens, section_number s_number, const std::locale& loc, value_format::result& result) {
  auto e_iter = std::find_if(tokens.begin(), tokens.end(), is_exponent);
  ED_ASSERT(e_iter != tokens.end());
  token_range mantissa_tokens(tokens.begin(), e_iter);
  token_range exponent_tokens(std::next(e_iter), tokens.end());

  double exponent = 0.;
  double mantissa = 0.;

  if (value != 0) {
    exponent = std::floor(std::log10(std::abs(value)));
    mantissa = value / std::pow(10., exponent);
  }

  format_number(mantissa, mantissa_tokens, s_number, loc, result);

  ED_ASSERT(e_iter->text.size() == 2);
  result.text += e_iter->text[0];
  if (e_iter->text[1] == L'+' && exponent >= 0.) {
    result.text += L'+';
  } else if (exponent < 0) {
    result.text += L'-';
  }

  std::wstring exponent_symbols = to_string(std::abs(exponent), std::nullopt, 0);
  format_integer_symbols(exponent_symbols, exponent_tokens, std::nullopt, result);
}


void format_date(double value, token_range tokens, const std::locale& loc, value_format::result& result) {
  if (value < 0) {
    _::format_error(result);
    return;
  }
  std::tm tm;

  try {
    const auto p_time =  cell_value(value).to<boost::posix_time::ptime>();
    const auto rounded_sec = std::round(p_time.time_of_day().total_milliseconds() / 1000.);
    tm = boost::posix_time::to_tm(boost::posix_time::ptime(p_time.date(), boost::posix_time::seconds(static_cast<long long>(rounded_sec))));
  } catch (const std::exception&) {
    _::format_error(result);
    return;
  }

  auto stringify = [&tm, &loc](const wchar_t* fmt) {
    std::wstringstream stream;
    stream.imbue(loc);
    stream << std::put_time(&tm, fmt);
    return stream.str();
  };

  for (auto tok_it = tokens.begin(); tok_it != tokens.end(); ++tok_it) {
    switch (tok_it->type) {
    case token_type::yy:
      result.text += std::to_wstring((tm.tm_year + 1900) % 100);
      break;

    case token_type::yyyy:
      result.text += std::to_wstring(tm.tm_year + 1900);
      break;

    // Месяц или минута. Если предыдущий токен час или последующий токен секунда, то это минута.
    case token_type::m:
    case token_type::mm: {
        bool is_month = true;
        auto prev_tokens = boost::make_iterator_range(tokens.begin(), tok_it);
        if (auto prev = boost::algorithm::find_if_backward(prev_tokens, is_date_part); prev != prev_tokens.end()) {
          if (prev->type == token_type::h || prev->type == token_type::hh) {
            is_month = false;
          }
        }
        if (is_month) {
          if (auto next = std::find_if(std::next(tok_it), tokens.end(), is_date_part); next != tokens.end()) {
            if (next->type == token_type::s || next->type == token_type::ss) {
              is_month = false;
            }
          }
        }
        if (is_month) {
          std::wstring str = std::to_wstring(tm.tm_mon + 1);
          if (tok_it->type == token_type::mm && str.size() < 2) {
            result.text += L'0';
          }
          result.text += str;
        } else {
          std::wstring str = stringify(L"%M");
          ED_ASSERT(str.size() == 2);
          if (tok_it->type == token_type::m && str.front() == L'0') {
            result.text += str.back();
          } else {
            result.text += str;
          }
        }
      }
      break;

    case token_type::mmm:
      result.text += stringify(L"%b");
      break;

    case token_type::mmmm:
      result.text += stringify(L"%B");
      break;

    case token_type::mmmmm:
      result.text += stringify(L"%b").front();
      break;

    case token_type::d:
      result.text += std::to_wstring(tm.tm_mday);
      break;

    case token_type::dd:
      result.text += stringify(L"%d");
      break;

    case token_type::ddd:
      result.text += stringify(L"%a");
      break;

    case token_type::dddd:
      result.text += stringify(L"%A");
      break;

    case token_type::h:
    case token_type::hh: {
        std::wstring str = std::any_of(tokens.begin(), tokens.end(), is_am_pm) ? stringify(L"%I") : stringify(L"%H");
        ED_ASSERT(str.size() == 2);
        if (tok_it->type == token_type::h && str.front() == L'0') {
          result.text += str.back();
        } else {
          result.text += str;
        }
      }
      break;

    case token_type::s:
    case token_type::ss: {
        std::wstring str = stringify(L"%S");
        ED_ASSERT(str.size() == 2);
        if (tok_it->type == token_type::s && str.front() == L'0') {
          result.text += str.back();
        } else {
          result.text += str;
        }
      }
      break;

    case token_type::am_pm:
      if (tok_it->text.size() == 3) {
        if (tm.tm_hour < 12) {
          result.text += tok_it->text[0];
        } else {
          result.text += tok_it->text[2];
        }
      } else if (tok_it->text.size() == 5) {
        if (tm.tm_hour < 12) {
          result.text += tok_it->text.substr(0, 2);
        } else {
          result.text += tok_it->text.substr(3, 2);
        }
      } else {
        ED_ASSERT(false);
      }
      break;

    default:
      format_literal(*tok_it, result);
      break;
    }
  }
}


void format_duration(double value, token_range tokens, const std::locale& loc, value_format::result& result) {
  // В продолжительности не должно быть больше 3 токенов '0'
  const auto zero_tokens = number_of_token(tokens, token_type::zero);
  if (value < 0 || zero_tokens > 3) {
    _::format_error(result);
    return;
  }

  // Если количетсво токенов '0' больше нуля, то не происходит округление в большую сторону и выводятся миллисекунды.
  boost::posix_time::time_duration duration;
  if (zero_tokens > 0) {
    duration = cell_value(value).to<boost::posix_time::time_duration>();
  } else {
    const auto temp = cell_value(value).to<boost::posix_time::time_duration>();
    const auto rounded_sec = std::round(temp.total_milliseconds() / 1000.);
    duration = boost::posix_time::seconds(static_cast<long long>(rounded_sec));
  }

  auto milliseconds = duration.total_milliseconds() - duration.total_seconds() * 1000; // Получаем миллисекунды
  auto pow          = 2;                                                               // Для разбиения на десятичные разряды (сотни, десятки, еденицы)
  for (auto& tok : tokens) {
    switch (tok.type) {
    case token_type::m:
    case token_type::mm:
    case token_type::mmm:
    case token_type::mmmm:
    case token_type::mmmmm:
      result.text += to_string(duration.minutes(), tok.text.size(), 0);
      break;

    case token_type::s:
    case token_type::ss:
      result.text += to_string(duration.seconds(), tok.text.size(), 0);
      break;

    case token_type::h_sqb: {
      auto total_h = duration.total_seconds() / (60 * 60);
      result.text += to_string(total_h, tok.text.size(), 0);
      duration -= boost::posix_time::hours(total_h);
    }
    break;

    case token_type::m_sqb: {
      auto total_m = duration.total_seconds() / 60;
      result.text += to_string(total_m, tok.text.size(), 0);
      duration -= boost::posix_time::minutes(total_m);
    }
    break;

    case token_type::s_sqb: {
      auto total_s = duration.total_seconds();
      result.text += to_string(total_s, tok.text.size(), 0);
      duration -= boost::posix_time::seconds(total_s);
    }
    break;
    case token_type::zero: {
      const auto current_milliseconds = static_cast<decltype(milliseconds)>(std::pow(10, pow--));
      result.text += to_string(milliseconds / current_milliseconds, tok.text.size(), std::nullopt);
      milliseconds %= current_milliseconds;
    }
    break;

    default:
      format_literal(tok, result);
      break;
    }
  }
}


void format(double value, const section_vector& sections, const std::locale& loc, value_format::result& result) {
  const auto sect = section_for_number(sections, value);
  if (!sect) {
    format_number_by_default(value, loc, result);
    return;
  }

  result.text_color = sect->text_color;

  switch (sect->type) {
    case section_type::number:
      format_number(value, sect->tokens, sect->number, loc, result);
      break;

    case section_type::fraction:
      format_fraction(value, sect->tokens, sect->number, loc, result);
      break;

    case section_type::exponential:
      format_exponential(value, sect->tokens, sect->number, loc, result);
      break;

    case section_type::date:
      format_date(value, sect->tokens, loc, result);
      break;

    case section_type::duration:
      format_duration(value, sect->tokens, loc, result);
      break;

    default:
      for (auto& tok : sect->tokens) {
        if (tok.type == token_type::at_sign && sect->number != section_number::undefined_section && sect->number != section_number::text) {
          // Особый случай - к числу применен текстовый формат (баг CEL-254)
          format_number_by_default(value, loc, result);
        } else {
          format_literal(tok, result);
        }
      }
  }
}


void format(const std::wstring& value, const section_vector& sections, value_format::result& result) {
  auto* sect = section_for_string(sections);
  if (!sect) {
    result.text = value;
    return;
  }

  result.text_color = sect->text_color;

  for (auto& tok : sect->tokens) {
    if (tok.type == token_type::at_sign) {
      result.text += value;
    } else {
      format_literal(tok, result);
    }
  }
}

}} // namespace _


struct value_format::impl {
  std::locale       loc;
  _::section_vector sections;
};


value_format::value_format(const std::wstring& format, std::locale loc)
  : impl_(std::make_unique<impl>()) {

  impl_->loc = std::move(loc);

  if (!format.empty()) {
    try {
      _::parser_state state;
      auto begin = format.begin();
      bool ok = x3::parse(begin, format.end(), with<_::parser_state>(state)[_::format_rule]);
      if (ok && begin == format.end()) {
        impl_->sections = std::move(state.parsed_sections);
      }
    } catch (const std::exception&) {}
  }
}


value_format::~value_format() = default;


value_format::result value_format::operator()(const cell_value& value) const {
  value_format::result result;

  if (value.is<bool>()) {
    _::format(value.as<bool>() ? L"TRUE" : L"FALSE", impl_->sections, result);
  } else if (value.is<double>()) {
    _::format(value.as<double>(), impl_->sections, impl_->loc, result);
  } else if (value.is<std::wstring>()) {
    _::format(value.as<std::wstring>(), impl_->sections, result);
  } else if (value.is<rich_text>()) {
    _::format(value.to<std::wstring>(), impl_->sections, result);
  } else if (value.is<cell_value_error>()) {
    result.text = value.to<std::wstring>();
  }

  return result;
}


bool value_format::has_date_section(double value) const {
  auto time_token = [](const auto& tok) {
    return tok.type == _::token_type::h ||
           tok.type == _::token_type::hh ||
           tok.type == _::token_type::s ||
           tok.type == _::token_type::ss;
  };

  const auto sect = _::section_for_number(impl_->sections, value);
  return sect && sect->type == _::section_type::date &&
         std::none_of(sect->tokens.begin(), sect->tokens.end(), time_token);
}


bool value_format::has_time_section(double value) const {
  const auto date_token = [](const auto& tok) {
    return tok.type == _::token_type::yy ||
           tok.type == _::token_type::yyyy ||
           tok.type == _::token_type::d ||
           tok.type == _::token_type::dd ||
           tok.type == _::token_type::ddd ||
           tok.type == _::token_type::dddd;
  };

  const auto check_m_mm_token = [](const auto sect) {
    const auto tok_beg = sect->tokens.begin();
    const auto tok_end = sect->tokens.end();

    const auto m_mm_token = std::find_if(tok_beg, tok_end, [](const auto& token) {
      return token.type == _::token_type::m ||
             token.type == _::token_type::mm;
    });
    const auto check_prev_next_token = [](const auto tok_beg, const auto tok_end, const auto m_mm_token) {
      const auto prev_token = boost::algorithm::find_if_backward(tok_beg, m_mm_token, [](const auto& tok) {
        return tok.type != _::token_type::string;
      });
      const auto next_token = std::find_if(std::next(m_mm_token), tok_end, [](const auto& tok) {
        return tok.type != _::token_type::string;
      });
      return (prev_token != m_mm_token && (prev_token->type == _::token_type::hh || prev_token->type == _::token_type::h)) ||
             (next_token != tok_end && (next_token->type == _::token_type::ss || next_token->type == _::token_type::s));
    };
    return (m_mm_token == tok_end || check_prev_next_token(tok_beg, tok_end, m_mm_token));
  };

  const auto sect = _::section_for_number(impl_->sections, value);
  return sect && sect->type == _::section_type::date &&
         std::none_of(sect->tokens.begin(), sect->tokens.end(), date_token) &&
         check_m_mm_token(sect);
}


bool value_format::has_date_time_section(double value) const {
  const auto sect = _::section_for_number(impl_->sections, value);
  return sect && sect->type == _::section_type::date;
}


bool value_format::has_duration_section(double value) const {
  const auto sect = _::section_for_number(impl_->sections, value);
  return sect && sect->type == _::section_type::duration;
}


bool value_format::has_exponent_section(double value) const {
  const auto sect = _::section_for_number(impl_->sections, value);
  return sect && sect->type == _::section_type::exponential;
}


bool value_format::has_percent_section(double value) const {
  const auto sect = _::section_for_number(impl_->sections, value);
  return sect && sect->type == _::section_type::number && std::any_of(sect->tokens.begin(), sect->tokens.end(), _::is_percent);
}


bool value_format::has_number_section(double value) const {
  const auto number_token = [](const auto& tok) {
    return tok.type == _::token_type::number_sign ||
           tok.type == _::token_type::period ||
           tok.type == _::token_type::comma ||
           tok.type == _::token_type::zero;
  };
  const auto sect = _::section_for_number(impl_->sections, value);
  return sect && sect->type == _::section_type::number && std::all_of(sect->tokens.begin(), sect->tokens.end(), number_token);
}


bool value_format::has_fraction_section(double value) const {
  const auto sect = _::section_for_number(impl_->sections, value);
  return sect && sect->type == _::section_type::fraction;
}


bool value_format::has_any_section() const noexcept {
  return !impl_->sections.empty();
}

} // namespace lde::cellfy::boox
