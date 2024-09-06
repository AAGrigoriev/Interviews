#include <gtest/gtest.h>

#include <lde/cellfy/boox/cell_addr.h>


using namespace lde::cellfy::boox;


TEST(cell_addr, row_column) {
  cell_addr a0(0, 0);
  ASSERT_EQ(a0.column(), 0);
  ASSERT_EQ(a0.row(), 0);
  ASSERT_EQ(a0.index(), 0);

  cell_addr a1(3, 2);
  ASSERT_EQ(a1.column(), 3);
  ASSERT_EQ(a1.row(), 2);
  ASSERT_EQ(a1.index(), 32771);

  cell_addr a2(16383, 1048575);
  ASSERT_EQ(a2.column(), 16383);
  ASSERT_EQ(a2.row(), 1048575);
  ASSERT_EQ(a2.index(), 17179869183);
}


TEST(cell_addr, to_string) {
  cell_addr a0(0, 0);
  ASSERT_EQ(std::wstring(a0), L"A1");

  cell_addr a1(3, 2);
  ASSERT_EQ(std::wstring(a1), L"D3");

  cell_addr a2(16383, 1048575);
  ASSERT_EQ(std::wstring(a2), L"XFD1048576");
}