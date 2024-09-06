#include <gtest/gtest.h>

#include <lde/cellfy/boox/src/base26.h>


using namespace lde::cellfy::boox;


TEST(base26, encode) {
  std::string str;

  base26::encode(16383, str);
  ASSERT_EQ(str, "XFD");

  base26::encode(0, str);
  ASSERT_EQ(str, "A");
}


TEST(base26, decode) {
  int i = 0;

  base26::decode<char>("XFD", i);
  ASSERT_EQ(i, 16383);

  base26::decode<char>("A", i);
  ASSERT_EQ(i, 0);
}
