#include <gtest/gtest.h>

#include <lde/cellfy/boox/area.h>


using namespace lde::cellfy::boox;


TEST(area, from_a1) {
  ASSERT_EQ(std::wstring(area(L"A1:H8")), L"A1:H8");
  ASSERT_EQ(std::wstring(area(L"H8")), L"H8");
}


TEST(area, disjoin) {
  area a{{0, 0}, {7, 7}}; // "A1:H8"

  auto r1 = a.disjoin({{2, 0}, {20, 20}});
  ASSERT_EQ(r1.size(), 1);
  ASSERT_EQ(std::wstring(r1.front()), L"A1:B8");

  auto r2 = a.disjoin({{0, 2}, {20, 20}});
  ASSERT_EQ(r2.size(), 1);
  ASSERT_EQ(std::wstring(r2[0]), L"A1:H2");

  auto r3 = a.disjoin({{2, 2}, {20, 20}});
  ASSERT_EQ(r3.size(), 2);
  ASSERT_EQ(std::wstring(r3[0]), L"A1:H2");
  ASSERT_EQ(std::wstring(r3[1]), L"A3:B8");

  auto r4 = a.disjoin({{0, 0}, {5, 20}});
  ASSERT_EQ(r4.size(), 1);
  ASSERT_EQ(std::wstring(r4[0]), L"G1:H8");

  auto r5 = a.disjoin({{0, 2}, {20, 20}});
  ASSERT_EQ(r5.size(), 1);
  ASSERT_EQ(std::wstring(r5[0]), L"A1:H2");

  auto r6 = a.disjoin({{0, 2}, {5, 20}});
  ASSERT_EQ(r6.size(), 2);
  ASSERT_EQ(std::wstring(r6[0]), L"A1:H2");
  ASSERT_EQ(std::wstring(r6[1]), L"G3:H8");

  auto r7 = a.disjoin({{2, 0}, {20, 20}});
  ASSERT_EQ(r7.size(), 1);
  ASSERT_EQ(std::wstring(r7[0]), L"A1:B8");

  auto r8 = a.disjoin({{0, 0}, {20, 5}});
  ASSERT_EQ(r8.size(), 1);
  ASSERT_EQ(std::wstring(r8[0]), L"A7:H8");

  auto r9 = a.disjoin({{2, 0}, {20, 5}});
  ASSERT_EQ(r9.size(), 2);
  ASSERT_EQ(std::wstring(r9[0]), L"A1:B8");
  ASSERT_EQ(std::wstring(r9[1]), L"C7:H8");

  auto r10 = a.disjoin({{2, 2}, {5, 5}});
  ASSERT_EQ(r10.size(), 4);
  ASSERT_EQ(std::wstring(r10[0]), L"A1:H2");
  ASSERT_EQ(std::wstring(r10[1]), L"A3:B8");
  ASSERT_EQ(std::wstring(r10[2]), L"G3:H8");
  ASSERT_EQ(std::wstring(r10[3]), L"C7:F8");

  auto r11 = area({4, 5}, {8, 18}).disjoin({{2, 7}, {4, 8}});
  ASSERT_EQ(std::wstring(r11[0]), L"E6:I7");
  ASSERT_EQ(std::wstring(r11[1]), L"F8:I19");
  ASSERT_EQ(std::wstring(r11[2]), L"E10:E19");
}