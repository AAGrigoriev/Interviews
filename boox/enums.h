#pragma once


namespace lde::cellfy::boox {


enum class cell_value_error : unsigned char {
  div0  = 0, // #DIV/0!
  na    = 1, // #N/A
  name  = 2, // #NAME?
  null  = 3, // #NULL!
  num   = 4, // #NUM!
  ref   = 5, // #REF!
  value = 6, // #VALUE!
};


/// Тип значения ячейки
enum class cell_value_type : unsigned char {
  none      = 0,
  boolean   = 1, // cell_node -> cell_data_node
  number    = 2, // cell_node -> cell_data_node
  string    = 3, // cell_node -> cell_data_node
  rich_text = 4, // cell_node -> text_run_node [1, ...]
  error     = 5  // cell_node -> cell_data_node
};


enum class pattern_type : unsigned char {
  solid   = 0,
  gray125 = 1,
  none    = 2,
  // ...
};


enum class border_style : unsigned char {
  none   = 0,
  thin   = 1,
  medium = 2,
  thick  = 3,
  // ...
};


enum class horizontal_alignment : unsigned char {
  left    = 0,
  center  = 1,
  right   = 2,
  general = 3
};


enum class vertical_alignment : unsigned char {
  top    = 0,
  center = 1,
  bottom = 2
};


enum class script {
  normal      = 0,
  superscript = 1,
  subscript   = 2
};


enum class calc_mode {
  automatic,     ///< Автоматический расчёт. Режим по умолчанию.
  manual         ///< Ручной режим.
};

} // namespace lde::cellfy::boox
