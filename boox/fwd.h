#pragma once


#include <cstdint>
#include <memory>


namespace lde::cellfy::boox {


using row_index    = std::uint32_t;
using column_index = std::uint32_t;
using cell_index   = std::uint64_t;

using node_version = std::uint8_t;

class area;
class cell_addr;
class cell_value;
class file_io;
class range;
class scoped_transaction;
class value_format;
class workbook;
class worksheet;

using file_io_ptr = std::unique_ptr<file_io>;


namespace fx {

class engine;
class operand;
class parser;

class function;
using function_ptr = std::shared_ptr<function>;

} // namespace fx
} // namespace lde::cellfy::boox
