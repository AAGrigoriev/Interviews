#pragma once


#include <lde/cellfy/boox/fwd.h>
#include <lde/cellfy/boox/forest.h>


namespace lde::cellfy::boox {


class scoped_transaction final {
public:
  explicit scoped_transaction(forest_t& f);
  explicit scoped_transaction(workbook& book);
  ~scoped_transaction();

  scoped_transaction(const scoped_transaction&) = delete;
  scoped_transaction& operator=(const scoped_transaction&) = delete;

  bool nested() const noexcept;

  void commit();
  void rollback();

private:
  forest_t& f_;
  bool      nested_ = false;
};


} // namespace lde::cellfy::boox
