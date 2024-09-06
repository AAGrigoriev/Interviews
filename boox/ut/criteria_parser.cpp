#include <gtest/gtest.h>

#include <tuple>
#include <utility>

#include <lde/cellfy/boox/cell_addr.h>
#include <lde/cellfy/boox/cell_value.h>

#include <lde/cellfy/fun/src/criteria_parser.h>


using namespace lde::cellfy::boox;
using parser = lde::cellfy::fun::criteria_parser;

// Проверяется criteria_parser с числами.
// Основные кейсы:
// - Строки <;>;<=;>=;=;<> плюс число/строка.
// На выдачу идёт функтор, который сверяет cell_value и возвращает true/false.
TEST(criteria_parser, cell_value) {
  parser parser;

  const auto true_expr = {
    std::pair{L">40",    cell_value{100.}},
    std::pair{L"<30",    cell_value{10.}},
    std::pair{L"=40",    cell_value{40.}},
    std::pair{L"<>33.3", cell_value{60.}},
    std::pair{L"4000",   cell_value{4000.}},
    std::pair{L"",       cell_value{}}
  };

  for (const auto& p : true_expr) {
    ASSERT_TRUE(parser(p.first)(p.second));
  }

  const auto false_expr = {
    std::pair{L">11",  cell_value{11.}},
    std::pair{L"<11",  cell_value{11.}},
    std::pair{L"=30",  cell_value{29.}},
    std::pair{L"<>11", cell_value{11.}},
    std::pair{L"400",  cell_value{399.}},
    std::pair{L">",    cell_value{}},
    std::pair{L"<",    cell_value{10.}}
  };

  for(const auto& p : false_expr) {
    ASSERT_FALSE(parser(p.first)(p.second));
  }
}


// Проверяется criteria_parser со строками.
// - wildcards */?/~.
// - обычные строки, тоже выстуают в качестве сравнения.
// Уточненение: Строковый поиск нечувствителен к регистру.
TEST(criteria_parser, text) {
  parser parser;

  const auto true_expr = {
    std::pair{L"It’s Over 9000!",   L"It’s Over 9000!"},
    std::pair{L"HOHOHO",            L"HOHOHO"},
    std::pair{L"=Лол Кек Чебурек",  L"Лол Кек Чебурек"},
    std::pair{L"<>Чебурек Кек Лол", L"What?"},
    std::pair{L"<>Москва",          L""},
    std::pair{L"арбуз",             L"АрБуЗ"},
    std::pair{L">Ана",              L"АНАНАС"},
    std::pair{L"<Работа",           L"Работ"},
    std::pair{L">=диплом",          L"ДИПЛОМНАЯ работа"},
    std::pair{L"<=Повторенье",      L"ПовтОреНье"}
  };

  for (const auto& p : true_expr) {
    ASSERT_TRUE(parser(p.first)(p.second));
  }

  const auto true_wildcards_expr = {
    std::pair{L"*",               L"Some text"},
    std::pair{L"*abc*",           L"SomeabcText"},
    std::pair{L"?? mind is ????", L"My mind is gone"},
    std::pair{L"2+2=4~?",         L"2+2=4?"},
    std::pair{L"?",               L"O"},
    std::pair{L"apply?",          L"apply1"},
    std::pair{L"<>abc?",          L"abc"},
    std::pair{L"abc?",            L"ABCD"},
    std::pair{L"КОщЕй*",          L"кощей БессМертНый"}
  };

  for (const auto& p : true_wildcards_expr) {
    ASSERT_TRUE(parser(p.first)(p.second));
  }
}


// Проверяются типы заданного и проверяемого значения.
TEST(criteria_parser, type) {
  const auto check_false_expr = [](auto&& first_arg, auto&& tuple) {
    std::apply([&first_arg](auto&& ...args) {
      const auto temp_func = [func = parser()(first_arg)](auto&& arg) {
        ASSERT_FALSE(func(arg));
      };
      (temp_func(args), ...);
    }, std::forward<decltype(tuple)>(tuple));
  };

  check_false_expr(L"abc",     std::make_tuple(cell_value{}, rich_text{}, 100.,  cell_value_error::div0, L"2442",   true));
  check_false_expr(L">1",      std::make_tuple(cell_value{}, rich_text{}, -100., cell_value_error::na,   L"Ab",     true));
  check_false_expr(L"#VALUE!", std::make_tuple(cell_value{}, rich_text{}, 2646., cell_value_error::num,  L"asffsa", true));
  check_false_expr(L"**a",     std::make_tuple(cell_value{}, rich_text{}, 1234., cell_value_error::null, L"abc",    true));
  check_false_expr(L"true",    std::make_tuple(cell_value{}, rich_text{}, 1234., cell_value_error::ref,  L"sdagdg", false));
  check_false_expr(L">50",     std::make_tuple(cell_value{}, rich_text{}, 49.,   cell_value_error::null, L"qwer",   true));
}


// Проверяется строкове сравнение. Если подстрока не найдена в основной строке, то возвращается std::wstring::npos.
TEST(criteria_parser, wstring_search) {
  parser p;

  const auto expr = {
    std::tuple{L"1234",   L"1234",             0},
    std::tuple{L"F",      L"ABCDF",            4},
    std::tuple{L"n",      L"printer",          3},
    std::tuple{L"base",   L"database",         4},
    std::tuple{L"margin", L"Profit Margin",    7},
    std::tuple{L"#???#",  L"Артикул #123# ID", 8},
    std::tuple{L"клад?",  L"докладная",        2},
    std::tuple{L"я",      L"ЛЯГУШКА",          1},
    std::tuple{L"*база",  L"это БАЗА",         0}
  };

  for (auto&& t : expr) {
    ASSERT_EQ(p.string_search(std::get<0>(t))(std::get<1>(t)), std::get<2>(t));
  }

  ASSERT_EQ(p.string_search(L"Типы данных")(L"1234"), std::wstring::npos);
}
