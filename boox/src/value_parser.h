#pragma once

#include <locale>
#include <utility>

#include <lde/cellfy/boox/cell_value.h>


namespace lde::cellfy::boox  {

class value_parser final {
public:
  using value_with_format = std::pair<cell_value, std::wstring>;

  // todo: boost locale.
  value_parser(const std::locale& loc = std::locale::classic());

  /// @brief Функиця парсинга строки.
  /// @details Если в строке была дата/процент, то возвращает cell_value и формат для этих чисел.
  ///          Если в строке был только текст или число, то возвращает пустой формат (std::wstring{}).
  /// @param mixed_fraction_search - Если true, то ищем все комбинации дроби (с целой частью и без). Если false - то, то целая часть обязательна.
  value_with_format operator()(const std::wstring& text, bool mixed_fraction_search = false) const;

private:
  const std::locale& loc_;
};

} // lde::cellfy::boox
