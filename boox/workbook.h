#pragma once


#include <functional>
#include <iosfwd>
#include <locale>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/range/any_range.hpp>

#include <ed/core/fwd.h>
#include <ed/core/mime.h>
#include <ed/core/property.h>

#include <lde/cellfy/boox/forest.h>
#include <lde/cellfy/boox/format.h>
#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/fx_parser.h>
#include <lde/cellfy/boox/node.h>
#include <lde/cellfy/boox/worksheet.h>


namespace lde::cellfy::boox {


/// Книга
class workbook final {
  friend class change_existing_cell_format_op;
  friend class change_cell_format_op;
  friend class change_existing_column_format_op;
  friend class change_column_format_op;
  friend class change_existing_row_format_op;
  friend class change_row_format_op;
  friend class range;
  friend class worksheet;

public:
  using sheets_range = boost::any_range<
    worksheet,
    boost::random_access_traversal_tag
  >;

  using sheets_crange = boost::any_range<
    const worksheet,
    boost::random_access_traversal_tag,
    const worksheet&
  >;

  using mimes_range = boost::any_range<
    ed::mime_type,
    boost::forward_traversal_tag
  >;

  using file_reader      = std::function<void(std::istream&, forest_t&)>;
  using file_writer      = std::function<void(std::ostream&, const forest_t&)>;
  using clipboard_reader = std::function<void(std::istream&, range&)>;
  using clipboard_writer = std::function<void(std::ostream&, const range&)>;

public:
  ed::ro_proxy_property<bool>        can_undo;
  ed::ro_proxy_property<bool>        can_redo;
  ed::ro_proxy_property<range>       active_selection;
  ed::ro_proxy_property<range>       active_cell;
  ed::ro_proxy_property<std::size_t> sheets_count;
  ed::ro_proxy_property<worksheet*>  active_sheet;
  ed::signal<worksheet&>             sheet_inserted;
  ed::signal<worksheet&>             sheet_removed;
  ed::signal<worksheet&>             sheet_shifted;
  ed::signal<forest::changes_cause>  changes_started;
  ed::signal<forest::changes_cause>  changes_finished;
  ed::signal<const range&>           changed;
  ed::signal<>                       about_to_close;
  ed::signal<>                       opened;

public:
  workbook();

  workbook(const workbook&) = delete;
  workbook& operator=(const workbook&) = delete;

  workbook_node::it node() const noexcept;

  /// Лес.
  const forest_t& forest() const noexcept;
  forest_t& forest() noexcept;

  /// Парсер формул.
  const fx::parser& formula_parser() const noexcept;
  fx::parser& formula_parser() noexcept;

  /// Листы.
  sheets_crange sheets() const noexcept;
  sheets_range sheets() noexcept;

  /// Получение листа по имени
  const worksheet* sheet_by_name(std::wstring_view name) const noexcept;
  worksheet* sheet_by_name(std::wstring_view name) noexcept;

  /// Получение листа по индексу
  const worksheet* sheet_by_index(std::size_t index) const noexcept;
  worksheet* sheet_by_index(std::size_t index) noexcept;

  /// Добавить новый лист
  worksheet& emplace_sheet(std::size_t index);

  /// Отменить действие.
  void undo();

  /// Повторить действие.
  void redo();

  /// Проверить в возможность чтения/записи файла определенного формата.
  bool can_read_file_format(const ed::mime_type& format) const noexcept;
  bool can_write_file_format(const ed::mime_type& format) const noexcept;

  mimes_range readable_file_formats() const noexcept;
  mimes_range writable_file_formats() const noexcept;

  /// Добавить поддержку определенного формата файла.
  void add_file_reader(const ed::mime_type& format, file_reader&& fn);
  void add_file_writer(const ed::mime_type& format, file_writer&& fn);

  /// Проверить в возможность чтения/записи буфера обмена определенного формата.
  bool can_read_clipboard_format(const ed::mime_type& format) const noexcept;
  bool can_write_clipboard_format(const ed::mime_type& format) const noexcept;

  mimes_range readable_clipboard_formats() const noexcept;
  mimes_range writable_clipboard_formats() const noexcept;

  /// Добавить поддержку чтения/записи буфера обмена определенного формата.
  void add_clipboard_reader(const ed::mime_type& format, clipboard_reader&& fn);
  void add_clipboard_writer(const ed::mime_type& format, clipboard_writer&& fn);

  /// Установить выбранную локаль.
  void set_locale(const std::locale& loc);
  const std::locale& get_locale() const noexcept;

  /// Новая пустая книга с одним листом.
  void open();

  /// Открыть книгу из потока
  void open(const ed::mime_type& format, std::istream& is);

  /// Сохранить книгу в поток
  void save(const ed::mime_type& format, std::ostream& os) const;

  /// Задать режим расчёта для книги.
  void set_calc_mode(calc_mode mode);
  /// Получить текущий режим расчёта.
  calc_mode get_calc_mode() const noexcept;

  /// Рассчитать формулы на всех листах. Не заносится в undo/redo.
  void сalculate_formulas_on_all_sheets();
  /// Рассчитать формулы на текущем листе. Не заносится в undo/redo.
  void calculate_formulas_on_active_sheet();

protected:
  cell_format_node::it ensure_format(cell_format&& fmt);

  void pre_open(bool block_signals);
  void post_open(bool unblock_signals);

private:
  using any_connections   = std::vector<ed::scoped_any_connection>;
  using file_readers      = std::unordered_map<ed::mime_type, file_reader>;
  using file_writers      = std::unordered_map<ed::mime_type, file_writer>;
  using clipboard_readers = std::unordered_map<ed::mime_type, clipboard_reader>;
  using clipboard_writers = std::unordered_map<ed::mime_type, clipboard_writer>;

  struct by_name {};
  struct by_format {};
  struct by_key {};

  struct get_sheet_name {
    using result_type = std::wstring_view;
    std::wstring_view operator()(const worksheet& sheet) const noexcept {
      return *sheet.name;
    }
  };

  using sheets_container = boost::multi_index_container<
    worksheet,
    boost::multi_index::indexed_by<
      boost::multi_index::random_access<>,
      boost::multi_index::hashed_unique<
        boost::multi_index::tag<by_name>,
        get_sheet_name
      >
    >
  >;

  struct get_format {
    using result_type = cell_format;
    const result_type& operator()(cell_format_node::it node) const noexcept {
      ED_ASSERT(node->format);
      return *node->format;
    }
  };

  struct get_key {
    using result_type = node_key_type;
    result_type operator()(cell_format_node::it node) const noexcept {
      return forest_t::key_of(node);
    }
  };

  using cell_formats_container = boost::multi_index_container<
    cell_format_node::it,
    boost::multi_index::indexed_by<
      boost::multi_index::hashed_unique<
        boost::multi_index::tag<by_format>,
        get_format
      >,
      boost::multi_index::hashed_unique<
        boost::multi_index::tag<by_key>,
        get_key
      >
    >
  >;

  ed::property<bool>        can_undo_           = {false};
  ed::property<bool>        can_redo_           = {false};
  ed::property<std::size_t> sheets_count_       = 0;
  ed::property<worksheet*>  active_sheet_       = {nullptr};
  forest_t                  forest_;
  any_connections           forest_conns_;
  workbook_node::it         book_node_;
  fx::parser                formula_parser_;
  file_readers              file_readers_;
  file_writers              file_writers_;
  clipboard_readers         clipboard_readers_;
  clipboard_writers         clipboard_writers_;
  sheets_container          sheets_;
  cell_formats_container    cell_formats_;
  bool                      formats_gc_at_work_ = false;
  std::locale               locale_             = {};
  calc_mode                 calc_mode_          = calc_mode::automatic;
};


} // namespace lde::cellfy::boox
