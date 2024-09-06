#pragma once


#include <ed/core/exception.h>

#include <lde/cellfy/boox/enums.h>


namespace lde::cellfy::boox {


class unsupported_file_format : public ed::runtime_error {
public:
  unsupported_file_format() : runtime_error("unsupported file format") {}
};


class unsupported_clipboard_format : public ed::runtime_error {
public:
  unsupported_clipboard_format() : runtime_error("unsupported clipboard format") {}
};


class file_is_broken : public ed::runtime_error {
public:
  file_is_broken() : runtime_error("file is broken") {}
};


class range_is_empty : public ed::runtime_error {
public:
  range_is_empty() : runtime_error("range is empty") {}
};


class too_many_areas_in_range : public ed::runtime_error {
public:
  too_many_areas_in_range() : runtime_error("too many areas in range") {}
};


class too_many_cells_in_range : public ed::runtime_error {
public:
  too_many_cells_in_range() : runtime_error("too many cells in range") {}
};


class does_not_contains_range : public ed::runtime_error {
public:
  does_not_contains_range() : runtime_error("does not contains a range") {}
};


class bad_value_cast : public ed::runtime_error {
public:
  bad_value_cast() : runtime_error("bad value cast") {}
};


class parser_failed : public ed::runtime_error {
public:
  parser_failed() : runtime_error("formula parser failed") {}
};


class invalid_function_name : public ed::runtime_error {
public:
  invalid_function_name() : runtime_error("formula parser failed") {}
};


class invalid_worksheet_name : public ed::runtime_error {
public:
  invalid_worksheet_name() : runtime_error("invalid worksheet name") {}
};


class division_by_zero : public ed::runtime_error {
public:
  division_by_zero() : runtime_error("division by zero") {}
};


using cell_value_error_info = boost::error_info<
  struct cell_value_error_info_tag,
  cell_value_error
>;


} // namespace lde::cellfy::boox
