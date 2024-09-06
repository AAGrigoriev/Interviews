#pragma once


#include <cctype>
#include <cmath>
#include <string>
#include <type_traits>


namespace lde::cellfy::boox::base26 {


template<typename T, typename Char>
void encode(T num, std::basic_string<Char>& encoded) {
  static_assert(std::is_integral_v<T>);
  encoded.clear();
  T n = num + 1;

  while (n > 0) {
    T rem = n % 26;
    if (rem == 0) {
      encoded.insert(encoded.begin(), Char('Z'));
      n = (n / 26) - 1;
    } else {
      encoded.insert(encoded.begin(), Char((rem - 1) + 'A'));
      n = n / 26;
    }
  }
}


template<typename Char, typename T>
void decode(const std::basic_string<Char>& encoded, T& num) noexcept {
  static_assert(std::is_integral_v<T>);
  num = 0;
  int i = 0;

  for (auto ci = encoded.rbegin(); ci != encoded.rend(); ++ci) {
    num += static_cast<T>((std::pow(26, i++) * (std::tolower(*ci) - 'a' + 1)));
  }

  --num;
}


} // namespace lde::cellfy::boox::base26
