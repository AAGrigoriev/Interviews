#pragma once


#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#include <boost/callable_traits/args.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/mp11.hpp>

#include <ed/core/type_traits.h>

#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/fx_operand.h>


namespace lde::cellfy::boox::fx {


/// Базовый интерфейс для функции.
class function {
public:
  virtual ~function() = default;

  /// Имя функции.
  virtual std::wstring_view name() const = 0;

  /// Если true, то ячейка с формулой, содержащая эту ф-ю, будет всегда пересчитываться.
  virtual bool is_volatile() const = 0;

  /// Вызов ф-ии.
  virtual operand invoke(const worksheet& sheet, operand::list&& args) const = 0;
};


/// Вспомогательный класс для создания ф-ии.
/// Fn - это функтор или ф-я, принимающая первым аргументом const worksheet& (лист, в рамках которого вызывается ф-я).
/// Остальные аргументы Fn должны иметь тип, в который можно конвертировать операнд operand::to.
/// В конце должны идти либо optional аргументы либо operand::list.
/// Fn должен возвращать тип конвертируемый в operand.
template<typename Fn>
class function_adapter final : public function {
public:
  function_adapter(std::wstring name, bool is_volatile, Fn&& fn);

  std::wstring_view name() const override;

  bool is_volatile() const override;

  operand invoke(const worksheet& sheet, operand::list&& args) const override;

private:
  std::wstring name_;
  bool         is_volatile_;
  Fn           fn_;
};


template<typename Fn>
function_adapter<Fn>::function_adapter(std::wstring name, bool is_volatile, Fn&& fn)
  : name_(std::move(name))
  , is_volatile_(is_volatile)
  , fn_(std::move(fn)) {
}


template<typename Fn>
std::wstring_view function_adapter<Fn>::name() const {
  return name_;
}


template<typename Fn>
bool function_adapter<Fn>::is_volatile() const {
  return is_volatile_;
}


template<typename Fn>
operand function_adapter<Fn>::invoke(const worksheet& sheet, operand::list&& args) const {
  using args_tuple_type = boost::mp11::mp_rename<
    boost::mp11::mp_transform<
      std::remove_reference_t,
      boost::mp11::mp_pop_front<
        boost::callable_traits::args_t<Fn, boost::mp11::mp_list>
      >
    >,
    std::tuple
  >;

  try {
    if constexpr (std::tuple_size_v<args_tuple_type> == 0) {
      ED_EXPECTS(args.empty());
      return fn_(sheet);
    } else {
      bool args_reversed = false;
      args_tuple_type args_tuple;

      std::apply([&args, &args_reversed](auto& ... tuple_elements) {
        auto convert_arg = [&args, &args_reversed](auto& tuple_element) {
          using tuple_element_type = ed::remove_cvref_t<decltype(tuple_element)>;

          // Если оставшиеся элементы не std::optional или не operand::list и в @args не осталось элементов, то выводим ошибку
          if constexpr (!ed::is_specialization_v<tuple_element_type, std::optional> &&
                        !std::is_same_v<tuple_element_type, operand::list>) {
            ED_EXPECTS(!args.empty());
          }

          if constexpr (std::is_same_v<tuple_element_type, operand::list>) {
            tuple_element = std::move(args);
          } else {
            if (!args.empty()) { // С учётом того, что в конце всегда идут optional.
              if (!args_reversed) {
                std::reverse(args.begin(), args.end());
                args_reversed = true;
              }
              auto arg = std::move(args.back());
              args.pop_back();
              if constexpr (std::is_same_v<tuple_element_type, operand>) {
                tuple_element = std::move(arg);
              } else if constexpr (ed::is_specialization_v<tuple_element_type, std::optional>) {
                if constexpr (std::is_same_v<tuple_element_type, std::optional<operand>>) {
                  tuple_element = std::move(arg);
                } else if constexpr (!std::is_same_v<tuple_element_type, std::optional<operand::list>>) {
                  tuple_element = arg.to<typename tuple_element_type::value_type>(); // std::optional<T>::value_type
                }
              } else {
                tuple_element = arg.to<tuple_element_type>();
              }
            }
          }
        };
        (convert_arg(tuple_elements), ...);
      }, args_tuple);

      // Если количество аргументов в @args оказалось больше, чем функция себе забрала, то нужно вывести ошибку.
      ED_EXPECTS(args.empty());

      return std::apply([&sheet, this](auto&& ... args) {
        return fn_(sheet, std::move(args) ...);
      }, args_tuple);
    }
  } catch (const bad_value_cast& e) { // Этим исключением отлавливаются ошибки конвертации из типа ошибок.
    if (const cell_value_error* error = boost::get_error_info<cell_value_error_info>(e)) {
      return *error;
    } else {
      return cell_value_error::value;
    }
  } catch (const parser_failed&) { // Если парсер не смог разобрать строку.
    return cell_value_error::value;
  } catch (const std::invalid_argument&) { // При конвертации используются функци std::stod, которая выкидывает исключения. Аналогично ошибке парсера.
    return cell_value_error::value;
  } catch (const std::out_of_range&) { // При выходе за пределы double(когда распознаём число в std::stod) или за пределы boost::date_time.
    return cell_value_error::num;
  }
}

} // namespace lde::cellfy::boox::fx
