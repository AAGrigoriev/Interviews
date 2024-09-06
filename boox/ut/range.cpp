#include <gtest/gtest.h>

#include <array>
#include <ctime>
#include <string_view>
#include <utility>

#include "boost/date_time/posix_time/posix_time.hpp"

#include <lde/cellfy/boox/value_format.h>
#include <lde/cellfy/boox/workbook.h>
#include <lde/cellfy/boox/src/value_parser.h>
#include <lde/cellfy/fun/add_functions.h>


using namespace lde::cellfy::boox;

namespace _ {
namespace {

auto create_date_from_ymd(int y, int m, int d) {
  using date_type = boost::gregorian::date;
  return date_type{static_cast<date_type::year_type>(y), static_cast<date_type::month_type>(m), static_cast<date_type::day_type>(d)};
}


auto create_ptime_from_ymd_hms(int y, int m, int d, int h, int min, int s){
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  date g_date{static_cast<date::year_type>(y),
    static_cast<date::month_type>(m),
    static_cast<date::day_type>(d)
  };
  time_duration dur{static_cast<time_duration::hour_type>(h),
    static_cast<time_duration::min_type>(min),
    static_cast<time_duration::sec_type>(s)
  };

  return ptime(g_date, dur);
}


auto create_duration_from_hms(int h, int min, int s, int m) {
  using namespace boost::posix_time;
  return time_duration{static_cast<time_duration::hour_type>(h),
         static_cast<time_duration::min_type>(min),
         static_cast<time_duration::sec_type>(s),
         static_cast<time_duration::fractional_seconds_type>(m * 1000)
 };
}

}} // namespace _::


TEST(range, format) {
  workbook book;
  auto& sheet = *book.sheets().begin();
  sheet.cells(L"A1:C1").set_format<font_name>("Arial");

  auto format_a1c1 = sheet.cells(L"A1:C1").format();
  ASSERT_EQ(format_a1c1.get_or_default<font_name>(), "Arial");

  auto format_a2c2 = sheet.cells(L"A2:C2").format();
  ASSERT_EQ(format_a2c2.get_or_default<font_name>(), cell_format().get_or_default<font_name>());
  ASSERT_EQ(format_a2c2.get_optional<font_name>(), std::nullopt);
}


TEST(range, value) {
  workbook book;
  auto& sheet = *book.sheets().begin();
  sheet.cells({0, 0}, {2, 2}).set_value(0.123);
  sheet.cells({0, 0}, {2, 0}).set_value(L"abc");

  ASSERT_EQ(sheet.cell({0, 0}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({1, 0}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({2, 0}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({0, 1}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({1, 1}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({2, 1}).value(), cell_value(0.123));

  ASSERT_EQ(sheet.cells({0, 1}, {2, 2}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cells({0, 0}, {2, 0}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cells({0, 0}, {2, 2}).value(), cell_value());

  sheet.cell({0, 0}).set_value(123.456);
  ASSERT_EQ(sheet.cell({0, 0}).value(), cell_value(123.456));
  book.undo();
  ASSERT_EQ(sheet.cell({0, 0}).value(), cell_value(L"abc"));

  sheet.cells({0, 0}, {2, 2}).set_value({});
  ASSERT_EQ(sheet.cells({0, 1}, {2, 2}).value(), cell_value());
  ASSERT_EQ(sheet.cells({0, 0}, {2, 0}).value(), cell_value());
  book.undo();
  ASSERT_EQ(sheet.cells({0, 1}, {2, 2}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cells({0, 0}, {2, 0}).value(), cell_value(L"abc"));
}


TEST(range, text) {
  workbook book;
  auto& sheet = *book.sheets().begin();
  sheet.cells({0, 0}, {2, 2}).set_text(L"0.123");
  sheet.cells({3, 0}, {5, 2}).set_text(L"abc");

  ASSERT_EQ(sheet.cell({0, 0}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({0, 1}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({0, 2}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({1, 0}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({1, 1}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({1, 2}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({2, 0}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({2, 1}).value(), cell_value(0.123));
  ASSERT_EQ(sheet.cell({2, 2}).value(), cell_value(0.123));

  ASSERT_EQ(sheet.cell({3, 0}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({3, 1}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({3, 2}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({4, 0}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({4, 1}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({4, 2}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({5, 0}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({5, 1}).value(), cell_value(L"abc"));
  ASSERT_EQ(sheet.cell({5, 2}).value(), cell_value(L"abc"));
}


// Проверяется корректность парсинга текста.
// В документ сначала присваиваются валидные строки 'valid_text', который должен распознаться.
// Потом невалидный текст 'bad_text', который не должен распознаться.
TEST(range, value_parsing) {
  workbook book;
  auto& sheet = *book.sheets().begin();
  std::time_t t = std::time(nullptr);
  auto current_year = std::localtime(&t)->tm_year + 1900;

  std::initializer_list<std::pair<std::wstring, cell_value>> valid_text {
    {L"04/10",               _::create_date_from_ymd(current_year, 10, 4)},         ///< dd mm
    {L"04.10",               _::create_date_from_ymd(current_year, 10, 4)},         ///< dd.mm
    {L"04/10.1941",          _::create_date_from_ymd(1941, 10, 4)},                 ///< dd.mm.yyyy
    {L"02.08/2077",          _::create_date_from_ymd(2077, 8,  2)},                 ///< dd mmm yyyy
    {L"10/Апр/2047",         _::create_date_from_ymd(2047, 4, 10)},                 ///< dd mmmm yyyy
    {L"01/Мая",              _::create_date_from_ymd(current_year, 5,  1)},         ///< dd mm
    {L"10.2020",             _::create_date_from_ymd(2020, 10, 1)},
    {L"12.10.1983 10:",      _::create_ptime_from_ymd_hms(1983, 10, 12, 10, 0, 0)},
    {L"12.10.1983 10:10",    _::create_ptime_from_ymd_hms(1983, 10, 12, 10, 10, 0)},
    {L"12.10.1983 10:10:10", _::create_ptime_from_ymd_hms(1983, 10, 12, 10, 10, 10)},
    {L"10.10.1988 48:00:00", _::create_ptime_from_ymd_hms(1988, 10, 12, 0, 0, 0)},
    {L"123:10:10",           _::create_duration_from_hms(123, 10, 10, 0)},
    {L"10:20:30",            _::create_duration_from_hms(10, 20, 30, 0)},
    {L"10:20:30.333",        _::create_duration_from_hms(10, 20, 30, 333)},
    {L"10%",                 0.1},
    {L"100     %",           1.},
    {L"100 $",               100.},
    {L"123 50/2",            148.},
    {L"-12 1/2",             -12.5},
    {L"1 0/1",               1.},
    {L"1 7/10",              1.7},
    {L"1E1",                 10.},
    {L"1e2",                 100.},
    {L"0",                   0.},
    {L"true",                true},
    {L"false",               false},
    {L"TRUE",                true},
    {L"FALSE",               false}
  };

  cell_index index{};
  for (auto&& pair : valid_text) {
    sheet.cell(index).set_text(pair.first);
    ASSERT_EQ(sheet.cell(index).value(), cell_value(pair.second));
    ++index;
  }

  std::initializer_list<std::wstring> bad_text {
    L"31 02",
    L"31.02",
    L"31.11.1994",
    L"30 Фев 2007",
    L"01sfg04etehfc1997",
    L"aaa01.wr10af",
    L"a10%313",
    L"34 100/2",
    L"1 1/0"
    L"10.10.1995 10"
    L"TRUE13424",
    L"ewrFALSE"
  };

  for (auto&& str : bad_text) {
    sheet.cell(index).set_text(str);
    ASSERT_FALSE(sheet.cell(index).format().holds<number_format>());
    ++index;
  }
}


// Проверяется строка форматирования. Используется для форматирования текста в ячейке.
// В датах, в зависимости от ввода пользователя, строка форматирования меняется.
TEST(range, formatted_text) {
  std::initializer_list<std::pair<std::wstring, std::wstring>> text {
    {L"04.10.1994 10:",      L"dd.mm.yyyy hh:mm"},
    {L"04.10.1994 10:10",    L"dd.mm.yyyy hh:mm:ss"},
    {L"04.10.1994 10:10:",   L"dd.mm.yyyy hh:mm:ss"},
    {L"04.10.1994 10:10:10", L"dd.mm.yyyy hh:mm:ss"},
    {L"04.10.1989 5:",       L"dd.mm.yyyy h:mm"},
    {L"31.03.1958 5:5",      L"dd.mm.yyyy h:mm:ss"},
    {L"31.03.1958 5:5:",     L"dd.mm.yyyy h:mm:ss"},
    {L"31.03.1958 5:5:5",    L"dd.mm.yyyy h:mm:ss"},
    {L"01.01.9999 10:10:10", L"dd.mm.yyyy hh:mm:ss"},
    {L"10:",                 L"hh:mm"},
    {L"10:10",               L"hh:mm:ss"},  // Это только согласно  текущей аналитике, в MS excel hh:mm,
    {L"10:10:",              L"hh:mm:ss"},  // возможно, надо будет сделать как там.
    {L"10:10:10",            L"hh:mm:ss"},
    {L"1234:20:20",          L"[h]:mm:ss"},
    {L"10:10:10.333",        L"hh:mm:ss"},
    {L"25:10:10.333",        L"[h]:mm:ss"},
    {L"5:",                  L"h:mm"},
    {L"5:5",                 L"h:mm:ss"},
    {L"5:5:5",               L"h:mm:ss"},
    {L"0:",                  L"h:mm"},
    {L"00:00",               L"h:mm:ss"},
    {L"00:00:00",            L"h:mm:ss"}
  };

  for (const auto& p : text) {
    ASSERT_EQ(value_parser()(p.first).second, p.second);
  }
}


// Проверяется корректное переключение форматов.
TEST(range, switch_format) {
  workbook book;
  auto& sheet = *book.sheets().begin();

  // Задаём числовой формат. Можно сначала задать значение ячеки, а потом сменить формат.
  sheet.cell(0).set_text(L"06.10.1994");
  ASSERT_EQ(sheet.cell(0).format().get<number_format>(), L"dd.mm.yyyy");

  // После преобразования будет формат "Обычный текст" и строка форматирования "@".
  // И в ячейке будет строка вместо числа.
  sheet.cell(0).prepare_and_apply_number_format(L"@");
  ASSERT_EQ(sheet.cell(0).format().get<number_format>(), L"@");
  ASSERT_EQ(sheet.cell(0).value(), cell_value("34613"));

  // Включим "Текстовый формат". Введём дату. Поскольку включен текстовый формат, то формат не сменится на dd.mm.yyyy.
  // Потом переведём в автоматический и получим число, которое представляет дату. 04.10.1994 = 34611.0 .
  sheet.cell(1).prepare_and_apply_number_format(L"@");
  ASSERT_EQ(sheet.cell(1).format().get<number_format>(), L"@");
  sheet.cell(1).set_text(L"04.10.1994");

  sheet.cell(1).prepare_and_apply_number_format({});
  ASSERT_FALSE(sheet.cell(1).format().holds<number_format>());
  ASSERT_EQ(sheet.cell(1).value(), cell_value(34611.));

  // Включим "Текстовый формат". Введём дату в виде строки и потом переключим на формат даты.
  // Важно, чтобы дата не потерялась.
  sheet.cell(2).prepare_and_apply_number_format(L"@");
  sheet.cell(2).set_text(L"04.10.1994");

  sheet.cell(2).prepare_and_apply_number_format(L"dd.mm.yyyy");
  ASSERT_EQ(sheet.cell(2).format().get<number_format>(), L"dd.mm.yyyy");
  ASSERT_EQ(sheet.cell(2).value(), cell_value(boost::gregorian::date{1994, 10, 04}));

  // Введём текст в Автоматическом формате, переведём в "Текстовый". Текст так и должен остаться без изменений.
  sheet.cell(3).set_text(L"Very important text");
  ASSERT_FALSE(sheet.cell(3).format().holds<number_format>());
  sheet.cell(3).prepare_and_apply_number_format(L"@");
  ASSERT_EQ(sheet.cell(3).value(), cell_value(L"Very important text"));
  ASSERT_EQ(sheet.cell(3).format().get<number_format>(), L"@");

  // Введём ноль и переключим формат на дату и продолительность
  sheet.cell(4).set_text(L"0");
  ASSERT_FALSE(sheet.cell(4).format().holds<number_format>());
  sheet.cell(4).prepare_and_apply_number_format(L"[h]:mm:ss");
  ASSERT_EQ(sheet.cell(4).value(), cell_value(boost::posix_time::time_duration(0, 0, 0)));
  sheet.cell(4).prepare_and_apply_number_format(L"dd.mm.yyyy");
  ASSERT_EQ(sheet.cell(4).value(), cell_value(_::create_ptime_from_ymd_hms(1899, 12, 30, 0, 0, 0)));
}


// Проверяется текст на редактирование
// Основные кейсы :
// - Дата:
// -- Если у числа есть дробная часть, то отображается dd.mm.yyyy hh:mm:ss
// -- Если нету дробной части, то отображается dd.mm.yyyyy
// - Время:
// -- Если есть целая часть, то отображается dd.mm.yyyy hh:mm:ss
// -- Если нету целой части, то отображается hh:mm:ss
// - Продолжительность:
// -- Отображается с миллисекундами.
// - Процент:
// -- Если нет дробной части, то отображается без чисел после запятой
// - Строка:
// -- Без изменений
TEST(range, text_for_edit) {
  workbook book;
  auto& sheet = *book.sheets().begin();
  struct initial_value {
    std::wstring number_format;
    double       value;
  };

  // На вход формат и значение. На выход -> текст, который отдаётся пользователю.
  std::initializer_list<std::pair<initial_value, std::wstring>> list{
    {{L"dd.mm.yyyy", 123.},     L"02.05.1900"},
    {{L"dd.mm.yyyy", 123.123},  L"02.05.1900 2:57:07"},
    {{L"h:mm:ss",    0.0123},   L"0:17:43"},
    {{L"hh:mm:ss",   100.0123}, L"09.04.1900 0:17:43"},
    {{L"[h]:mm:ss",  199.4444}, L"4786:39:56.160"},
    {{L"0.00%",      100},      L"10000%"},
    {{L"@",          123},      L"123"}
  };

  cell_index index{};
  for (auto&& pair : list) {
    sheet.cell(index).set_format<number_format>(pair.first.number_format);
    sheet.cell(index).set_value(pair.first.value);
    ASSERT_EQ(sheet.cell(index).text_for_edit(), pair.second);
    ++index;
  }
}


// Проверяется автоматическая смена формата.
// В MS Excel, если выставлен формат "Число", то формат, найденныё value_parser игнорируется.
// Но значения применяются.
TEST(range, set_text) {
  workbook book;
  auto& sheet = *book.sheets().begin();

  sheet.cell(0).set_format<number_format>(L"#,##0.00");
  sheet.cell(0).set_text(L"04.10.1994");
  ASSERT_EQ(sheet.cell(0).format().get<number_format>(), L"#,##0.00");
  ASSERT_EQ(sheet.cell(0).value(), cell_value(34611.));

  // Другие форматы меняются автоматически
  sheet.cell(1).set_format<number_format>(L"0.00E+00");
  sheet.cell(1).set_text(L"04.10.1994");
  ASSERT_EQ(sheet.cell(1).format().get<number_format>(), L"dd.mm.yyyy");
  ASSERT_EQ(sheet.cell(1).value(), cell_value(34611.));

  // Если выставлен формат дробный, то ищется как смешанная дробь (1 1/3), так и обыкновенная. Т.е дата не перекрывает.
  sheet.cell(2).set_format<number_format>(L"#\" \"?/?");
  sheet.cell(2).set_text(L"1/2");
  ASSERT_EQ(sheet.cell(2).format().get<number_format>(), L"#\" \"?/?");
  ASSERT_EQ(sheet.cell(2).value(), cell_value(0.5));

  // Если выставлен формат даты/времени, то он не меняется автоматически.
  sheet.cell(3).set_format<number_format>(L"dd.mm.yyyy");
  sheet.cell(3).set_text(L"1 1/3");
  ASSERT_EQ(sheet.cell(3).format().get<number_format>(), L"dd.mm.yyyy");
  ASSERT_EQ(value_format(sheet.cell(3).format().get<number_format>())(sheet.cell(3).value()).text, L"31.12.1899");
  ASSERT_EQ(sheet.cell(3).text_for_edit(), L"31.12.1899 8:00:00");

  sheet.cell(4).set_format<number_format>(L"h:mm:ss");
  sheet.cell(4).set_text(L"10.10.1994 10:10:10");
  ASSERT_EQ(sheet.cell(4).format().get<number_format>(), L"h:mm:ss");
  ASSERT_EQ(value_format(sheet.cell(4).format().get<number_format>())(sheet.cell(4).value()).text, L"10:10:10");
  ASSERT_EQ(sheet.cell(4).text_for_edit(), L"10.10.1994 10:10:10");

  // Если формат не задан и это не числовой формат, то должен примениться формат от функций.
  lde::cellfy::fun::add_functions temp(book);
  sheet.cell(5).set_text(L"=DATE(10;10;10)");
  ASSERT_TRUE(sheet.cell(5).format().holds<number_format>());

  sheet.cell(6).set_text(L"=NOW()");
  ASSERT_TRUE(sheet.cell(6).format().holds<number_format>());

  // Если первая (относительно ввода) функция не применяет формат. То формат от функций игнорируется.
  sheet.cell(7).set_text(L"=DAY(DATE(10;10;10))");
  ASSERT_FALSE(sheet.cell(7).format().holds<number_format>());
}


// Проверяется заполнение range. C числами.
TEST(range, double_fill) {
  workbook book;
  auto& sheet = *book.sheets().begin();

  // Арифметическая прогрессия. Строка. Выделение на право.
  sheet.cell(0).set_value(2.);
  sheet.cell(1).set_value(4.);
  sheet.cell(2).set_value(6.);

  auto range_in_1 = sheet.cells(0, 2);
  auto range_out_1 = sheet.cells(0, 20);
  range_in_1.fill(range_out_1);
  // Для проверки используем сумму арифметической прогрессии.
  // Sn = (2a + d(n-1))/2 * n
  double sum_1 = 0;
  range_out_1.for_existing_cells([&sum_1](const range& r) {
    sum_1 += r.value().as<double>();
  });

  auto expect_sum_1 = (2 * 2 + 2 * 20) / 2 * 21;
  ASSERT_DOUBLE_EQ(sum_1, expect_sum_1);

  // Арифметическая прогрессия. Столбец. Выделение вниз.
  sheet.cell({0, 2}).set_value(3.);
  sheet.cell({0, 3}).set_value(7.);
  sheet.cell({0, 4}).set_value(11.);
  auto range_in_2 = sheet.cells({0, 2}, {0, 4});
  auto range_out_2 = sheet.cells({0, 2}, {0, 20});
  range_in_2.fill(range_out_2);

  double sum_2 = 0;
  range_out_2.for_existing_cells([&sum_2](const range& r) {
    sum_2 += r.value().as<double>();
  });

  auto expect_sum_2 = ((2 * 3 + 4 * 18) / 2 * 19);
  ASSERT_DOUBLE_EQ(sum_2, expect_sum_2);

  // Копия. Одно число. Выделение вверх.
  sheet.cell({1, 10}).set_value(10.);
  auto range_in_3  = sheet.cell({1, 10});
  auto range_out_3 = sheet.cells({1, 1}, {1, 10});
  range_in_3.fill(range_out_3);

  auto sum_3 = 0.;
  auto expect_sum_3 = 10. * range_out_3.rows_count();
  range_out_3.for_existing_cells([&sum_3](const range& r) {
    sum_3 += r.value().as<double>();
  });

  ASSERT_DOUBLE_EQ(sum_3, expect_sum_3);

  // МНК. Выделение влево. a = 1.3714285714285714, b = 1.2380952380952379.
  sheet.cell({10, 2}).set_value(7.);
  sheet.cell({11, 2}).set_value(9.);
  sheet.cell({12, 2}).set_value(5.);
  sheet.cell({13, 2}).set_value(2.);
  sheet.cell({14, 2}).set_value(4.);
  sheet.cell({15, 2}).set_value(1.);

  auto range_in_4 = sheet.cells({10, 2}, {15, 2});
  auto range_out_4 = sheet.cells({2, 2}, {15, 2});
  auto p = std::pair{1.3714285714285714, 1.2380952380952379};
  range_in_4.fill(range_out_4);
  cell_index i = range_out_4.top_right().column() - range_out_4.top_left().column();
  range_out_4.resize(8, {}).for_existing_cells([i, p](const range& range) mutable {
    ASSERT_DOUBLE_EQ(range.value().as<double>(), p.first *i-- + p.second);
  });
}


// Проверяется заполнение range. Cо строками.
TEST(range, string_fill) {
  workbook book;
  auto& sheet = *book.sheets().begin();

  // Вверх
  sheet.cell({10, 20}).set_value(L"A");
  sheet.cell({10, 19}).set_value(L"A");
  sheet.cell({10, 18}).set_value(L"A");
  sheet.cell({10, 17}).set_value(L"A");
  sheet.cell({10, 16}).set_value(L"A");

  auto range_in_1  = sheet.cells({10, 20}, {10, 16});
  auto range_out_1 = sheet.cells({10, 20}, {10, 0});

  range_in_1.fill(range_out_1);
  range_out_1.for_each_cell([](const range& range) {
    ASSERT_EQ(range.value().as<std::wstring>(), L"A");
  });

  //  Вниз
  sheet.cell({10, 21}).set_value(L"B");
  sheet.cell({10, 22}).set_value(L"B");
  sheet.cell({10, 23}).set_value(L"B");
  sheet.cell({10, 24}).set_value(L"B");
  sheet.cell({10, 25}).set_value(L"B");

  auto range_in_2  = sheet.cells({10, 21}, {10, 25});
  auto range_out_2 = sheet.cells({10, 21}, {10, 30});

  range_in_2.fill(range_out_2);
  range_out_2.for_each_cell([](const range& range) {
    ASSERT_EQ(range.value().as<std::wstring>(), L"B");
  });

  // Влево
  sheet.cell({9, 20}).set_value(L"C");
  sheet.cell({8, 20}).set_value(L"C");
  sheet.cell({7, 20}).set_value(L"C");
  sheet.cell({6, 20}).set_value(L"C");
  sheet.cell({5, 20}).set_value(L"C");

  auto range_in_3  = sheet.cells({9, 20}, {5, 20});
  auto range_out_3 = sheet.cells({9, 20}, {0, 20});

  range_in_3.fill(range_out_3);
  range_in_3.for_each_cell([](const range& range) {
    ASSERT_EQ(range.value().as<std::wstring>(), L"C");
  });

  // Вправо
  sheet.cell({11, 20}).set_value(L"D");
  sheet.cell({12, 20}).set_value(L"D");
  sheet.cell({13, 20}).set_value(L"D");
  sheet.cell({14, 20}).set_value(L"D");
  sheet.cell({15, 20}).set_value(L"D");

  auto range_in_4  = sheet.cells(cell_addr(11, 20), cell_addr(15, 20));
  auto range_out_4 = sheet.cells(cell_addr(11, 20), cell_addr(20, 20));

  range_in_4.fill(range_out_4);
  range_out_4.for_each_cell([](const range& range) {
    ASSERT_EQ(range.value().as<std::wstring>(), L"D");
  });
}


// Проверяется заполнение range. C формулами.
TEST(range, formula_fill) {
  workbook book;

  book.formula_parser().add_function(L"func", false, [](const worksheet&) {
    return 1.;
  });

  auto& sheet = *book.sheets().begin();

  // Вверх
  sheet.cell({11, 30}).set_text(L"=8 + 4");         // 12
  sheet.cell({11, 29}).set_text(L"=1488 + 12");     // 1500
  sheet.cell({11, 28}).set_text(L"=1 + 10 + 1945"); // 1956
  std::array expected_value = {12, 1500, 1956};

  auto range_in_1  = sheet.cells({11, 30}, {11, 28});
  auto range_out_1 = sheet.cells({11, 30}, {11, 20});

  range_in_1.fill(range_out_1);
  range_out_1.for_each_cell([expected_value](const range& range) {
    const auto remainder = (30 - range.row()) % 3;
    ASSERT_EQ(expected_value.at(remainder), range.value().as<double>());
  });

  // Заполним 1, 2, 3 столбец числами.
  for (row_index i = 0; i < 10; ++i) {
    sheet.cell({0, i}).set_value(10.);
    sheet.cell({1, i}).set_value(10.);
    sheet.cell({2, i}).set_value(10.);
  }

  // Вниз.
  sheet.cell({11, 32}).set_text(L"=A1 + 4");
  sheet.cell({11, 33}).set_text(L"=B1 + 6");
  sheet.cell({11, 34}).set_text(L"=C1 + 10");
  std::array expected_value_2 = {14, 16, 20};

  auto range_in_2  = sheet.cells({11, 32}, {11, 34});
  auto range_out_2 = sheet.cells({11, 32}, {11, 40});

  range_in_2.fill(range_out_2);
  range_out_2.for_each_cell([expected_value_2](const range& range) {
    const auto remainder = (range.row() - 32) % 3;
    ASSERT_DOUBLE_EQ(expected_value_2.at(remainder), range.value().as<double>());
  });

  // Проверка унарных операций. Влево.
  sheet.cell({10, 31}).set_text(L"=-4 + 5 + 2"); // 3

  auto range_in_3 = sheet.cell({10, 31});
  auto range_out_3 = sheet.cells({5, 31}, {10, 31});
  range_in_3.fill(range_out_3);
  range_out_3.for_existing_cells([](const auto& range) {
    ASSERT_DOUBLE_EQ(range.value(). template as<double>(), 3.);
  });


  // Проверка копирования строк в формулах. Вправо.
  sheet.cell({10, 32}).set_text(L"=\"ABC\"=\"ABC\"");
  auto range_in_4 = sheet.cell({10, 32});
  auto range_out_4 = sheet.cells({10, 32}, {14, 32});
  range_in_4.fill(range_out_4);
  range_out_4.for_existing_cells([](const auto& range) {
   ASSERT_EQ(range.value(), cell_value{true});
  });


  // Проверка копирования формул без аргументов.
  sheet.cell({8, 0}).set_text(L"=func()");
  sheet.cell({9, 0}).set_text(L"=func()");
  auto range_in_5 = sheet.cell({9, 0});
  auto range_out_5 = sheet.cells({9, 0}, {9, 10});
  range_in_5.fill(range_out_5);
  range_out_5.for_existing_cells([&sheet](const auto& range) {
    ASSERT_EQ(range.value(), sheet.cell({8, 0}).value());
  });
}


// Проверяется заполнение boox::range. Разных типов, с пробелами, в несколько строк/столбцов.
TEST(range, main_filling_cases) {
  workbook book;
  auto& sheet = *book.sheets().begin();

  //     |  0  1  ..
  //     |
  // 0   |  1
  // 1   |  -
  // 2   |  2
  // 3   |  -
  // 4   |  3
  // 5   |  -
  // 6   |  4
  // 7   |  - <- Отсюда протягиваем.
  // 8   |  5
  // 9   |  -
  // 10  |  6
  // 11  |  -
  // 12  |  7
  // 13  |  -
  // 14  |  8
  // 15  |  -
  // 16  |  9
  // 17  |  -
  // 18  |  10
  // 19  |  -
  // 20  |  11

  // С пробелами между ячейками.
  sheet.cell({0, 0}).set_value(1.);
  sheet.cell({0, 2}).set_value(2.);
  sheet.cell({0, 4}).set_value(3.);
  sheet.cell({0, 6}).set_value(4.);

  auto range_1_in = sheet.cells({0, 0}, {0, 7});
  auto range_1_out = sheet.cells({0, 0}, {0, 20});
  range_1_in.fill(range_1_out);

  range_1_out.for_existing_cells([start_index = 0, start_value = 0.](const auto& range) mutable {
    ASSERT_EQ(range.row(), start_index);
    ASSERT_DOUBLE_EQ(range.value().template as<double>(), static_cast<double>(++start_value));
    start_index += 2;
  });

  // Несколько строк.

  // C 4 столбца протягиваем 2 строки.
  //    0   1   2   3      4     5   6   7   8     9     10   11   12   13    14     15    16    17    18    19    20
  // 0  AB  1   AB  2            AB  2   AB  3           AB   3    AB   4            AB    4     AB    5           AB
  // 1  5   9   1   FIFA   FIFA  1   -1  -3  FIFA  FIFA  -5   -7   -9   FIFA  FIFA   -11   -13   -15   FIFA  FIFA  -17

  sheet.cell({0, 0}).set_value(L"AB");
  sheet.cell({1, 0}).set_value(1.);
  sheet.cell({2, 0}).set_value(L"AB");
  sheet.cell({3, 0}).set_value(2.);

  sheet.cell({0, 1}).set_value(5.);
  sheet.cell({1, 1}).set_value(9.);
  sheet.cell({2, 1}).set_value(1.);
  sheet.cell({3, 1}).set_value(L"FIFA");
  sheet.cell({4, 1}).set_value(L"FIFA");

  auto range_2_in  = sheet.cells({0, 0}, {4, 1});
  auto range_2_out = sheet.cells({0, 0}, {20, 1});

  std::vector<cell_value> values;
  range_2_in.fill(range_2_out);
  range_2_out.for_existing_cells([&](const auto& range) {
    values.push_back(range.value());
  });

  std::array expected_value {cell_value{"AB"}, cell_value{1.}, cell_value{"AB"}, cell_value{2.}};
  // C [0 - 17) - первая строка. С [17 - 38). Вторая строка.
  for (decltype(values.size()) i = 0; i < values.size() - 21; ++i) {
    if (i % 4 == 0 && i != 0) {
      expected_value[1].as<double>() += 1.;
      expected_value[3].as<double>() += 1.;
    }
    ASSERT_EQ(values[i], expected_value[i % 4]);
  }

  std::pair<double, double> a_b = {-2., 7.};
  auto x = 3.; // Стартовое значение в уравнении y = a*x+b.
  for (decltype(values.size()) i = values.size() - 16; i < values.size(); ++i) {
    if (values[i].is<std::wstring>()) {
      ASSERT_EQ(values[i].as<std::wstring>(), L"FIFA");
    } else {
      ASSERT_DOUBLE_EQ(values[i].as<double>(), a_b.first * x++ + a_b.second);
    }
  }
}


// Проверяется копирование формата.
TEST(range, fill_format) {
  workbook book;
  auto& sheet = *book.sheets().begin();

  constexpr auto f_size = 15_pt;
  //Влево копируем формат.
  sheet.cell({10, 0}).set_value(1.).set_format<font_size>(f_size);
  sheet.cell({9,  0}).set_value(2.).set_format<font_size>(f_size);
  sheet.cell({8,  0}).set_value(3.).set_format<font_size>(f_size);

  auto range_1_in = sheet.cells({8, 0}, {10, 0});
  auto range_1_out = sheet.cells({0, 0}, {10, 0});

  range_1_in.fill(range_1_out);

  range_1_out.for_each_cell([f_size](const range& range) {
    ASSERT_TRUE(range.format().holds<font_size>());
    ASSERT_EQ(range.format().get<font_size>(), f_size);
  });
}


// Проверяется копирование bool.
TEST(range, fill_bool) {
  workbook book;
  auto& sheet = *book.sheets().begin();

  sheet.cell({0, 0}).set_value(true);
  sheet.cell({1, 0}).set_value(false);

  auto range_in = sheet.cells({0, 0}, {1, 0});
  auto range_out = sheet.cells({0, 0}, {10, 0});

  range_in.fill(range_out);

  range_out.for_existing_cells([](const range& range) {
    ASSERT_EQ(range.value(), range.addr().index() % 2 ? false : true);
  });
}


// Проверяется копирование ошибок.
TEST(range, fill_error) {
  workbook book;
  auto& sheet = *book.sheets().begin();

  sheet.cell({0, 0}).set_value(cell_value_error::value);
  sheet.cell({1, 0}).set_value(cell_value_error::div0);

  auto range_in = sheet.cells({0, 0}, {1, 0});
  auto range_out = sheet.cells({0, 0}, {20, 0});

  range_in.fill(range_out);

  range_out.for_existing_cells([](const range& range) {
    ASSERT_EQ(range.value(), range.addr().index() % 2 ? cell_value_error::div0 : cell_value_error::value);
  });
}


// Проверяется General Horizontal Alignment.
// Нужно проверить:
// - Если horizontal_alignment::general, то только при отрисовке показывается выравнивание текста.
//   Поэтому, пока его не сменили, выравнивание должно иметь тип horizontal_alignment::general.
// - После ручной смены горизонтального выравнивания. Оно не должно само меняться. CEL-265.
TEST(range, general_horizontal_alignment) {
  workbook book;
  auto& sheet = *book.sheets().begin();

  std::array text_format {L"09.05.1945", L"100%", L"10:10:10"};

  auto cells = sheet.cells({0, 0}, {0, 2});
  cells.for_each_cell([beg = text_format.begin(), end = text_format.end()](auto&& rng) mutable {
    ASSERT_TRUE(beg != end);
    rng.set_text(*beg++);
  });

  cells.for_each_cell([](auto&& rng) {
    ASSERT_FALSE(rng.format().template holds<text_horizontal_alignment>());
  });

  // Когда вручную зададим, то не должно слететь. CEL-265.
  auto cells_2 = sheet.cells({1, 0}, {1, 2});
  cells_2.for_each_cell([](auto&& rng) {
    rng.template set_format<text_horizontal_alignment>(horizontal_alignment::center);
    rng.set_text(L"10000%");
  });

  cells_2.for_each_cell([](auto&& rng) {
    ASSERT_EQ(rng.format().template get<text_horizontal_alignment>(), horizontal_alignment::center);
  });
}
