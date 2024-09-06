#include <gtest/gtest.h>

#include <lde/cellfy/boox/vector_2d.h>


TEST(vector_2d, smoke) {
  lde::cellfy::boox::vector_2d<int> m(3, 4);

  int v = 0;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 4; ++c) {
      m[r][c] = v++;
    }
  }

  ASSERT_EQ(m.rows_count(), 3);
  ASSERT_EQ(m.columns_count(), 4);
  ASSERT_EQ(m[0][0], 0);
  ASSERT_EQ(m[0][3], 3);
  ASSERT_EQ(m[2][0], 8);
  ASSERT_EQ(m[2][3], 11);
}
