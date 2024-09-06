set(name lde-cellfy-boox-ut)

add_executable(${name}
  area.cpp
  base26.cpp
  cell_addr.cpp
  criteria_parser.cpp
  fx.cpp
  main.cpp
  range.cpp
  value_format.cpp
  vector_2d.cpp
)

if(ED_USE_PCH)
  target_precompile_headers(${name}
    PRIVATE
    ${COMMON_STL_PCH}
    ${COMMON_BOOST_PCH}
  )
endif()

target_link_libraries(${name}
  gtest
  lde-cellfy-boox
  lde-cellfy-fun
)

lde_gtest(${name})
