#pragma once


#include <memory>

#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/fx_ast.h>
#include <lde/cellfy/boox/fx_function.h>


namespace lde::cellfy::boox::fx {


/// Парсер формул.
class parser final {
public:
  parser();
  ~parser();

  parser(const parser&) = delete;
  parser& operator=(const parser&) = delete;

  /// Парсит formula выдает список элементов в обратной польской записи. Может кинуть исключение.
  void parse(const std::wstring& formula, ast::tokens& ast) const;

  /// Парсит formula выдает список элементов в обратной польской записи. Не кидает исключений. Возвращает true в случае успеха.
  bool parse_no_throw(const std::wstring& formula, ast::tokens& ast) const;

  /// Зарегистрировать функцию.
  void add_function(function_ptr func);

  /// Зарегистрировать функцию.
  template<typename Fn>
  void add_function(std::wstring name, bool is_volatile, Fn&& fn);

private:
  struct impl;
  std::unique_ptr<impl> impl_;
};


template<typename Fn>
void parser::add_function(std::wstring name, bool is_volatile, Fn&& fn) {
  add_function(std::make_shared<function_adapter<Fn>>(std::move(name), is_volatile, std::move(fn)));
}


} // namespace lde::cellfy::boox::fx
