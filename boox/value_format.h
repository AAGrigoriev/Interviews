#pragma once


#include <locale>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <ed/core/color.h>

#include <lde/cellfy/boox/fwd.h>


namespace lde::cellfy::boox {


/// Форматирование значений ячейки.
class value_format final {
public:
  /// Результат форматирования.
  struct result {
    std::wstring               text;            /// Результирующий текст
    std::optional<ed::color>   text_color;      /// Цвет текста.
    std::optional<std::size_t> fill_positions;  /// Позиция в тексте символов, которая должна заполнить ширину ячейки.
    std::vector<std::size_t>   white_positions; /// Позиции в тексте символов, которые не должны рисоваться, но должны занимать свою ширину.
  };

public:
  /// format - строка форматирования. https://developers.google.com/sheets/api/guides/formats
  /// loc - локаль для форматирования.
  explicit value_format(const std::wstring& format, std::locale loc = std::locale());
  ~value_format();

  value_format(const value_format&) = delete;
  value_format& operator=(const value_format&) = delete;

  /// Форматировать значение value.
  result operator()(const cell_value& value) const;

  /// @brief Функция проверки секции.
  /// @param [out] - true, если есть подходящая для @value секция с токенами даты.
  bool has_date_section(double value) const;

  ///@brief Функция проверки секции.
  /// @param[out] - true, если есть подходящая для @value секция с токенами времени.
  bool has_time_section(double value) const;

  ///@brief Функция проверки секции.
  /// @param[out] - true, если есть подходящая для @value секция с токенами даты или времени.
  bool has_date_time_section(double value) const;

  /// @brief Функция проверки секции.
  /// @param [out] - true, если есть подходящая для @value секция с токенами прододжительности.
  bool has_duration_section(double value) const;

  /// @brief Функция проверки секции.
  /// @param [out] - true, если есть подходящая для @value секция с токенами експоненты.
  bool has_exponent_section(double value) const;

  /// @brief Функция проверки секции.
  /// @param [out] - true, если есть подходящая для @value секция с токенами процента.
  bool has_percent_section(double value) const;

  /// @brief Функция проверки секции.
  /// @param [out] - true, если есть подходящая для @value секция с токенами числа.
  bool has_number_section(double value) const;

  /// @brief Функция проверки секции.
  /// @param [out] - true, если есть подходящая для @value секция с токенами дроби.
  bool has_fraction_section(double value) const;

  /// @brief Функция проверки секции.
  /// @param [out] - true, если есть любая секция.
  bool has_any_section() const noexcept;

private:
  struct impl;
  std::unique_ptr<impl> impl_;
};


} // namespace lde::cellfy::boox
