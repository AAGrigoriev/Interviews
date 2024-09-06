#include <lde/cellfy/boox/fx_engine.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <boost/exception/get_error_info.hpp>
#include <boost/functional/hash.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/mpl_list.hpp>

#include <ed/core/assert.h>
#include <ed/core/math.h>
#include <ed/core/exception.h>
#include <ed/core/type_traits.h>
#include <ed/core/unicode.h>

#include <lde/cellfy/boox/exception.h>
#include <lde/cellfy/boox/fx_function.h>
#include <lde/cellfy/boox/range.h>
#include <lde/cellfy/boox/workbook.h>
#include <lde/cellfy/boox/worksheet.h>


namespace lde::cellfy::boox::fx {
namespace _ {
namespace {


using operand_tokens = boost::mp11::mp_list<
  ast::number,
  ast::string,
  ast::boolean,
  ast::reference
>;


using binary_operator_tokens = boost::mp11::mp_list<
  ast::less,
  ast::less_equal,
  ast::greater,
  ast::greater_equal,
  ast::equal,
  ast::not_equal,
  ast::concat,
  ast::add,
  ast::subtract,
  ast::multiply,
  ast::divide,
  ast::power,
  ast::range
>;


using unary_operator_tokens = boost::mp11::mp_list<
  ast::plus,
  ast::minus,
  ast::percent
>;


template<typename Token>
constexpr std::size_t args_count(const Token&) noexcept {
  if constexpr (boost::mp11::mp_contains<binary_operator_tokens, Token>::value) {
    return 2;
  } else if constexpr (boost::mp11::mp_contains<unary_operator_tokens, Token>::value) {
    return 1;
  }
}


std::size_t args_count(const ast::func& token) noexcept {
  return token.args_count;
}


auto& get_func_format() {
  static const std::unordered_map<std::wstring_view, std::wstring_view, ed::ihash, ed::is_iequal> func_format {
    {L"DATE",      L"dd.mm.yyyy"},
    {L"EOMONTH",   L"dd.mm.yyyy"},
    {L"EDATE",     L"dd.mm.yyyy"},
    {L"TIME",      L"hh:mm:ss"},
    {L"TIMEVALUE", L"hh:mm:ss"},
    {L"TODAY",     L"dd.mm.yyyy"},
    {L"NOW",       L"dd.mm.yyyy hh:mm:ss"},
    {L"WORKDAY",   L"dd.mm.yyyy"}
  };

  return func_format;
}

}} // namespace _



std::wstring ast_to_formula(const ast::tokens& ast) {
  using stack = boost::container::small_vector<std::wstring, 10>;
  if (ast.empty()) {
    return {};  // покрыть тестами этот кейс.
  }

  stack result;
  for (auto&& tok : ast) {
    std::visit([&result](auto&& tok) {
      using T = std::decay_t<decltype(tok)>;
      if constexpr (boost::mp11::mp_contains<_::operand_tokens, T>::value) {
        if constexpr (std::is_same_v<T, ast::number>) {
          double i_ptr;
          const auto frac = std::modf(tok.value, &i_ptr);
          if (ed::fuzzy_equal(frac, 0.)) {
            result.push_back(std::to_wstring(static_cast<std::int64_t>(i_ptr)));
          } else {
            result.push_back(std::to_wstring(tok.value));
          }
        } else if constexpr (std::is_same_v<T, ast::string>) {
          result.push_back(L"\"" + std::move(tok.value) + L"\"");
        } else if constexpr (std::is_same_v<T, ast::boolean>) {
          result.emplace_back(tok.value ? L"TRUE" : L"FALSE"); // todo: locale
        } else if constexpr (std::is_same_v<T, ast::reference>) {
          result.push_back(std::wstring(tok.addr));
        }
      } else {
        std::size_t args_count = _::args_count(tok);
        ED_EXPECTS(result.size() >= args_count);

        if constexpr (boost::mp11::mp_contains<_::binary_operator_tokens, T>::value) {
          auto operand_1 = std::move(result.back());
          result.pop_back();
          auto operand_2 = std::move(result.back());
          result.pop_back();
          std::wstring bin_operator;
          if constexpr (std::is_same_v<T, ast::less>) {
            bin_operator = L"<";
          } else if constexpr (std::is_same_v<T, ast::less_equal>) {
            bin_operator = L"<=";
          } else if constexpr (std::is_same_v<T, ast::greater>) {
            bin_operator = L">";
          } else if constexpr (std::is_same_v<T, ast::greater_equal>) {
            bin_operator = L">=";
          } else if constexpr (std::is_same_v<T, ast::equal>) {
            bin_operator = L"=";
          } else if constexpr (std::is_same_v<T, ast::not_equal>) {
            bin_operator = L"<>";
          } else if constexpr (std::is_same_v<T, ast::concat>) {
            bin_operator = L"&";
          } else if constexpr (std::is_same_v<T, ast::add>) {
            bin_operator = L"+";
          } else if constexpr (std::is_same_v<T, ast::subtract>) {
            bin_operator = L"-";
          } else if constexpr (std::is_same_v<T, ast::multiply>) {
            bin_operator = L"*";
          } else if constexpr (std::is_same_v<T, ast::divide>) {
            bin_operator = L"/";
          } else if constexpr (std::is_same_v<T, ast::power>) {
            bin_operator = L"^";
          } else if constexpr (std::is_same_v<T, ast::range>) {
            bin_operator = L":";
          }
          result.push_back(std::move(operand_2) + std::move(bin_operator) + std::move(operand_1));
        } else if constexpr (boost::mp11::mp_contains<_::unary_operator_tokens, T>::value) {
          auto operand = std::move(result.back());
          result.pop_back();
          std::wstring unary_operator;
          if constexpr (std::is_same_v<T, ast::plus>) {
            unary_operator = L"+";
          } else if constexpr (std::is_same_v<T, ast::minus>) {
            unary_operator = L"-";
          } else if constexpr (std::is_same_v<T, ast::percent>) {
            unary_operator = L"%";
          }
          result.push_back(std::move(unary_operator) + std::move(operand));
        } else {
          static_assert(std::is_same_v<T, ast::func>);

          stack args{std::make_move_iterator(result.end() - args_count), std::make_move_iterator(result.end())};
          if (!args.empty()) {
            result.erase(result.end() - args_count, result.end());
            auto set_comma = [](auto&& a, auto&& b) {
              return std::forward<decltype(a)>(a) + L";" + std::forward<decltype(b)>(b);
            };
            result.push_back(std::wstring(tok.ptr->name()) +
                             L"(" +
                             std::accumulate(std::next(args.begin()), args.end(), *args.begin(), set_comma) +
                             L")");
          } else {
            result.push_back(std::wstring(tok.ptr->name()) + L"()"); // Если формула не имеет аргументов.
          }
        }
      }
    }, tok);
  }

  return std::move(result.back());
}


std::wstring get_func_format(const ast::tokens& ast) {
  std::wstring result;

  // Поиск до первой функции. Если у функции есть формат, то он возращается.
  // Самая первая функция, относительно ввода, находится в конце массива.
  for (auto ast_it = ast.rbegin(); ast_it != ast.rend(); ++ast_it) {
    if (const ast::func* ast_func = std::get_if<ast::func>(&(*ast_it))) {
      if (auto f_it = _::get_func_format().find(ast_func->ptr->name()); f_it != _::get_func_format().end()) {
        result.assign(f_it->second);
      }
      break;
    }
  }

  return result;
}


engine::engine(const worksheet& sheet) noexcept
  : sheet_(&sheet) {
}


// Особенности работы операндов.
// 1 - Если есть ошибки в ячейках и мы используем ссылки на них. Типо А1=А2. То операция не применяется. Возвращается ошибка.
//     Если ошибки были в ячейках А1 и А2, то применяется ошибка из А1(слева направо).
// 2 - Есть сравнение с пустыми ячейками всеми операторами.
// 3 - Not case sensetive сравнение текста.
cell_value engine::evaluate(const ast::tokens& ast) const {
  ED_EXPECTS(!ast.empty());

  try {
    operand::list stack;

    for (auto& token : ast) {
      std::visit([&stack, this](auto& token) {
        using token_type = ed::remove_cvref_t<decltype(token)>;

        if constexpr (boost::mp11::mp_contains<_::operand_tokens, token_type>::value) { // Операнд
          stack.push_back(to_operand(token));
        } else { // Оператор
          std::size_t args_count = _::args_count(token);
          ED_EXPECTS(stack.size() >= args_count);

          if constexpr (boost::mp11::mp_contains<_::binary_operator_tokens, token_type>::value) {
            auto rhs = std::move(stack.back());
            stack.pop_back();
            auto lhs = std::move(stack.back());
            stack.pop_back();

            // Если бинарная операция не работает с range. То проверим его на наличие ошибок в cell_value.
            if constexpr (!std::is_same_v<token_type, ast::range>) {
              auto lhs_v = lhs.to<cell_value>();
              auto rhs_v = rhs.to<cell_value>();

              if (lhs_v.type() == cell_value_type::error) {
                stack.push_back(std::move(lhs));
              } else if (rhs_v.type() == cell_value_type::error) {
                stack.push_back(std::move(rhs));
              } else {
                stack.push_back(exec(token, lhs_v, rhs_v));
              }
            } else { // Если бинарная операция с range.
              stack.push_back(exec(token, std::move(lhs), std::move(rhs)));
            }
          } else if constexpr (boost::mp11::mp_contains<_::unary_operator_tokens, token_type>::value) {
            auto v = stack.back().to<cell_value>();
            stack.pop_back();
            if (v.type() == cell_value_type::error) {
              stack.push_back(std::move(v));
            } else {
              stack.push_back(exec(token, v));
            }
          } else {
            operand::list args(
              std::make_move_iterator(stack.end() - args_count),
              std::make_move_iterator(stack.end()));
            stack.erase(stack.end() - args_count, stack.end());
            stack.push_back(exec(token, std::move(args)));
          }
        }
      }, token);
    }

    ED_ENSURES(stack.size() == 1);
    return stack.back().to<cell_value>();
  } catch (const invalid_function_name&) {
    return cell_value_error::name;
  } catch (const invalid_worksheet_name&) {
    return cell_value_error::ref;
  } catch (const division_by_zero&) {
    return cell_value_error::div0;
  } catch (const bad_value_cast& e) {
    if (const cell_value_error* error = boost::get_error_info<cell_value_error_info>(e)) {
      return *error;
    } else {
      return cell_value_error::value;
    }
  } catch (const std::exception&) {
    return cell_value_error::na; /// TODO: Это не точно.
  }
}


operand engine::to_operand(const ast::number& token) const {
  return token.value;
}


operand engine::to_operand(const ast::string& token) const {
  return token.value;
}


operand engine::to_operand(const ast::boolean& token) const {
  return token.value;
}


operand engine::to_operand(const ast::reference& token) const {
  if (token.sheet.empty() || token.sheet == *sheet_->name) {
    return range_list{sheet_->cell(token.addr)};
  }
  if (auto* other_sheet = sheet_->book().sheet_by_name(token.sheet)) {
    return range_list{other_sheet->cell(token.addr)};
  } else {
    return cell_value_error::ref;
  }
  return {};
}


operand engine::exec(ast::less, const cell_value& lhs, const cell_value& rhs) const {
  if (lhs.is<std::wstring>() && rhs.is<std::wstring>()) {
    auto& lhs_s = lhs.as<std::wstring>();
    auto& rhs_s = rhs.as<std::wstring>();
    return ed::iless(lhs_s, rhs_s);
  }
  return lhs < rhs;
}


operand engine::exec(ast::less_equal, const cell_value& lhs, const cell_value& rhs) const {
  if (lhs.is<std::wstring>() && rhs.is<std::wstring>()) {
    auto& lhs_s = lhs.as<std::wstring>();
    auto& rhs_s = rhs.as<std::wstring>();
    return !ed::iless(rhs_s, lhs_s);
  }
  return lhs <= rhs;
}


operand engine::exec(ast::greater, const cell_value& lhs, const cell_value& rhs) const {
  if (lhs.is<std::wstring>() && rhs.is<std::wstring>()) {
    auto& lhs_s = lhs.as<std::wstring>();
    auto& rhs_s = rhs.as<std::wstring>();
    return ed::iless(rhs_s, lhs_s);
  }
  return lhs > rhs;
}


operand engine::exec(ast::greater_equal, const cell_value& lhs, const cell_value& rhs) const {
  if (lhs.is<std::wstring>() && rhs.is<std::wstring>()) {
    auto& lhs_s = lhs.as<std::wstring>();
    auto& rhs_s = rhs.as<std::wstring>();
    return !ed::iless(lhs_s, rhs_s);
  }
  return lhs >= rhs;
}


operand engine::exec(ast::equal, const cell_value& lhs, const cell_value& rhs) const {
  if (lhs.is<std::wstring>() && rhs.is<std::wstring>()) {
    return ed::iequals(lhs.as<std::wstring>(), rhs.as<std::wstring>());
  }
  return lhs == rhs;
}


operand engine::exec(ast::not_equal, const cell_value& lhs, const cell_value& rhs) const {
  if (lhs.is<std::wstring>() && rhs.is<std::wstring>()) {
    return !ed::iequals(lhs.as<std::wstring>(), rhs.as<std::wstring>());
  }
  return lhs != rhs;
}


operand engine::exec(ast::concat, const cell_value& lhs, const cell_value& rhs) const {
  return lhs.to<std::wstring>() + rhs.to<std::wstring>();
}


operand engine::exec(ast::add, const cell_value& lhs, const cell_value& rhs) const {
  return lhs.to<double>() + rhs.to<double>();
}


operand engine::exec(ast::subtract, const cell_value& lhs, const cell_value& rhs) const {
  return lhs.to<double>() - rhs.to<double>();
}


operand engine::exec(ast::multiply, const cell_value& lhs, const cell_value& rhs) const {
  return lhs.to<double>() * rhs.to<double>();
}


operand engine::exec(ast::divide, const cell_value& lhs, const cell_value& rhs) const {
  double rhs_d = rhs.to<double>();
  if (ed::fuzzy_is_null(rhs_d)) {
    return cell_value_error::div0;
  }
  return lhs.to<double>() / rhs_d;
}


operand engine::exec(ast::power, const cell_value& lhs, const cell_value& rhs) const {
  return std::pow(lhs.to<double>(), rhs.to<double>());
}


operand engine::exec(ast::range, operand&& lhs, operand&& rhs) const {
  range_list lhs_rl = lhs.to<range_list>();
  range_list rhs_rl = rhs.to<range_list>();

  ED_EXPECTS(!lhs_rl.empty());
  ED_EXPECTS(!rhs_rl.empty());

  range& lhs_r = lhs_rl.front();
  range& rhs_r = rhs_rl.back();

  ED_EXPECTS(!lhs_r.empty());
  ED_EXPECTS(!rhs_r.empty());

  // TODO: Сделать поддержку 3D диапазонов
  ED_EXPECTS(&lhs_r.sheet() == &rhs_r.sheet());

  return range_list{lhs_r.top_left().unite(rhs_r.bottom_right())};
}


operand engine::exec(ast::plus, const cell_value& v) const {
  return v;
}


operand engine::exec(ast::minus, const cell_value& v) const {
  return -v.to<double>();
}


operand engine::exec(ast::percent, const cell_value& v) const {
  return v.to<double>() / 100.;
}


operand engine::exec(const ast::func& token, operand::list&& args) const {
  ED_EXPECTS(token.ptr);
  return token.ptr->invoke(*sheet_, std::move(args));
}


} // namespace lde::cellfy::boox::fx
