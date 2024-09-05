#include <lde/cellfy/boox/fx_parser.h>

#include <boost/functional/hash.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/spirit/home/x3.hpp>

#include <ed/core/assert.h>
#include <ed/core/unicode.h>

#include <lde/cellfy/boox/exception.h>
#include <lde/cellfy/boox/src/base26.h>


using namespace boost::spirit;
using x3::_attr;
using x3::_val;
using x3::double_;
using x3::lexeme;
using x3::lit;
using x3::no_case;
using x3::omit;
using x3::raw;
using x3::uint32;
using x3::unicode::alnum;
using x3::unicode::alpha;
using x3::unicode::char_;
using x3::unicode::space;
using x3::with;


namespace lde::cellfy::boox::fx {
namespace _ {
namespace {


struct ci_wstring_view_hash final {
  std::size_t operator()(const std::wstring_view& sv) const noexcept {
    std::size_t seed = 0;
    for (wchar_t c : sv) {
      boost::hash_combine(seed, ed::to_lower_copy(c));
    }
    return seed;
  }
};


struct ci_wstring_view_equal final {
  bool operator()(const std::wstring_view& sv1, const std::wstring_view& sv2) const noexcept {
    return ed::iequals(sv1, sv2);
  }
};


using func_container = boost::multi_index_container<
  function_ptr,
  boost::multi_index::indexed_by<
    boost::multi_index::hashed_unique<
      boost::multi_index::key<
        &function::name
      >,
      ci_wstring_view_hash,
      ci_wstring_view_equal
    >
  >
>;


}} // namespace _


struct parser::impl {
  _::func_container funcs;
};


namespace _ {
namespace {


struct parser_impl_tag {};


template<typename ... Action>
struct each final {
  template<typename Context>
  void operator()(Context& ctx) const {
    (Action()(ctx), ...);
  }
};


struct append final {
  template<typename Context>
  void operator()(Context& ctx) const {
    if (_val(ctx).empty()) {
      _val(ctx) = std::move(_attr(ctx));
    } else {
      _val(ctx).insert(
        _val(ctx).end(),
        std::make_move_iterator(_attr(ctx).begin()),
        std::make_move_iterator(_attr(ctx).end()));
    }
  }
};


template<typename Token>
struct push_back final {
  template<typename Context>
  void operator()(Context& ctx) const {
    _val(ctx).push_back(Token{});
  }
};


struct parsed_column {
  column_index index = 0;
  bool         abs   = false;
};


struct parsed_row {
  row_index index = 0;
  bool      abs   = false;
};


struct parsed_cell {
  cell_addr addr;
  bool      col_abs;
  bool      row_abs;
};


const x3::rule<struct expr, ast::tokens>                 expr                = "expr";
const x3::rule<struct logical_expr, ast::tokens>         logical_expr        = "logical_expr";
const x3::rule<struct concat_expr, ast::tokens>          concat_expr         = "concat_expr";
const x3::rule<struct additive_expr, ast::tokens>        additive_expr       = "additive_expr";
const x3::rule<struct multiplicative_expr, ast::tokens>  multiplicative_expr = "multiplicative_expr";
const x3::rule<struct power_expr, ast::tokens>           power_expr          = "power_expr";
const x3::rule<struct percent_expr, ast::tokens>         percent_expr        = "percent_expr";
const x3::rule<struct unary_expr, ast::tokens>           unary_expr          = "unary_expr";
const x3::rule<struct range_expr, ast::tokens>           range_expr          = "range_expr";
const x3::rule<struct reference_expr, ast::tokens>       reference_expr      = "reference_expr";
const x3::rule<struct constant, ast::tokens>             constant            = "constant";
const x3::rule<struct number_literal, ast::tokens>       number_literal      = "number_literal";
const x3::rule<struct string_literal, ast::tokens>       string_literal      = "string_literal";
const x3::rule<struct boolean_literal, ast::tokens>      boolean_literal     = "boolean_literal";
const x3::rule<struct reference, ast::tokens>            reference           = "reference";
const x3::rule<struct cell, parsed_cell>                 cell                = "cell";
const x3::rule<struct column, parsed_column>             column              = "column";
const x3::rule<struct row, parsed_row>                   row                 = "row";
const x3::rule<struct sheet_name, std::wstring>          sheet_name          = "sheet_name";
const x3::rule<struct enclosed_sheet_name, std::wstring> enclosed_sheet_name = "enclosed_sheet_name";
const x3::rule<struct func_call, ast::tokens>            func_call           = "func_call";
const x3::rule<struct func_name, std::wstring>           func_name           = "func_name";


// expr ----------------------------------------------------------------------------------------------------------------
const auto expr_def = -lit(L"=") >> logical_expr;

BOOST_SPIRIT_DEFINE(expr);


// logical_expr --------------------------------------------------------------------------------------------------------
const auto logical_expr_def =
  concat_expr[append()] >>
  *(
    (L"<=" > concat_expr[each<append, push_back<ast::less_equal>>()])    |
    (L">=" > concat_expr[each<append, push_back<ast::greater_equal>>()]) |
    (L"<>" > concat_expr[each<append, push_back<ast::not_equal>>()])     |
    (L"<"  > concat_expr[each<append, push_back<ast::less>>()])          |
    (L">"  > concat_expr[each<append, push_back<ast::greater>>()])       |
    (L"="  > concat_expr[each<append, push_back<ast::equal>>()])
  );

BOOST_SPIRIT_DEFINE(logical_expr);


// concat_expr ---------------------------------------------------------------------------------------------------------
const auto concat_expr_def =
  additive_expr[append()] >>
  *(
    L"&" > additive_expr[each<append, push_back<ast::concat>>()]
  );

BOOST_SPIRIT_DEFINE(concat_expr);


// additive_expr -------------------------------------------------------------------------------------------------------
const auto additive_expr_def =
  multiplicative_expr[append()] >>
  *(
    (L"+" > multiplicative_expr[each<append, push_back<ast::add>>()]) |
    (L"-" > multiplicative_expr[each<append, push_back<ast::subtract>>()])
  );

BOOST_SPIRIT_DEFINE(additive_expr);


// multiplicative_expr -------------------------------------------------------------------------------------------------
const auto multiplicative_expr_def =
  power_expr[append()] >>
  *(
    (L"*" > power_expr[each<append, push_back<ast::multiply>>()]) |
    (L"/" > power_expr[each<append, push_back<ast::divide>>()])
  );

BOOST_SPIRIT_DEFINE(multiplicative_expr);


// power_expr ----------------------------------------------------------------------------------------------------------
const auto power_expr_def =
  percent_expr[append()] >>
  -(
    L"^" > percent_expr[each<append, push_back<ast::power>>()]
  );

BOOST_SPIRIT_DEFINE(power_expr);


// percent_expr --------------------------------------------------------------------------------------------------------
const auto percent_expr_def =
  unary_expr[append()] >>
  -(
    lit(L"%")[push_back<ast::percent>()]
  );

BOOST_SPIRIT_DEFINE(percent_expr);


// unary_expr ----------------------------------------------------------------------------------------------------------
const auto unary_expr_def =
  range_expr[append()] |
  (
    (L"+" > range_expr[each<append, push_back<ast::plus>>()]) |
    (L"-" > range_expr[each<append, push_back<ast::minus>>()])
  );

BOOST_SPIRIT_DEFINE(unary_expr);


// range_expr ----------------------------------------------------------------------------------------------------------
const auto range_expr_def =
  reference_expr[append()] >>
  *(
    (L":" >> reference_expr[each<append, push_back<ast::range>>()])
  );

BOOST_SPIRIT_DEFINE(range_expr);


// reference_expr ------------------------------------------------------------------------------------------------------
const auto reference_expr_def =
  func_call[append()] |
  constant[append()]  |
  reference[append()] |
  (L"(" > logical_expr[append()] > L")");

BOOST_SPIRIT_DEFINE(reference_expr);


// constant ------------------------------------------------------------------------------------------------------------
const auto constant_def =
  number_literal |
  string_literal |
  boolean_literal;

BOOST_SPIRIT_DEFINE(constant);


// number_literal ------------------------------------------------------------------------------------------------------
const auto number_literal_def = double_[([](auto& ctx) {
  _val(ctx).push_back(ast::number{_attr(ctx)});
})];

BOOST_SPIRIT_DEFINE(number_literal);


// string_literal-------------------------------------------------------------------------------------------------------
const auto string_literal_def = raw[lexeme[L'"' >> (*(char_ - L'"')) >> L'"'][([](auto& ctx) {
  _val(ctx).push_back(ast::string{std::wstring(_attr(ctx).begin(), _attr(ctx).end())});
})]];

BOOST_SPIRIT_DEFINE(string_literal);


// boolean_literal -----------------------------------------------------------------------------------------------------
const auto boolean_literal_def = no_case[x3::standard_wide::symbols<bool>({
  {L"true", true},
  {L"false", false}
})][([](auto& ctx) {
  _val(ctx).push_back(ast::boolean{_attr(ctx)});
})];

BOOST_SPIRIT_DEFINE(boolean_literal);


// reference -----------------------------------------------------------------------------------------------------------
// TODO: Доделать для целых колонок и строк (2:35, H:W)
// Добавлена поддержка списка ячеек через ':'
const auto reference_def =
  (sheet_name >> L'!' >> cell % ':')[([](auto& ctx) {
    auto& n = boost::fusion::at_c<0>(_attr(ctx));
    auto& c = boost::fusion::at_c<1>(_attr(ctx));
    for (auto beg = c.begin(); beg != c.end(); ++beg) {
      _val(ctx).push_back(ast::reference{beg->addr, beg->col_abs, beg->row_abs, n});
      if (beg != c.begin()) {
        _val(ctx).push_back(ast::range{});
      }
    }
  })] |
  (enclosed_sheet_name >> L'!' >> cell)[([](auto& ctx) {
    auto& n = boost::fusion::at_c<0>(_attr(ctx));
    auto& c = boost::fusion::at_c<1>(_attr(ctx));
    _val(ctx).push_back(ast::reference{c.addr, c.col_abs, c.row_abs, n});
  })] |
  cell[([](auto& ctx) {
    auto& c = _attr(ctx);
    _val(ctx).push_back(ast::reference{c.addr, c.col_abs, c.row_abs});
  })];

BOOST_SPIRIT_DEFINE(reference);


// cell ----------------------------------------------------------------------------------------------------------------
const auto cell_def = (column >> row)[([](auto& ctx) {
  auto& c = boost::fusion::at_c<0>(_attr(ctx));
  auto& r = boost::fusion::at_c<1>(_attr(ctx));
  _val(ctx).addr = cell_addr(c.index, r.index);
  _val(ctx).col_abs = c.abs;
  _val(ctx).row_abs = r.abs;
})];

BOOST_SPIRIT_DEFINE(cell);


// column --------------------------------------------------------------------------------------------------------------
const auto column_def =
  raw[+char_(L"a-zA-Z")][([](auto& ctx) {
    base26::decode(std::wstring(_attr(ctx).begin(), _attr(ctx).end()), _val(ctx).index);
    _val(ctx).abs = false;
  })] |
  (L"$" >> raw[+char_(L"a-zA-Z")])[([](auto& ctx) {
    base26::decode(std::wstring(_attr(ctx).begin(), _attr(ctx).end()), _val(ctx).index);
    _val(ctx).abs = true;
  })];

BOOST_SPIRIT_DEFINE(column);


// row -----------------------------------------------------------------------------------------------------------------
const auto row_def =
  uint32[([](auto& ctx) {
    _val(ctx).index = _attr(ctx) - 1;
    _val(ctx).abs = false;
  })] |
  (L"$" >> uint32)[([](auto& ctx) {
    _val(ctx).index = _attr(ctx) - 1;
    _val(ctx).abs = true;
  })];

BOOST_SPIRIT_DEFINE(row);


// sheet_name ----------------------------------------------------------------------------------------------------------
// TODO: временное решение. переделать
const auto sheet_name_def = +(char_
  - L"!" - L"'" - L"(" - L")" - L"<" - L">" - L"=" - L"&"
  - L"+" - L"-" - L"*" - L"/" - L"^" - L":" - L"%" - L";");

BOOST_SPIRIT_DEFINE(sheet_name);


// enclosed_sheet_name -------------------------------------------------------------------------------------------------
const auto enclosed_sheet_name_def = lexeme[L"'" >> sheet_name >> L"'"];

BOOST_SPIRIT_DEFINE(enclosed_sheet_name);


// func_call -----------------------------------------------------------------------------------------------------------
const auto func_call_def =(func_name >> L"(" >> -(logical_expr[append()] % (lit(L",") | lit(L";"))) >> L")")[([](auto& ctx) {
  auto&& impl = x3::get<parser_impl_tag>(ctx);
  ast::func item;

  auto i = impl->funcs.find(boost::fusion::at_c<0>(_attr(ctx)));
  if (i == impl->funcs.end()) {
    ED_THROW_EXCEPTION(invalid_function_name());
  }

  item.ptr = *i;

  if (auto& args = boost::fusion::at_c<1>(_attr(ctx))) {
    item.args_count = args->size();
  } else {
    item.args_count = 0;
  }
  _val(ctx).push_back(std::move(item));
})];

BOOST_SPIRIT_DEFINE(func_call);


// func_name -----------------------------------------------------------------------------------------------------------
const auto func_name_def = raw[lexeme[(alpha | '_') >> *(alnum | '_')]];

BOOST_SPIRIT_DEFINE(func_name);


}} // namespace _


parser::parser()
  : impl_(std::make_unique<impl>()) {
}


parser::~parser() = default;


void parser::parse(const std::wstring& formula, ast::tokens& ast) const {
  ast.clear();
  auto begin = formula.begin();

  bool ok = x3::phrase_parse(
    begin,
    formula.end(),
    with<_::parser_impl_tag>(impl_)[_::expr],
    space,
    ast);

  if (!ok || begin != formula.end()) {
    ED_THROW_EXCEPTION(parser_failed());
  }
}


bool parser::parse_no_throw(const std::wstring& formula, ast::tokens& ast) const {
  ast.clear();
  auto begin = formula.begin();

  try {
    bool ok = x3::phrase_parse(
      begin,
      formula.end(),
      with<_::parser_impl_tag>(impl_)[_::expr],
      space,
      ast);

    return ok && begin == formula.end();
  } catch (const std::exception&) {
    return false;
  }
}


void parser::add_function(function_ptr func) {
  ED_EXPECTS(func);
  impl_->funcs.insert(std::move(func));
}


} // namespace lde::cellfy::boox::fx
