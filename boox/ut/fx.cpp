#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <tuple>

#include <boost/date_time/gregorian/greg_duration_types.hpp>
#include <boost/format.hpp>

#include <ed/core/math.h>

#include <lde/cellfy/boox/fx_engine.h>
#include <lde/cellfy/boox/fx_parser.h>
#include <lde/cellfy/boox/workbook.h>

#include <lde/cellfy/fun/add_functions.h>
#include <lde/cellfy/fun/src/functions.h>


using namespace lde::cellfy::boox;


TEST(fx, check_rpn) {
  fx::ast::tokens ast;
  fx::parser().parse(L"(8 + 2 * 5)/(1 + 3 * 2 - 4)", ast);
  ASSERT_EQ(ast.size(), 13);
  ASSERT_EQ(std::get<fx::ast::number>(ast[0]).value, 8.);
  ASSERT_EQ(std::get<fx::ast::number>(ast[1]).value, 2.);
  ASSERT_EQ(std::get<fx::ast::number>(ast[2]).value, 5.);
  ASSERT_TRUE(std::holds_alternative<fx::ast::multiply>(ast[3]));
  ASSERT_TRUE(std::holds_alternative<fx::ast::add>(ast[4]));
  ASSERT_EQ(std::get<fx::ast::number>(ast[5]).value, 1.);
  ASSERT_EQ(std::get<fx::ast::number>(ast[6]).value, 3.);
  ASSERT_EQ(std::get<fx::ast::number>(ast[7]).value, 2.);
  ASSERT_TRUE(std::holds_alternative<fx::ast::multiply>(ast[8]));
  ASSERT_TRUE(std::holds_alternative<fx::ast::add>(ast[9]));
  ASSERT_EQ(std::get<fx::ast::number>(ast[10]).value, 4.);
  ASSERT_TRUE(std::holds_alternative<fx::ast::subtract>(ast[11]));
  ASSERT_TRUE(std::holds_alternative<fx::ast::divide>(ast[12]));
}


TEST(fx, parse_reference) {
  fx::parser pr;
  fx::ast::tokens ast;

  pr.parse(L"AB65", ast);
  ASSERT_TRUE(!ast.empty());
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast.front()).addr), L"AB65");
  ASSERT_FALSE(std::get<fx::ast::reference>(ast.front()).col_abs);
  ASSERT_FALSE(std::get<fx::ast::reference>(ast.front()).row_abs);

  pr.parse(L"$AB65", ast);
  ASSERT_TRUE(!ast.empty());
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast.front()).addr), L"AB65");
  ASSERT_TRUE(std::get<fx::ast::reference>(ast.front()).col_abs);
  ASSERT_FALSE(std::get<fx::ast::reference>(ast.front()).row_abs);

  pr.parse(L"AB$65", ast);
  ASSERT_TRUE(!ast.empty());
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast.front()).addr), L"AB65");
  ASSERT_FALSE(std::get<fx::ast::reference>(ast.front()).col_abs);
  ASSERT_TRUE(std::get<fx::ast::reference>(ast.front()).row_abs);

  pr.parse(L"$AB$65", ast);
  ASSERT_TRUE(!ast.empty());
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast.front()).addr), L"AB65");
  ASSERT_TRUE(std::get<fx::ast::reference>(ast.front()).col_abs);
  ASSERT_TRUE(std::get<fx::ast::reference>(ast.front()).row_abs);

  pr.parse(L"Лист1!A1", ast);
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast.front()).addr), L"A1");
  ASSERT_EQ(std::get<fx::ast::reference>(ast.front()).sheet, L"Лист1");

  pr.parse(L"Sheet1!A1", ast);
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast.front()).addr), L"A1");
  ASSERT_EQ(std::get<fx::ast::reference>(ast.front()).sheet, L"Sheet1");

  pr.parse(L"'Sheet Name'!$A$1", ast);
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast.front()).addr), L"A1");
  ASSERT_TRUE(std::get<fx::ast::reference>(ast.front()).col_abs);
  ASSERT_TRUE(std::get<fx::ast::reference>(ast.front()).row_abs);
  ASSERT_EQ(std::get<fx::ast::reference>(ast.front()).sheet, L"Sheet Name");

  pr.parse(L"(8 + 2 * 5)/ 'Имя Листа'!AB65 * (1 + 3 * 2 - 4)", ast);
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast.at(5)).addr), L"AB65");
  ASSERT_EQ(std::get<fx::ast::reference>(ast.at(5)).sheet, L"Имя Листа");
}


TEST(fx, func) {
  fx::parser pr;
  fx::ast::tokens ast;

  pr.add_function(L"arg_count", false, [](const worksheet&, fx::operand::list&& args) {
    return static_cast<double>(args.size());
  });

  pr.add_function(L"summ", false, [](const worksheet&, double d1, double d2) {
    return d1 + d2;
  });

  pr.add_function(L"len", false, [](const worksheet&, std::wstring&& text) {
    return static_cast<double>(text.length());
  });

  pr.add_function(L"opt_func", false, [](const worksheet&, std::optional<double> val) {
    return val.value_or(1);
  });

  pr.add_function(L"ATAN2", false, [](const worksheet&, double d1) {
    return d1;
  });

  pr.add_function(L"TRUE", false, [](const worksheet&) {
    return true;
  });

  pr.add_function(L"error", false, [](const worksheet&, cell_value&& v) {
    return v.type() == cell_value_type::error ? true : false;
  });

  pr.parse(L"ARG_COUNT()", ast);
  ASSERT_EQ(ast.size(), 1);
  ASSERT_EQ(std::get<fx::ast::func>(ast[0]).ptr->name(), L"arg_count");
  ASSERT_EQ(std::get<fx::ast::func>(ast[0]).args_count, 0);

  pr.parse(L"ARG_COUNT('Sheet Name'!A1, 500.5, AB65)", ast);
  ASSERT_EQ(ast.size(), 4);
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast[0]).addr), L"A1");
  ASSERT_EQ(std::get<fx::ast::reference>(ast[0]).sheet, L"Sheet Name");
  ASSERT_EQ(std::get<fx::ast::number>(ast[1]).value, 500.5);
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast[2]).addr), L"AB65");
  ASSERT_TRUE(std::get<fx::ast::reference>(ast[2]).sheet.empty());
  ASSERT_EQ(std::get<fx::ast::func>(ast[3]).ptr->name(), L"arg_count");
  ASSERT_EQ(std::get<fx::ast::func>(ast[3]).args_count, 3);

  // Должно быть 4 элемента. Функция, 2 ячейки, range. И у каждой ячейки должно быть заполнено поле sheet.
  pr.parse(L"arg_count(Sheet2!A1:B3)", ast);
  ASSERT_EQ(ast.size(), 4);
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast[0]).addr), L"A1");
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast[0]).sheet), L"Sheet2");
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast[1]).addr), L"B3");
  ASSERT_EQ(std::wstring(std::get<fx::ast::reference>(ast[1]).sheet), L"Sheet2");
  ASSERT_TRUE(std::get_if<fx::ast::range>(&ast[2]));
  ASSERT_EQ(std::get<fx::ast::func>(ast[3]).ptr->name(), L"arg_count");
  ASSERT_EQ(std::get<fx::ast::func>(ast[3]).args_count, 1);

  workbook book;
  auto& sheet0 = book.sheets().front();
  fx::engine ng(sheet0);
  cell_value val;

  pr.parse(L"ARG_COUNT(\"string\", 500.5, AB65) + SUMM((8 + 2 * 5)/(1 + 3 * 2 - 4), 100)", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(109.));

  pr.parse(LR"(len(""))", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value{0.});

  // Если аргументов больше, чем функция может принять, то выводится ошибка.
  pr.parse(L"summ(10., 10., 10.)", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value{cell_value_error::na});

  // Если аргументов меньше (и они не optional), чем функция может принять, то выводится ошибка.
  pr.parse(L"summ(10.)", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value{cell_value_error::na});

  // Если в конце идут optional, то они могут не заполняться.
  pr.parse(L"opt_func()", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value{1.});

  pr.parse(L"opt_func(10)", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value{10.});

  // Если функция имеет название совпадающее с ячейкой. Например ATAN2, где ATAN - col, 2 - row. То с приоритетом ищется функция.
  pr.parse(L"ATAN2(10.)", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value{10.});

  // Если функция иммеет название TRUE() или FALSE(). То с приоритетом ищется функция.
  pr.parse(L"TRUE()", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, true);

  // Функции должны принимать ошибки, которые возникли во время расчёта параметров функции.
  pr.parse(L"error(A1/0)", ast);
  ASSERT_EQ(ng.evaluate(ast), true);
}


//TEST(fx, benchmark) {
//  // Около 2.5 секунд в релизе
//  std::wstring f = L"(8 + 2 * 5)/(1 + 3 * 2 - 4)";
//  fx::ast::tokens ast;
//  fx::parser pr;
//  for (int i = 0; i < 1000000; ++i) {
//    pr.parse(f, ast);
//  }
//}


TEST(fx, exec) {
  using namespace std::string_literals;

  workbook book;
  auto& sheet0 = book.sheets().front();
  auto& sheet1 = book.emplace_sheet(1);
  sheet1.rename(L"SecondSheet");

  fx::parser pr;
  fx::engine ng(sheet0);
  fx::ast::tokens ast;
  cell_value val;

  pr.parse(L"=(8 + 2 * 5)/(1 + 3 * 2 - 4)", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(6.));

  pr.parse(L"8 + 2 > 3 * 2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(true));

  pr.parse(L"8 + 2 >= 3 * 2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(true));

  pr.parse(L"8 + 2 < 3 * 2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(false));

  pr.parse(L"8 + 2 <= 3 * 2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(false));

  pr.parse(L"8 + 2 = 3 * 2 + 4", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(true));

  pr.parse(L"8 + 2 <> 3 * 2 + 4", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(false));

  pr.parse(L"10 + -2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(8.));

  pr.parse(L"10 + +2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(12.));

  pr.parse(L"1 + 3 * 3^2 - 4", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(24.));

  pr.parse(L"1 + (3 * 3)^2 - 4", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(78.));

  pr.parse(L"10%", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(0.1));

  pr.parse(L"=3000 * 10%", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(300.));

  pr.parse(L"=-(3>2)", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(-1.));

  pr.parse(L"=-(-(3>2))", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(1.));

  sheet0.cells(L"A1").set_value(550.123);
  pr.parse(L"A1 + 5", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(555.123));

  sheet0.cells(L"A1").set_value(550.123);
  sheet1.cells(L"B10").set_value(5.);
  pr.parse(L"=A1 + SecondSheet!B10", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, cell_value(555.123));

  // Проверяется текстовое сравнение ячеек.
  sheet0.cells(L"A1").set_value(L"AntOn");
  sheet0.cells(L"A2").set_value(L"aNtoN");
  pr.parse(L"=A1=A2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, true);

  pr.parse(L"=A1>A2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, false);

  pr.parse(L"=A1<A2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, false);

  pr.parse(L"=A1<=A2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, true);

  pr.parse(L"=A1>=A2", ast);
  val = ng.evaluate(ast);
  ASSERT_EQ(val, true);

  // Проверяется сравнения с разными типами. B7 пустая.
  sheet0.cells(L"B1").set_value(cell_value_error::value);
  sheet0.cells(L"B2").set_value(cell_value_error::name);
  sheet0.cells(L"B3").set_value(10.);
  sheet0.cells(L"B4").set_value(L"some text");
  sheet0.cells(L"B5").set_value(true);
  sheet0.cells(L"B6").set_value(false);

  for (auto&& bin_op : {L">"s, L"<"s, L"<="s, L">="s, L"<>"s}) {
    // Если есть ошибка в ячейке, то она всегда выводится.
    for (auto&& cell : {L"B2"s, L"B3"s, L"B4"s, L"B5"s, L"B6"s, L"B7"s}) {
      pr.parse(L"=B1" + bin_op + cell, ast);
      ASSERT_EQ(ng.evaluate(ast), cell_value_error::value);
    }
  }

  // Bool идут после ошибок по приоритетам. И если сверяются две bool ячейки, то TRUE ячейка больше FALSE ячейки.
  for (auto&& cell : {L"B3"s, L"B4"s, L"B6"s, L"B7"s}) {
    pr.parse(L"=B5>" + cell, ast);
    ASSERT_EQ(ng.evaluate(ast), true);
  }

  // Пустые ячейки наоборот. Всех меньше.
  for (auto&& cell : {L"B3"s, L"B4"s, L"B5"s, L"B6"s}) {
    pr.parse(L"B7<" + cell, ast);
    ASSERT_EQ(ng.evaluate(ast), true);
  }

  pr.parse(L"B7=B8", ast);
  ASSERT_EQ(ng.evaluate(ast), true);

  // В сложном выражении должны возвращаться ошибки до тех пор, пока они не будут обработаны.
  pr.parse(L"=(10 + 33) * 33 + B1", ast);
  ASSERT_EQ(ng.evaluate(ast), cell_value_error::value);
}


// Проверяется функция VLOOKUP.
TEST(fx, vpr) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  sheet.cell({0, 1}).set_value(0.);
  sheet.cell({0, 2}).set_value(1.);
  sheet.cell({0, 3}).set_value(2.);
  sheet.cell({0, 4}).set_value(3.);
  sheet.cell({0, 5}).set_value(4.);

  sheet.cell({1, 1}).set_value(0.);
  sheet.cell({1, 2}).set_value(1.);
  sheet.cell({1, 3}).set_value(2.);
  sheet.cell({1, 4}).set_value(3.);
  sheet.cell({1, 5}).set_value(4.);

  auto range = sheet.cells({0, 1}, {1, 5});

  auto value_1 = vlook_up(sheet, {3.}, {range}, 2, {});
  ASSERT_DOUBLE_EQ(value_1.as<double>(), 3.);

  auto value_2 = vlook_up(sheet, {4.}, {range}, 2, {true});
  ASSERT_DOUBLE_EQ(value_2.as<double>(), 4.);

  sheet.cell({2, 0}).set_value(L"A");
  sheet.cell({2, 1}).set_value(L"ABC");
  sheet.cell({2, 2}).set_value(L"ABCD");
  sheet.cell({2, 3}).set_value(L"ABCDE");

  sheet.cell({3, 0}).set_value(L"A");
  sheet.cell({3, 1}).set_value(L"ABC");
  sheet.cell({3, 2}).set_value(L"ABCD");
  sheet.cell({3, 3}).set_value(L"ABCDE");

  auto string_range = sheet.cells({2, 0}, {3, 3});

  auto string_value_1 = vlook_up(sheet, {L"ABC"}, {string_range}, 2, {false});
  ASSERT_EQ(string_value_1.as<std::wstring>(), L"ABC");

  auto string_value_2 = vlook_up(sheet, {L"A"}, {string_range}, 2, {true});
  ASSERT_EQ(string_value_2.as<std::wstring>(), L"A");

  sheet.cell({4, 0}).set_value(10.);
  sheet.cell({4, 3}).set_value(12.);
  sheet.cell({4, 5}).set_value(16.);

  sheet.cell({5, 0}).set_value(11.);
  sheet.cell({5, 1}).set_value(134.);
  sheet.cell({5, 2}).set_value(2.);
  sheet.cell({5, 3}).set_value(299.);
  sheet.cell({5, 4}).set_value(105.);
  sheet.cell({5, 5}).set_value(-5.);

  auto partialy_empty_range = sheet.cells({4, 0}, {5, 5});

  auto number_value_1 = vlook_up(sheet, {12.}, {partialy_empty_range}, 2, {}); // true
  ASSERT_DOUBLE_EQ(number_value_1.as<double>(), 299.);

  auto number_value_2 = vlook_up(sheet, {16.}, {partialy_empty_range}, 1, {false});
  ASSERT_DOUBLE_EQ(number_value_2.as<double>(), 16.);

  auto ref_value = vlook_up(sheet, {12.}, {partialy_empty_range}, 9999, {});
  ASSERT_EQ(ref_value.as<cell_value_error>(), cell_value_error::ref);

  auto na_value = vlook_up(sheet, {}, {partialy_empty_range}, 1, {});
  ASSERT_EQ(na_value.as<cell_value_error>(), cell_value_error::na);

  auto max_value = vlook_up(sheet, {9999.}, {partialy_empty_range}, 1, {});
  ASSERT_DOUBLE_EQ(max_value.as<double>(), 16.);

  auto na_value_2 = vlook_up(sheet, {0.}, {partialy_empty_range}, 2, {});
  ASSERT_EQ(na_value.as<cell_value_error>(), cell_value_error::na);
}


// Проверяется функция AVERAGE.
// - Проверить на пустых ячейках. Ошибка div/0
// - Проверить на ячейках разных типов, учитываются только ячейки с double.
TEST(fx, average) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  sheet.cell(0).set_value(0.);
  sheet.cell(1).set_value(0.);
  sheet.cell(2).set_value(0.);

  ASSERT_DOUBLE_EQ(average(sheet, {}, {fx::range_list{sheet.cells(0, 2)}}).to<double>(), 0.);

  ASSERT_EQ(average(sheet, {}, {fx::range_list{sheet.cells(3, 6)}}), cell_value(cell_value_error::div0));

  sheet.cell(7).set_value(10.5);
  sheet.cell(8).set_value(12.8);
  sheet.cell(9).set_value(13.3);

  ASSERT_DOUBLE_EQ(average(sheet, {}, {fx::range_list{sheet.cells(7, 9)}}).to<double>(), 12.2);

  sheet.cell(10).set_value(L"ABC");
  sheet.cell(11).set_value(true);
  sheet.cell(12).set_value(1.);

  ASSERT_DOUBLE_EQ(average(sheet, {}, {fx::range_list{sheet.cells(10, 12)}}).to<double>(), 1.);

  sheet.cell(13).set_value(-10.);
  sheet.cell(14).set_value(15.);
  sheet.cell(15).set_value(5.);

  ASSERT_DOUBLE_EQ(average(sheet, {}, {fx::range_list{sheet.cells(13, 15)}, 2.}).to<double>(), 3.);

  sheet.cell(16).set_value(1.);
  sheet.cell(17).set_value(L"ssfsf");
  sheet.cell(18).set_value(cell_value_error::name);

  ASSERT_EQ(average(sheet, fx::range_list{sheet.cells(16, 18)}, {fx::range_list{sheet.cells(13, 15)}, 2.}), cell_value_error::name);
}


// Проверяется функция COUNT.
// - Проверить на пустых ячейках. Значение = 0.
// - Проверить на ячейках разных типов, учитываются только ячейки с типом double.
// - Проверить на пустых ячейках, но заданы строки с числами и без чисел.
TEST(fx, count) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  const auto result_1 = count(sheet, {fx::range_list{sheet.cells(0, 2)}});
  ASSERT_DOUBLE_EQ(result_1, 0.);

  const auto result_2 = count(sheet, {fx::range_list{sheet.cells(0, 2)}, "1", "2", 100., "sfdasfd1", "1asfasf"});
  ASSERT_DOUBLE_EQ(result_2, 3.);

  sheet.cell(3).set_value(L"ABC");
  sheet.cell(4).set_value(1.);
  sheet.cell(5).set_value(true);
  sheet.cell(6).set_value(cell_value_error::div0);

  const auto result_3 = count(sheet, {fx::range_list{sheet.cells(3, 6)}});
  ASSERT_DOUBLE_EQ(result_3, 1);
}


// Проверяется функция CONCATENATE.
// - Проверить на ошибках.
// - Проверить с числами, bool, строками.
TEST(fx, concatenate) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  sheet.cell(0).set_value(cell_value_error::name);
  sheet.cell(1).set_value(1.);
  sheet.cell(2).set_value(L"ABC");
  const auto error_result = concatenate(sheet, {fx::range_list{sheet.cells(0, 2)}});
  ASSERT_EQ(error_result, cell_value(cell_value_error::name));

  sheet.cell(3).set_value(10.);
  sheet.cell(4).set_value(L"100500");
  sheet.cell(5).set_value(true);
  const auto mixed_result = concatenate(sheet, {fx::range_list{sheet.cells(3, 5)}});
  ASSERT_EQ(mixed_result, cell_value(L"10100500TRUE"));

  sheet.cell(6).set_value(100000.);
  sheet.cell(7).set_value(-20.);

  const auto string_row_result = concatenate(sheet, {L"Stream population ", fx::range_list{sheet.cell(6)}, L" is ", fx::range_list{sheet.cell(7)}, " millions"});
  ASSERT_EQ(string_row_result, cell_value((boost::wformat(L"Stream population %d is %d millions") % 100000 % -20).str()));
}


// Проверяется функция SQRT.
// - Проверить со всеми типами. Работает с number, none, bool.
// - Если в ячейке ошибка, то ошибка копируется в эту ячейку.
TEST(fx, sqrt) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  sheet.cell(0).set_value(cell_value_error::div0);
  const auto error_result = sqrt(sheet, sheet.cell(0).value());
  ASSERT_EQ(error_result, cell_value(cell_value_error::div0));

  sheet.cell(1).set_value(-4.);
  const auto error_result_2 = sqrt(sheet, sheet.cell(1).value());
  ASSERT_EQ(error_result_2, cell_value(cell_value_error::num));

  sheet.cell(2).set_value(9.);
  const auto normal_result = sqrt(sheet, sheet.cell(2).value());
  ASSERT_EQ(normal_result, cell_value(3.));

  sheet.cell(3).set_value({});
  const auto normal_result_1 = sqrt(sheet, sheet.cell(3).value());
  ASSERT_EQ(normal_result_1, cell_value(0.));

  sheet.cell(4).set_value(L"blablabla");
  const auto error_result_3 = sqrt(sheet, sheet.cell(4).value());
  ASSERT_EQ(error_result_3, cell_value(cell_value_error::value));

  ASSERT_EQ(sqrt(sheet, -0.), cell_value{0.}); // CEL-543.
}


// Проверяются функции UPPER, LOWER.
// - Если ошибка, то она копируется.
// - Если число - без изменений.
// - Строка, bool, rich_text - в нижний регистр.
TEST(fx, upper_lower) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(lower(sheet, cell_value_error::na), cell_value{cell_value_error::na});
  ASSERT_EQ(upper(sheet, cell_value_error::na), cell_value{cell_value_error::na});

  ASSERT_EQ(lower(sheet, 0.), cell_value{L"0"});
  ASSERT_EQ(upper(sheet, 1.), cell_value{L"1"});

  ASSERT_EQ(lower(sheet, L"AIGEN"), cell_value{L"aigen"});
  ASSERT_EQ(upper(sheet, L"aigen"), cell_value{L"AIGEN"});

  ASSERT_EQ(lower(sheet, true),  cell_value{L"true"});  // todo: locale
  ASSERT_EQ(upper(sheet, false), cell_value{L"FALSE"}); // todo: locale

  rich_text text;
  text.push_back({.text = L"hohoho", .format = {}});
  ASSERT_EQ(lower(sheet, text), cell_value{L"hohoho"});
  ASSERT_EQ(upper(sheet, text), cell_value{L"HOHOHO"});
}


// Проверяется функция AND.
// - Если ошибка, то она копируется.
// - Если числа, то все кроме 0 - true.
// - Если строки и пустые ячейки в первом аргументе, то ошибка #VALUE!. Во втором и далее аргументах не учитываются.
// - Пустые ячеки работают аналогично строкам.
TEST(fx, and) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  constexpr auto string = L"Best job i ever had (Fury 2014)";
  ASSERT_EQ(and_(sheet, 100., {string, {}}),               cell_value{true});
  ASSERT_EQ(and_(sheet, string, {true, true, true}),       cell_value{true});
  ASSERT_EQ(and_(sheet, {}, {true, 9999.}),                cell_value{true});
  ASSERT_EQ(and_(sheet, true, {false}),                    cell_value{false});
  ASSERT_EQ(and_(sheet, false, {true, 1., true}),          cell_value{false});
  ASSERT_EQ(and_(sheet, true, {true, string, {}, 0.}),     cell_value{false});
  ASSERT_EQ(and_(sheet, cell_value_error::num, {}),        cell_value{cell_value_error::num});
  ASSERT_EQ(and_(sheet, string, {}),                       cell_value{cell_value_error::value});
  ASSERT_EQ(and_(sheet, string, {string, string, string}), cell_value{cell_value_error::value});
  ASSERT_EQ(and_(sheet, {}, {string}),                     cell_value{cell_value_error::value});
  ASSERT_EQ(and_(sheet, {}, {}),                           cell_value{cell_value_error::value});
  ASSERT_EQ(and_(sheet, true, {cell_value_error::name}),   cell_value{cell_value_error::name});

  sheet.cell(0).set_value(true);
  sheet.cell(1).set_value(true);
  sheet.cell(2).set_value(cell_value_error::name);

  ASSERT_EQ(and_(sheet, fx::range_list{sheet.cells(0, 3)}, {true, true}),        cell_value_error::name);
  ASSERT_EQ(and_(sheet, true, {true, false, fx::range_list{sheet.cells(0, 3)}}), cell_value_error::name);
}


// Проверяется функция ABS.
// - Ошибки копируются.
// - Из чисел берётся модуль.
// - Пустая ячейка - 0.
// - Строки - #VALUE!.
TEST(fx, abs) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(abs_(sheet, cell_value_error::num), cell_value{cell_value_error::num});
  ASSERT_EQ(abs_(sheet, {}),                    cell_value{0.});
  ASSERT_EQ(abs_(sheet, {-10E7}),               cell_value{10E7});
  ASSERT_EQ(abs_(sheet, {10E7}),                cell_value{10E7});
  EXPECT_THROW(abs_(sheet, L"gsfhsdfhfh"),      std::invalid_argument);
}


// Проверяется функиция NOT.
// - Ошбика копируется.
// - Если 0 - true, иначе false.
// - Если true - false, false - true.
// - Если строки - #VALUE!.
TEST(fx, not) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(not_(sheet, 0.),                    cell_value{true});
  ASSERT_EQ(not_(sheet, L"Press F"),            cell_value{cell_value_error::value});
  ASSERT_EQ(not_(sheet, true),                  cell_value{false});
  ASSERT_EQ(not_(sheet, cell_value_error::ref), cell_value{cell_value_error::ref});
}


// Проверяется функция LEFT.
// - Ошибка копируется.
// - bool/number конвертируются в строку.
// - string обрабатывается как есть.
TEST(fx, left) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(left(sheet, L"NIGGER", {-10.}),          cell_value{cell_value_error::value});
  ASSERT_EQ(left(sheet, L"Отель Кооператор", {5.}),  cell_value{L"Отель"});
  ASSERT_EQ(left(sheet, true, {}),                   cell_value{L"T"}); // todo локаль.
  ASSERT_EQ(left(sheet, 1000., {999999999.}),        cell_value{L"1000"});
  ASSERT_EQ(left(sheet, cell_value_error::null, {}), cell_value{cell_value_error::null});
  ASSERT_EQ(left(sheet, L"123", 0.),                 cell_value{std::wstring{}});
}


// Проверяется функция PROPER.
TEST(fx, proper) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(proper(sheet, L"1abc2def-tyu"),        cell_value{L"1Abc2Def-Tyu"});
  ASSERT_EQ(proper(sheet, L"this is a TITLE"),     cell_value{L"This Is A Title"});
  ASSERT_EQ(proper(sheet, L"2-way street"),        cell_value{L"2-Way Street"});
  ASSERT_EQ(proper(sheet, L"76BudGet"),            cell_value{L"76Budget"});
  ASSERT_EQ(proper(sheet, true),                   cell_value{L"True"});
  ASSERT_EQ(proper(sheet, cell_value_error::div0), cell_value{cell_value_error::div0});
  ASSERT_EQ(proper(sheet, 333.),                   cell_value{L"333"});
}


// Проверяется функция MID.
TEST(fx, mid) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(mid(sheet, L"123", 0, 100),                 cell_value{cell_value_error::value});
  ASSERT_EQ(mid(sheet, L"123", 1, -100),                cell_value{cell_value_error::value});
  ASSERT_EQ(mid(sheet, L"123", 1, 10000),               cell_value{L"123"});
  ASSERT_EQ(mid(sheet, cell_value_error::null, 1, 100), cell_value{cell_value_error::null});
  ASSERT_EQ(mid(sheet, L"фис", 1, 0),                   cell_value{L""});
  ASSERT_EQ(mid(sheet, L"some text", 1000, 1),          cell_value{L""});
  ASSERT_EQ(mid(sheet, true, 1, 4),                     cell_value{L"TRUE"});
  ASSERT_EQ(mid(sheet, 123., 1, 3),                     cell_value{"123"});
  ASSERT_EQ(mid(sheet, L"", 1, 10),                     cell_value{L""});
}


// Проверяется функция LEN.
TEST(fx, len) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(len(sheet, 123.),                   cell_value{3.});
  ASSERT_EQ(len(sheet, L"123"),                 cell_value{3.});
  ASSERT_EQ(len(sheet, cell_value_error::null), cell_value{cell_value_error::null});
  ASSERT_EQ(len(sheet, {}),                     cell_value{0.});
}


// Проверяется функция IFERROR
TEST(fx, iferror) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(iferror(sheet, {}, {}),                                         cell_value{0.});
  ASSERT_EQ(iferror(sheet, cell_value_error::value, {}),                    cell_value{0.});
  ASSERT_EQ(iferror(sheet, 10., cell_value_error::na),                      cell_value{10.});
  ASSERT_EQ(iferror(sheet, cell_value_error::null, cell_value_error::name), cell_value{cell_value_error::name});
  ASSERT_EQ(iferror(sheet, true, false),                                    cell_value{true});
  ASSERT_EQ(iferror(sheet, L"123", L"456"),                                 cell_value{L"123"});
}


// Проверяется функция OR
TEST(fx, or) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  constexpr auto string = L"Джордж Оруэлл 1984";
  ASSERT_EQ(or_(sheet, {}, {}),                               cell_value{cell_value_error::value});
  ASSERT_EQ(or_(sheet, cell_value_error::div0, {true, true}), cell_value{cell_value_error::div0});
  ASSERT_EQ(or_(sheet, true, {cell_value_error::na}),         cell_value{cell_value_error::na});
  ASSERT_EQ(or_(sheet, string, {string, string, string}),     cell_value{cell_value_error::value});
  ASSERT_EQ(or_(sheet, string, {true}),                       cell_value{true});
  ASSERT_EQ(or_(sheet, true, {false}),                        cell_value{true});
  ASSERT_EQ(or_(sheet, 123., {}),                             cell_value{true});

  sheet.cell(0).set_value(true);
  sheet.cell(1).set_value(true);
  sheet.cell(2).set_value(cell_value_error::name);

  ASSERT_EQ(or_(sheet, fx::range_list{sheet.cells(0, 2)}, {true, true}),   cell_value{cell_value_error::name});
  ASSERT_EQ(or_(sheet, true, {true, false, true, cell_value_error::null}), cell_value{cell_value_error::null});
}


// Проверяется функиця DECIMAL
TEST(fx, decimal) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();
  constexpr auto long_string = "ABC472343724247748327DEF84236748328675478635678"
    "674238576783456783782465867348592385763257873469834598573832583258234765678"
    "324675786345673845678364756783856685834FFF6734567867834358493589235832095395"
    "3450493595583485935034534532454358987987690078766767C0CF";

  // Ошибки конвератции теперь обрабатываются до вызова функции.
  ASSERT_EQ(decimal(sheet, long_string, 36),                          cell_value_error::num);
  ASSERT_EQ(decimal(sheet, "110.3", 2),                               cell_value_error::num);
  ASSERT_EQ(decimal(sheet, "110", 1.),                                cell_value_error::num);
  ASSERT_EQ(decimal(sheet, "110", 37.),                               cell_value_error::num);
  ASSERT_EQ(decimal(sheet, "-123", 8.),                               cell_value_error::num);
  ASSERT_EQ(decimal(sheet, "12?3", 8.),                               cell_value_error::num);
  ASSERT_EQ(decimal(sheet, "99",   2.),                               cell_value_error::num);
  ASSERT_EQ(decimal(sheet, "0", 2),                                                      0.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "f", 16.).as<double>(),                               15.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "F", 16.).as<double>(),                               15.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "123456712345671234", 8.).as<double>(), 2941117697323674.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "11", 2.).as<double>(),                                3.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "22", 3.).as<double>(),                                8.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "33", 4.).as<double>(),                               15.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "77", 8.).as<double>(),                               63.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "99", 10.).as<double>(),                              99.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "FF", 16.).as<double>(),                             255.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "jJ", 20.).as<double>(),                             399.);
  ASSERT_DOUBLE_EQ(decimal(sheet, "Zz", 36.).as<double>(),                            1295.);
}


// Проверяется функции RAND и RANDBETWEEN
TEST(fx, rand) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  // rand между [0; 1). Но распределение std::uniform_real_distribution, может выдать 1. Например при переполнении.
  ASSERT_TRUE(ed::fuzzy_less_equal(rand(sheet), 1.));

  // rand_between между [-4; 10]
  const auto rand_between_1 = rand_between(sheet, -4., 10.).as<double>();
  ASSERT_TRUE(ed::fuzzy_greater_equal(rand_between_1, -4.) && ed::fuzzy_less_equal(rand_between_1, 10.));

  // С дробными числами. Происходит округление в большую сторону.
  ASSERT_TRUE(ed::fuzzy_equal(rand_between(sheet, 1.03, 1.9).as<double>(), 2.));

  // Если bottom > top, то выдаётся ошибка #NUM!
  ASSERT_EQ(rand_between(sheet, 10, 0), cell_value_error::num);
}


// Проверяется функция COUNTA. Подсчитывает все существующие ячейки.
TEST(fx, counta) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_DOUBLE_EQ(count_a(sheet, {}, {}),                                                 0.);
  ASSERT_DOUBLE_EQ(count_a(sheet, {1.}, fx::operand::list{1., 2.}),                        3.);
  ASSERT_DOUBLE_EQ(count_a(sheet, {cell_value_error::div0}, {}),                           1.);
  ASSERT_DOUBLE_EQ(count_a(sheet, {-100.}, fx::operand::list{1., cell_value_error::null}), 3.);
}


// Проверяется функция AVERAGEA. Подсчитывает double, bool.
TEST (fx, averagea) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(average_a(sheet, {10.}, fx::operand::list{1., cell_value_error::na}),                     cell_value_error::na);
  ASSERT_EQ(average_a(sheet, {cell_value_error::null}, fx::operand::list{1., true, 2.}),              cell_value_error::null);
  ASSERT_EQ(average_a(sheet, {}, {}),                                                                 cell_value_error::div0);
  ASSERT_DOUBLE_EQ(average_a(sheet, {3.}, fx::operand::list{L"121", false}).as<double>(),             1.);
  ASSERT_DOUBLE_EQ(average_a(sheet, {true}, fx::operand::list{true, true}).as<double>(),              1.);
  ASSERT_DOUBLE_EQ(average_a(sheet, {L"fsdsfsf"}, fx::operand::list{L"13134", L"1414"}).as<double>(), 0.);
}


// Проверяются функции atan, cos, sin, tan.
TEST(fx, trigonometry) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(cos_(sheet,  cell_value_error::na), cell_value_error::na);
  ASSERT_EQ(sin_(sheet,  cell_value_error::na), cell_value_error::na);
  ASSERT_EQ(tan_(sheet,  cell_value_error::na), cell_value_error::na);
  ASSERT_EQ(atan_(sheet, cell_value_error::na), cell_value_error::na);
  ASSERT_EQ(acos_(sheet, cell_value_error::na), cell_value_error::na);
  ASSERT_EQ(asin_(sheet, cell_value_error::na), cell_value_error::na);
  ASSERT_EQ(atan_(sheet, cell_value_error::na), cell_value_error::na);

  ASSERT_EQ(atan2_(sheet, cell_value_error::na, cell_value_error::div0), cell_value_error::na);
  ASSERT_EQ(atan2_(sheet, {}, cell_value_error::null),                   cell_value_error::null);
  ASSERT_EQ(atan2_(sheet, 100., 0.),                                     cell_value_error::div0);

  ASSERT_EQ(cos_(sheet, 0.),  1.);
  ASSERT_EQ(sin_(sheet, 0.),  0.);
  ASSERT_EQ(tan_(sheet, 0.),  0.);
  ASSERT_EQ(atan_(sheet, 0.), 0.);
  ASSERT_EQ(asin_(sheet, 0.), 0.);

  const auto acos_0 = std::acos(0.);
  const auto atan2_1 = std::atan2(1., 1.);
  ASSERT_DOUBLE_EQ(acos_(sheet, 0.).to<double>(),      acos_0);
  ASSERT_DOUBLE_EQ(atan2_(sheet, 1., 1.).to<double>(), atan2_1);

  ASSERT_EQ(cos_(sheet, false),  1.);
  ASSERT_EQ(sin_(sheet, false),  0.);
  ASSERT_EQ(tan_(sheet, false),  0.);
  ASSERT_EQ(atan_(sheet, false), 0.);
  ASSERT_EQ(asin_(sheet, false), 0.);

  ASSERT_DOUBLE_EQ(acos_(sheet, false).to<double>(),       acos_0);
  ASSERT_DOUBLE_EQ(atan2_(sheet, true, true).to<double>(), atan2_1);

  ASSERT_EQ(cos_(sheet, {}),  1.);
  ASSERT_EQ(sin_(sheet, {}),  0.);
  ASSERT_EQ(tan_(sheet, {}),  0.);
  ASSERT_EQ(atan_(sheet, {}), 0.);
  ASSERT_EQ(asin_(sheet, {}), 0.);

  ASSERT_DOUBLE_EQ(acos_(sheet, {}).to<double>(), acos_0);

  ASSERT_EQ(cos_(sheet, L"0"),  1.);
  ASSERT_EQ(sin_(sheet, L"0"),  0.);
  ASSERT_EQ(tan_(sheet, L"0"),  0.);
  ASSERT_EQ(atan_(sheet, L"0"), 0.);

  ASSERT_DOUBLE_EQ(acos_(sheet, L"0").to<double>(), acos_0);
}


// Проверяется функция sum_if.
// У функции имеется опциональный параметр.
// Нужно проверить кейсы:
// 1) Опциональный range находится левее.
// 2) Находится выше.
// 3) Находится ниже.
// 4) Находится правее.
// 5) Имеются пересечения с обязательным range.
// 6) Отсутвие опционального параметра.
// 7) Присутсвуют ошибки в обязательном range. Но при этом отсутвует опциональный.
// 8) Присутсвуют ошибки в необязатльном range. И присутвует обязательный.
TEST(fx, sum_if) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  // Подготовка для условия, где опциональный справа/слева.
  // 0 1 2 3 4 5 6 7 8
  // 1 2 3 4   5 6 7 8
  for (column_index i{}; i < 4; ++i) {
    sheet.cell({i, 0}).set_value(static_cast<double>(i + 1));
  }

  for (column_index i{5}; i <= 8; ++i) {
    sheet.cell({i, 0}).set_value(static_cast<double>(i));
  }

  // Слева.
  ASSERT_EQ(sum_if(sheet, {sheet.cells({0, 0}, {3, 0})}, L">0", fx::range_list{sheet.cells({5, 0}, {8, 0})}), 26.);
  // Справа.
  ASSERT_EQ(sum_if(sheet, {sheet.cells({5, 0}, {8, 0})}, L">0", fx::range_list{sheet.cells({0, 0}, {3, 0})}), 10.);

  // Подготовка условий, где опциональный параметр сверху/снизу.
  for (row_index i{}; i < 4; ++i) {
    sheet.cell({0, i}).set_value(static_cast<double>(i + 1));
  }

  for (row_index i{5}; i <= 8; ++i) {
    sheet.cell({0, i}).set_value(static_cast<double>(i));
  }

  // Снизу.
  ASSERT_EQ(sum_if(sheet, {sheet.cells({0, 0}, {0, 3})}, L">0", fx::range_list{sheet.cells({0, 5}, {0, 8})}), 26.);
  // Сверху.
  ASSERT_EQ(sum_if(sheet, {sheet.cells({0, 5}, {0, 8})}, L">0", fx::range_list{sheet.cells({0, 0}, {0, 3})}), 10.);

  // Отсутвует опциональный параметр.
  // Особенность. Если встречаем ошибку, то её не обрабатываем.
  sheet.cell({0, 4}).set_value(cell_value_error::div0);

  ASSERT_EQ(sum_if(sheet, {sheet.cells({0, 0}, {0, 4})}, L">0", {}), 10.);

  sheet.cell({0, 9}).set_value(10.);

  // Присутсвует опциональный параметр и в опциональном range есть ошибка. То должна выводится ошибка.
  ASSERT_EQ(sum_if(sheet, {sheet.cells({0, 5}, {0, 9})}, L">0", fx::range_list{sheet.cells({0, 0}, {0, 4})}), cell_value_error::div0);

  // Проверяется наложение обязательного range и опционального.
  for (row_index i{10}; i < 20; i++) {
    sheet.cell({0, i}).set_value(static_cast<double>(i));
    sheet.cell({1, i}).set_value(static_cast<double>(1));
  }

  ASSERT_EQ(sum_if(sheet, {sheet.cells({0, 10}, {1, 19})}, L">0", fx::range_list{sheet.cells({1, 10}, {1, 19})}), 10.);
}


// Проверяется функция RIGHT.
// Кейсы:
// 1) Num_chars must be greater or equal to zero.
// 2) If num_chars is greater than the length of text, RIGHT return all text.
// 3) If num_chars is ommited, it is assumed to be 1.
TEST(fx, right) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(right(sheet, L"1234", -1.),             cell_value_error::value);
  ASSERT_EQ(right(sheet, L"1234", 1000000000.),     L"1234");
  ASSERT_EQ(right(sheet, L"1234", {}),              L"4");
  ASSERT_EQ(right(sheet, 1234., 100),               L"1234");
  ASSERT_EQ(right(sheet, true,  4),                 L"TRUE"); // todo locale
  ASSERT_EQ(right(sheet, {}, {}),                   cell_value{});
  ASSERT_EQ(right(sheet, cell_value_error::na, {}), cell_value_error::na);
  ASSERT_EQ(right(sheet, L"MAMA MIA", 3.),          L"MIA");
}


// Проверяется функция XOR.
// Кейсы:
// 1) If an array or reference argument contains text or empty cells, those values are ignored.
// 2) If the specified range contains no logical values, XOR returns the #VALUE! error value.
// 3) The result of XOR is TRUE when the number of TRUE inputs is odd and FALSE when the number of TRUE inputs is even.
TEST(fx, XOR) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(xor_(sheet, {}, {}),                                        cell_value_error::value);
  ASSERT_EQ(xor_(sheet, {L"rrawr"}, {{L"fafasfasf"}, {L"fafsasfafs"}}), cell_value_error::value);
  ASSERT_EQ(xor_(sheet, {cell_value_error::div0}, {true, true}),        cell_value_error::div0);
  ASSERT_EQ(xor_(sheet, {true}, {true, cell_value_error::name}),        cell_value_error::name);
  ASSERT_EQ(xor_(sheet, {true}, {}),                                    true);
  ASSERT_EQ(xor_(sheet, {false}, {true}),                               true);
  ASSERT_EQ(xor_(sheet, {}, {true}),                                    true);
  ASSERT_EQ(xor_(sheet, {true}, {true, true}),                          true);
  ASSERT_EQ(xor_(sheet, {true}, {true, true, true}),                    false);
}


// Проверяется функция SEARCH.
// 1) The SEARCH function is not case sensitive.
// 2) You can use the wildcard characters — the question mark (?) and asterisk (*) in the @find_text.
// 3) If the value of find_text is not found, the #VALUE! error value is returned.
// 4) If the start_num argument is omitted, it is assumed to be 1.
// 5) If start_num is not greater than 0 (zero) or is greater than the length of the within_text argument, the #VALUE! error value is returned.
TEST(fx, search) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(search(sheet, {},        {},                  {}), cell_value_error::value);
  ASSERT_EQ(search(sheet, L"123",    L"1",                {}), cell_value_error::value);
  ASSERT_EQ(search(sheet, L"1",      L"111111",           9.), cell_value_error::value);
  ASSERT_EQ(search(sheet, L"e",      L"Statement",        6.), 7.);
  ASSERT_EQ(search(sheet, L"margin", L"Profit Margin",    {}), 8.);
  ASSERT_EQ(search(sheet, L"#???#",  L"Артикул #123# ID", {}), 9.);
  ASSERT_EQ(search(sheet, L"клад?",  L"докладная",        {}), 3.);
}


// Проверяются функции YEAR, MONTH, DAY.
TEST(fx, year_month_day) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(year_(sheet, {}),                        1899.);
  ASSERT_EQ(year_(sheet, 1000.),                     1902.);
  ASSERT_EQ(year_(sheet, L""),                       1899.);
  ASSERT_EQ(year_(sheet, true),                      1899.);
  ASSERT_EQ(year_(sheet, false),                     1899.);
  ASSERT_EQ(year_(sheet, L"100%"),                   1899.);
  ASSERT_EQ(year_(sheet, L"1 1/3"),                  1899.);
  ASSERT_THROW(year_(sheet, L"01.01.10000"),         parser_failed);
  ASSERT_THROW(year_(sheet, L"04.10.1700"),          parser_failed);
  ASSERT_THROW(year_(sheet, L"abc"),                 parser_failed);
  ASSERT_THROW(year_(sheet, cell_value_error::name), bad_value_cast);

  ASSERT_EQ(month_(sheet, {}),                       12.);
  ASSERT_EQ(month_(sheet, L""),                      12.);
  ASSERT_EQ(month_(sheet, true),                     12.);
  ASSERT_EQ(month_(sheet, false),                    12.);
  ASSERT_EQ(month_(sheet, 1000.),                    9.);
  ASSERT_EQ(month_(sheet, L"1231313"),               3.);
  ASSERT_EQ(month_(sheet, L"09.03.1961"),            3.);
  ASSERT_THROW(month_(sheet, L"24.07.1812"),         parser_failed);
  ASSERT_THROW(month_(sheet, cell_value_error::ref), bad_value_cast);

  ASSERT_EQ(day_(sheet, {}),                        30.);
  ASSERT_EQ(day_(sheet, L""),                       30.);
  ASSERT_EQ(day_(sheet, true),                      31.);
  ASSERT_EQ(day_(sheet, false),                     30.);
  ASSERT_EQ(day_(sheet, 10000.),                    18.);
  ASSERT_EQ(day_(sheet, L"123456"),                 3.);
  ASSERT_EQ(day_(sheet, L"09.09.1952"),             9.);
  ASSERT_THROW(day_(sheet, cell_value_error::null), bad_value_cast);
}


// Проверяется функции DATE и TIME.
TEST(fx, date) {
  using namespace lde::cellfy::fun;

  workbook book;
  auto& sheet = book.sheets().front();

  ASSERT_EQ(date_(sheet, 0., 0., 0.),        cell_value_error::num); // Дата 1899 0 0 не подхоит.
  ASSERT_EQ(date_(sheet, {}, {}, 30.),       boost::gregorian::date(1899, 12, 30));
  ASSERT_EQ(date_(sheet, 0., 0., 30.),       boost::gregorian::date(1899, 12, 30));
  ASSERT_EQ(date_(sheet, 1., 0., 0.),        boost::gregorian::date(1900, 11, 30));
  ASSERT_EQ(date_(sheet, 1899., 10., 1.),    boost::gregorian::date(3799, 10, 1));
  ASSERT_EQ(date_(sheet, 2020., 1., 12.),    boost::gregorian::date(2020, 1, 12));
  ASSERT_EQ(date_(sheet, 2020., -1., 12.),   boost::gregorian::date(2019, 11, 12)); // Месяц отнимаем.
  ASSERT_THROW(date_(sheet, 10000., 1., 1.), std::out_of_range); // 9999 год последний.

  ASSERT_DOUBLE_EQ(time_(sheet, 12., 0., 0.).as<double>(),         0.5);
  ASSERT_DOUBLE_EQ(time_(sheet, 16., 48., 10.).as<double>(),       0.70011574074074079);
  ASSERT_DOUBLE_EQ(time_(sheet, 1000., -100., -100.).as<double>(), 41.596064814814817);
  ASSERT_EQ(time_(sheet, -1000., 1000., 1000.),                    cell_value_error::num);
}


// Проверяются функция EDATE и EOMONTH.
// EMONTH:
// - If start_date is not a valid date, EOMONTH returns the #NUM! error value.
// - If start_date plus months yields an invalid date, EOMONTH returns the #NUM! error value.
// EDATE:
// - If start_date is not a valid date, EDATE returns the #VALUE! error value.
// - If months is not an integer, it is truncated.
TEST(fx, edate) {
  using namespace lde::cellfy::fun;
  using g_date = boost::gregorian::date;

  workbook book;
  auto& sheet = book.sheets().front();

  g_date start_date{2018, 2, 1};
  g_date leap_year{2020, 1, 31};
  g_date end_of_month{2019, 2, 28};
  g_date not_valid_date{1600, 1, 1};
  ASSERT_EQ(edate(sheet, start_date, 1.),     g_date(2018, 3, 1));
  ASSERT_EQ(edate(sheet, start_date, 3.),     g_date(2018, 5, 1));
  ASSERT_EQ(edate(sheet, start_date, -1.),    g_date(2018, 1, 1));
  ASSERT_EQ(edate(sheet, start_date, -2.),    g_date(2017, 12, 1));
  ASSERT_EQ(edate(sheet, leap_year, 1.),      g_date(2020, 2, 29));
  ASSERT_EQ(edate(sheet, end_of_month, 1.),   g_date(2019, 3, 28));

  ASSERT_THROW(edate(sheet, not_valid_date, 1.), std::out_of_range);

  g_date add_month{1999, 12, 15};
  g_date end_month{2011, 2, 23};
  ASSERT_EQ(eomonth(sheet, add_month, 2.),      g_date(2000, 2, 29));
  ASSERT_EQ(eomonth(sheet, end_month, -2.),     g_date(2010, 12, 31));

  ASSERT_THROW(eomonth(sheet, not_valid_date, 1.), std::out_of_range);
}


// Проверяется функция DATEDIFF.
TEST(fx, datediff) {
  using namespace lde::cellfy::fun;
  using g_date = boost::gregorian::date;
  using tuple = std::tuple<cell_value, cell_value, cell_value, cell_value>;

  workbook book;
  auto& sheet = book.sheets().front();

  std::array<tuple, 6> param_result{
    tuple{g_date{2001, 1, 1},   g_date{2003, 1, 1},                 L"Y",                     2.},
    tuple{g_date{2001, 6, 1},   g_date{2002, 8, 15},                L"D",                   440.},
    tuple{g_date{2001, 6, 1},   g_date{2002, 8, 15},                L"YD",                   75.},
    tuple{g_date{2001, 1, 1},   g_date{2020, 3, 16},                L"MD",                   15.},
    tuple{g_date{1999, 1, 10},  g_date{2021, 8, 1},                 L"YM",                    7.},
    tuple{g_date{1999, 1, 10},  g_date{2021, 3, 3}, cell_value_error::num, cell_value_error::num}
  };

  for (auto&& t : param_result) {
    ASSERT_EQ(date_diff(sheet, std::move(std::get<0>(t)), std::move(std::get<1>(t)), std::move(std::get<2>(t))), std::get<3>(t));
  }

  ASSERT_THROW(date_diff(sheet, cell_value_error::na, cell_value_error::null, L"Y"), bad_value_cast);
  ASSERT_THROW(date_diff(sheet, g_date{1900, 1, 1}, cell_value_error::null,   L"Y"), bad_value_cast);
}


// Проверяется функция WEEKDAY.
TEST(fx, weekday) {
  using namespace lde::cellfy::fun;
  using g_date = boost::gregorian::date;
  using tuple = std::tuple<cell_value, std::optional<int>, cell_value>;

  workbook book;
  auto& sheet = book.sheets().front();

  g_date start_date{2008, 2, 14};
  std::array<tuple, 11> param_result{
    tuple{start_date, {}, 5.},
    tuple{start_date, 1,  5.},
    tuple{start_date, 2,  4.},
    tuple{start_date, 3,  3.},
    tuple{start_date, 11, 4.},
    tuple{start_date, 12, 3.},
    tuple{start_date, 13, 2.},
    tuple{start_date, 14, 1.},
    tuple{start_date, 15, 7.},
    tuple{start_date, 16, 6.},
    tuple{start_date, 17, 5.}
  };

  for (auto&& t : param_result) {
    ASSERT_EQ(weekday(sheet, std::move(std::get<0>(t)), std::move(std::get<1>(t))), std::get<2>(t));
  }

  ASSERT_THROW(weekday(sheet, cell_value_error::na, {}), bad_value_cast);
}


// Проверяется функция WORKDAY
// - If any argument is not a valid date, WORKDAY returns the #VALUE! error value.
// - If start_date plus days yields an invalid date, WORKDAY returns the #NUM! error value.
// - If days is not an integer, it is truncated.
TEST(fx, workday) {
  using namespace lde::cellfy::fun;
  using g_date = boost::gregorian::date;
  using tuple = std::tuple<cell_value, int, std::optional<fx::operand>, cell_value>;

  workbook book;
  auto& sheet = book.sheets().front();
  sheet.cell(0).set_text(L"26.11.2008");
  sheet.cell(1).set_text(L"04.12.2008");
  sheet.cell(2).set_text(L"21.01.2009");
  sheet.cell(3).set_value(cell_value_error::name);
  sheet.cell(4).set_value(L"not valid text");
  sheet.cell(5).set_value(100.);

  g_date start_date  {2022, 12, 5};
  g_date start_date_2{2008, 10, 1};
  std::array<tuple, 6> param_result{
    tuple{start_date,             6,  {},                                  g_date{2022, 12, 13}},
    tuple{start_date,            -6,  {},                                  g_date{2022, 11, 25}},
    tuple{g_date{2022, 12, 11},  -1,  {},                                  g_date{2022, 12, 9}},  // если начинаем с выходного дня.
    tuple{g_date{2022, 12, 10},   1,  {},                                  g_date{2022, 12, 12}}, // если начинаем с выходного дня.
    tuple{start_date_2,         151,  {},                                  g_date{2009,  4, 30}},
    tuple{start_date_2,         151,  {fx::range_list{sheet.cells(0, 2)}}, g_date{2009,  5,  5}},
 };

  for (auto&& t : param_result) {
    ASSERT_EQ(workday(sheet, std::move(std::get<0>(t)), std::move(std::get<1>(t)), std::move(std::get<2>(t))), std::get<3>(t));
  }

  ASSERT_THROW(workday(sheet, g_date{boost::gregorian::max_date_time}, 2.,                                  {}), std::out_of_range);
  ASSERT_THROW(workday(sheet, start_date_2,                            1., {fx::range_list{sheet.cells(4, 5)}}), parser_failed);
  ASSERT_THROW(workday(sheet, start_date_2,                          151., {fx::range_list{sheet.cells(0, 3)}}), bad_value_cast);
}


// Проверяется функция NETWORKDAYS
TEST(fx, networkdays) {
  using namespace lde::cellfy::fun;
  using g_date = boost::gregorian::date;

  workbook book;
  auto& sheet = book.sheets().front();
  sheet.cell(0).set_text(L"01.10.2012");
  sheet.cell(1).set_text(L"01.03.2013");
  sheet.cell(2).set_text(L"22.11.2012");
  sheet.cell(3).set_text(L"04.12.2012");
  sheet.cell(4).set_text(L"21.01.2013");

  ASSERT_EQ(networkdays(sheet, sheet.cell(0).value(), sheet.cell(1).value(),                                {}), 110.);
  ASSERT_EQ(networkdays(sheet, sheet.cell(0).value(), sheet.cell(1).value(),             sheet.cell(2).value()), 109.);
  ASSERT_EQ(networkdays(sheet, sheet.cell(0).value(), sheet.cell(1).value(), fx::range_list{sheet.cells(2, 4)}), 107.);
  ASSERT_EQ(networkdays(sheet, g_date{2022, 12, 30},  g_date{2022, 12, 19},                                 {}), -10.);
}


// Проверяется функция WEEKNUM
TEST(fx, weeknum) {
  using namespace lde::cellfy::fun;
  using g_date = boost::gregorian::date;
  using tuple  = std::tuple<cell_value, std::optional<int>, cell_value>;

  workbook book;
  auto& sheet = book.sheets().front();

  // Взято из примеров Google Sheets.
  std::array<tuple, 14> param_result{
    tuple{g_date{2020, 12, 31},  {}, 53.},
    tuple{g_date{2021,  1,  1},  {},  1.},
    tuple{g_date{2021,  1,  3},   2,  1.},
    tuple{g_date{2021,  1,  4},   2,  2.},
    tuple{g_date{2020,  4,  7},  12, 15.},
    tuple{g_date{2015, 10,  1},  15, 40.},
    tuple{g_date{2015,  1, 15},  21,  3.},
    tuple{g_date{2016, 11, 12},  14, 46.},
    tuple{g_date{2015,  2, 15},  17,  8.},
    tuple{g_date{2014,  1,  1},  11,  1.},
    tuple{g_date{2017,  7, 15},   1, 28.},
    tuple{g_date{2018,  3, 10},  13, 11.},
    tuple{g_date{2017, 11, 24},  16, 47.},
    tuple{g_date{2016,  4,  2},  15, 14.}
  };

  for (auto&& t : param_result) {
    ASSERT_EQ(weeknum(sheet, std::move(std::get<0>(t)), std::move(std::get<1>(t))), std::get<2>(t));
  }
}


// Проверяется функция DAYS360
TEST(fx, days360) {
  using namespace lde::cellfy::fun;
  using g_date = boost::gregorian::date;
  using tuple  = std::tuple<cell_value, cell_value, std::optional<bool>, cell_value>;

  workbook book;
  auto& sheet = book.sheets().front();

  std::array<tuple, 3> param_result{
    tuple{g_date{2011, 1, 30}, g_date{2011, 2, 1},   {},   1.},
    tuple{g_date{2011, 1, 1},  g_date{2011, 12, 31}, {}, 360.},
    tuple{g_date{2011, 1, 1},  g_date{2011, 2, 1},   {},  30.}
  };

  for (auto&& t : param_result) {
    ASSERT_EQ(days360(sheet, std::move(std::get<0>(t)), std::move(std::get<1>(t)), std::move(std::get<2>(t))), std::get<3>(t));
  }
}
