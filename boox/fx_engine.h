#pragma once


#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/fx_ast.h>
#include <lde/cellfy/boox/fx_operand.h>


namespace lde::cellfy::boox::fx {

///@brief Функция конвертации ast в строку формулы.
std::wstring ast_to_formula(const ast::tokens& ast);


/// @brief Получить формат первой функции.
/// @details В @ast может быть больше одной функции, поэтому возвращается формат, только одной.
/// @returns Формат функции, либо пустая строка.
std::wstring get_func_format(const ast::tokens& ast);


/// Движок, который запускает вычисление формулы.
class engine final {
public:
  explicit engine(const worksheet& sheet) noexcept;

  /// Вычислить формулу по ast.
  cell_value evaluate(const ast::tokens& ast) const;

private:
  operand to_operand(const ast::number& item) const;
  operand to_operand(const ast::string& item) const;
  operand to_operand(const ast::boolean& item) const;
  operand to_operand(const ast::reference& item) const;

  operand exec(ast::less, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::less_equal, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::greater, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::greater_equal, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::equal, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::not_equal, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::concat, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::add, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::subtract, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::multiply, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::divide, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::power, const cell_value& lhs, const cell_value& rhs) const;
  operand exec(ast::range, operand&& lhs, operand&& rhs) const;
  operand exec(ast::plus, const cell_value& v) const;
  operand exec(ast::minus, const cell_value& v) const;
  operand exec(ast::percent, const cell_value& v) const;
  operand exec(const ast::func& token, operand::list&& args) const;

private:
  const worksheet* sheet_;
};


} // namespace lde::cellfy::boox::fx
