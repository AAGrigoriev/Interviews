#include <filesystem>
#include <fstream>
#include <iostream>

#include "file.hpp"

using namespace std;

int main(int argc, char *argv[]) {

  if (argc != 3) {
    return EXIT_FAILURE;
  }

  auto path     = filesystem::current_path() / filesystem::path(argv[1]);
  auto out_path = filesystem::current_path() / filesystem::path(argv[2]);

  if (!filesystem::is_regular_file(path)) {
    std::cerr << "wrong file name" << std::endl;
    return EXIT_FAILURE;
  }

  auto vec = avito_test::sorted_vector(path);

  std::ofstream out{out_path};
  for (auto&& p : vec) {
    out << p.first << " " << p.second << std::endl;
  }
  out.close();

  return 0;
}
