#include <gtest/gtest.h>

#include <ed/config.h>

#include <ed/rasta/font.h>
#include <ed/rasta/font_picker.h>
#include <ed/rasta/font_storage.h>


int main(int argc, char* argv[]) {
  ed::rasta::font::picker().add(std::filesystem::path(ED_FONTS_DIR));
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
