#include <gtest/gtest.h>

#include <lde/cellfy/boox/value_format.h>
#include <lde/cellfy/boox/cell_value.h>


using namespace lde::cellfy::boox;


TEST(cell_value, ptime) {
  boost::posix_time::ptime time(boost::gregorian::date(2015, 1, 1), boost::posix_time::hours(6));
  cell_value v(time);
  ASSERT_EQ(v.to<double>(), 42005.25);
  ASSERT_EQ(v.to<boost::posix_time::ptime>(), time);

  boost::gregorian::date date(2015, 1, 1);
  cell_value v2(date);
  ASSERT_EQ(v2.to<double>(), 42005.);
  ASSERT_EQ(v2.to<boost::gregorian::date>(), date);

  boost::posix_time::time_duration duration(14, 15, 16);
  ASSERT_EQ(cell_value(duration).to<boost::posix_time::time_duration>(), duration);
}


TEST(cell_value, to_wstring) {
  ASSERT_EQ(cell_value{12345.}.to<std::wstring>(),        L"12345");
  ASSERT_EQ(cell_value{12345.0012300}.to<std::wstring>(), L"12345.00123");
  ASSERT_EQ(cell_value{12345.123}.to<std::wstring>(),     L"12345.123");
  ASSERT_EQ(cell_value{0.}.to<std::wstring>(),            L"0");
}


TEST(value_format, number) {
  ASSERT_EQ(value_format(L"####.#")(1234.59).text, L"1234.6");
  ASSERT_EQ(value_format(L"#.000")(8.9).text, L"8.900");
  ASSERT_EQ(value_format(L"0.#")(0.631).text, L"0.6");
  ASSERT_EQ(value_format(L"#.0#")(12.).text, L"12.0");
  ASSERT_EQ(value_format(L"#.0#")(1234.568).text, L"1234.57");
  ASSERT_EQ(value_format(L"???.???")(44.398).text, L" 44.398");
  ASSERT_EQ(value_format(L"???.???")(102.65).text, L"102.65 ");
  ASSERT_EQ(value_format(L"???.???")(2.8).text, L"  2.8  ");
  ASSERT_EQ(value_format(L"000.0000")(12.34).text, L"012.3400");
  ASSERT_EQ(value_format(L"#,###")(12000.).text, L"12,000");
  ASSERT_EQ(value_format(L"#,##0")(123.5).text, L"124");
  ASSERT_EQ(value_format(L"#,#")(123456789.).text, L"123,456,789");
  ASSERT_EQ(value_format(L"#.000")("raw").text, L"raw"); ///< Bugfix CEL-106.
  ASSERT_EQ(value_format(L"###0.000;\"TEXT: \"@")("raw").text, L"TEXT: raw");
  ASSERT_EQ(value_format(L"###0.000;\"TEXT: \"@")(33.).text, L"33.000");
  ASSERT_EQ(value_format(LR"(# ?/?)")(0.1).text, L"0 "); //"CEL-383".
}


TEST(value_format, fraction) {
  ASSERT_EQ(value_format(LR"(#$$???$/$000#$)")(-5.25).text, L"-5$$  1$/$0004$");
  ASSERT_EQ(value_format(LR"(# ???/???)")(5.25).text, L"5   1/4  ");
  ASSERT_EQ(value_format(LR"(# ???/???)")(5.3).text, L"5   3/10 ");
}


TEST(value_format, exponential) {
  ASSERT_EQ(value_format(LR"(0.00e+00)")(-1234500000.).text, L"-1.23e+09");
  ASSERT_EQ(value_format(LR"(0.00E-00)")(-1234500000.).text, L"-1.23E09");
}


TEST(value_format, date) {
  boost::posix_time::ptime time1(boost::gregorian::date(2021, 2, 3), boost::posix_time::time_duration(9, 7, 8));
  boost::posix_time::ptime time2(boost::gregorian::date(2021, 2, 3), boost::posix_time::time_duration(14, 0, 0));
  boost::posix_time::ptime time3(boost::gregorian::date(2021, 2, 3), boost::posix_time::time_duration(0, 17, 42, 720000));

  ASSERT_EQ(value_format(L"yy")(time1).text, L"21");
  ASSERT_EQ(value_format(L"yyyy")(time1).text, L"2021");
  ASSERT_EQ(value_format(L"m")(time1).text, L"2");
  ASSERT_EQ(value_format(L"mm")(time1).text, L"02");
  ASSERT_EQ(value_format(L"mmm")(time1).text, L"Feb");
  ASSERT_EQ(value_format(L"mmmm")(time1).text, L"February");
  ASSERT_EQ(value_format(L"mmmmm")(time1).text, L"F");
  ASSERT_EQ(value_format(L"d")(time1).text, L"3");
  ASSERT_EQ(value_format(L"dd")(time1).text, L"03");
  ASSERT_EQ(value_format(L"ddd")(time1).text, L"Wed");
  ASSERT_EQ(value_format(L"dddd")(time1).text, L"Wednesday");
  ASSERT_EQ(value_format(L"h")(time1).text, L"9");
  ASSERT_EQ(value_format(L"hh")(time1).text, L"09");
  ASSERT_EQ(value_format(L"s")(time1).text, L"8");
  ASSERT_EQ(value_format(L"ss")(time1).text, L"08");
  ASSERT_EQ(value_format(L"dd.mm.yyyy hh:mm:ss")(time1).text, L"03.02.2021 09:07:08");
  ASSERT_EQ(value_format(L"dd.mm.yyyy am/pm hh:mm:ss")(time1).text, L"03.02.2021 am 09:07:08");
  ASSERT_EQ(value_format(L"dd.mm.yyyy am/PM hh:mm:ss")(time2).text, L"03.02.2021 PM 02:00:00");
  ASSERT_EQ(value_format(L"dd.mm.yyyy A/P hh:mm:ss")(time2).text, L"03.02.2021 P 02:00:00");
  ASSERT_EQ(value_format(LR"(hh:mm A/P".M.")")(time1).text, L"09:07 A.M.");
  ASSERT_EQ(value_format(LR"(mmmm d \[dddd\])")(time1).text, L"February 3 [Wednesday]");
  ASSERT_EQ(value_format(LR"(hh:mm:ss)")(time3).text, L"00:17:43");
}


TEST(value_format, duration) {
  boost::posix_time::time_duration duration(3, 13, 41);
  ASSERT_EQ(value_format(L"[hh]:[mm]:[ss]")(duration).text, L"03:13:41");
  ASSERT_EQ(value_format(L"[mmmm]:[ss]")(duration).text, L"0193:41");
  boost::posix_time::time_duration fractional_seconds(3, 15, 18, 720000);
  ASSERT_EQ(value_format(L"[h]:mm:ss")(fractional_seconds).text, L"3:15:19");
  ASSERT_EQ(value_format(L"[h]:mm:ss.000")(fractional_seconds).text, L"3:15:18.720");
  ASSERT_EQ(value_format(L"[h]:[mm]-000:[ss]")(fractional_seconds).text, L"3:15-720:18");
  ASSERT_EQ(value_format(L"[h]:mm:ss.00")(fractional_seconds).text, L"3:15:18.72");
  ASSERT_EQ(value_format(L"[h]:mm:ss.0")(fractional_seconds).text, L"3:15:18.7");
  ASSERT_EQ(value_format(L"[h]:[mm].000000000:[ss]")(fractional_seconds).text, L"#"); // Гугл выдаёт "Недействительный формат".
}


TEST(value_format, text) {
  ASSERT_EQ(value_format(LR"(###0.000;"TEXT: "@)")(L"MyText").text, L"TEXT: MyText");
  ASSERT_EQ(value_format(LR"(###0.000;"TEXT: "@)")(123114.15115).text, L"123114.151");
}


TEST(value_format, color) {
  std::wstring fmt = LR"([Blue]#,##0;[Red]#,##0;[Green]0.0;[Magenta]@)";
  ASSERT_EQ(value_format(fmt)(1234.).text_color, ed::color::known::blue);
  ASSERT_EQ(value_format(fmt)(-1234.).text_color, ed::color::known::red);
  ASSERT_EQ(value_format(fmt)(0.).text_color, ed::color::known::green);
  ASSERT_EQ(value_format(fmt)(L"MyText").text_color, ed::color::known::magenta);
}


TEST(value_format, condition) {
  std::wstring fmt = LR"([>1000]"HIGH";[Green][<=200]"LOW";0000)";
  ASSERT_EQ(value_format(fmt)(1005.).text, L"HIGH");
  ASSERT_EQ(value_format(fmt)(32.).text, L"LOW");
  ASSERT_EQ(value_format(fmt)(527.).text, L"0527");
}


TEST(value_format, fill_position) {
  auto not_valid_date = -1.5;
  auto format = value_format(LR"(dd.mm.yyyy)");
  auto result = format(not_valid_date);

  ASSERT_EQ(result.text, L"#");
  ASSERT_TRUE(result.fill_positions.has_value() && result.fill_positions.value() == 0);
}


TEST(value_format, section) {
  value_format date_format(LR"(dd.mm.yyyy)");
  ASSERT_TRUE(date_format.has_date_section(123.));

  value_format duration_format(LR"([h]:mm:ss)");
  ASSERT_TRUE(duration_format.has_duration_section(228.));

  value_format exponent_format(LR"(0.00E+00)");
  ASSERT_TRUE(exponent_format.has_exponent_section(1333.));

  value_format percent_format(LR"(0.00%)");
  ASSERT_TRUE(percent_format.has_percent_section(1.));

  value_format not_date_format(LR"(mm:ss)");
  ASSERT_TRUE(!not_date_format.has_date_section(1488.) && not_date_format.has_time_section(1488.));

  value_format not_time_section(LR"(mm)");
  ASSERT_TRUE(!not_time_section.has_time_section(1488.) && not_time_section.has_date_section(1488.));

  value_format number_format(LR"(#,##0.00)");
  ASSERT_TRUE(number_format.has_number_section(333.));
  ASSERT_FALSE(percent_format.has_number_section(333.) && exponent_format.has_number_section(333.));

  value_format fraction_format(LR"(# ?/?)");
  ASSERT_TRUE(fraction_format.has_fraction_section(0.25));
}


// Проверяются строки форматирования со системными(региональными) настройками.
// Пока они не применяются, так как они зависят от локали, но разбираются.
TEST(value_format, system_information) {
  boost::posix_time::ptime time(boost::gregorian::date(2021, 2, 3), boost::posix_time::time_duration(14, 0, 0));

  ASSERT_EQ(value_format(LR"([$USD-409]dd.mm.yyyy)")(time).text,    L"03.02.2021");
  ASSERT_EQ(value_format(L"[$-F400]h:mm:ss AM/PM")(time).text,      L"2:00:00 PM");
  ASSERT_EQ(value_format(L"[$-F800]dd.mm.yyyy")(time).text,         L"03.02.2021");
  ASSERT_EQ(value_format(L"[$ABFDFS-FBAF8]dd.mm.yyyy")(time).text,  L"03.02.2021");
}
