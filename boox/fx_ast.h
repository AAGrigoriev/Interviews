#pragma once


#include <string>
#include <variant>

#include <boost/container/small_vector.hpp>

#include <lde/cellfy/boox/cell_addr.h>
#include <lde/cellfy/boox/fwd.h>


namespace lde::cellfy::boox::fx::ast {


// Операнды
struct number {
  double value;
};

struct string {
  std::wstring value;
};

struct boolean {
  bool value;
};

struct reference {
  cell_addr     addr;
  unsigned char col_abs : 1;
  unsigned char row_abs : 1;
  std::wstring  sheet;
};

// Бинарные операторы
struct less {};          // <
struct less_equal {};    // <=
struct greater {};       // >
struct greater_equal {}; // >=
struct equal {};         // =
struct not_equal {};     // <>
struct concat {};        // &
struct add {};           // +
struct subtract {};      // -
struct multiply {};      // *
struct divide {};        // /
struct power {};         // ^
struct range {};         // :
// TODO: Доделать операторы объединения и пересечения диапазонов.

// Унарные операторы
struct plus {};          // +
struct minus {};         // -
struct percent {};       // %

// Вызов функции
struct func {
  function_ptr ptr;
  std::size_t  args_count;
};


using token = std::variant<
  number,
  string,
  boolean,
  reference,
  less,
  less_equal,
  greater,
  greater_equal,
  equal,
  not_equal,
  concat,
  add,
  subtract,
  multiply,
  divide,
  power,
  range,
  plus,
  minus,
  percent,
  func
>;


// Элементы в обратной польской записи (RPN).
// https://ru.wikipedia.org/wiki/Обратная_польская_запись
using tokens = boost::container::small_vector<token, 32>;


} // namespace lde::cellfy::boox::fx::ast
