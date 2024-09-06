set(name lde-cellfy-boox)

add_definitions(-DBOOST_SPIRIT_X3_UNICODE)

add_library(${name}
  area.h
  cell_addr.h
  cell_value.h
  enums.h
  exception.h
  forest.h
  format.h
  fwd.h
  fx_ast.h
  fx_engine.h
  fx_function.h
  fx_operand.h
  fx_parser.h
  node.h
  range.h
  range_op.h
  scoped_transaction.h
  value_format.h
  vector_2d.h
  workbook.h
  worksheet.h
  src/area.cpp
  src/base26.h
  src/cell_addr.cpp
  src/cell_op.cpp
  src/cell_op.h
  src/cell_value.cpp
  src/column_op.cpp
  src/column_op.h
  src/fx_engine.cpp
  src/fx_parser.cpp
  src/range.cpp
  src/range_op.cpp
  src/row_op.cpp
  src/row_op.h
  src/scoped_transaction.cpp
  src/value_format.cpp
  src/value_parser.cpp
  src/value_parser.h
  src/workbook.cpp
  src/worksheet.cpp
)

if(ED_USE_PCH)
  target_precompile_headers(${name}
    PRIVATE
    ${COMMON_STL_PCH}
    ${COMMON_BOOST_PCH}
    "<boost/spirit/home/x3.hpp>"
  )
endif()

target_link_libraries(${name}
  ed-core
  ed-rasta
  lde-cellfy-forest
)

if(LDE_BUILD_UNIT_TESTS)
  add_subdirectory(ut)
endif()
