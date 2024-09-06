#include <lde/cellfy/boox/scoped_transaction.h>
#include <lde/cellfy/boox/workbook.h>


namespace lde::cellfy::boox {


scoped_transaction::scoped_transaction(forest_t& f)
  : f_(f)
  , nested_(f.in_transaction()) {

  if (!nested_) {
    f.begin_transaction();
  }
}


scoped_transaction::scoped_transaction(workbook& book)
  : scoped_transaction(book.forest()) {
}


scoped_transaction::~scoped_transaction() {
  if (!nested_ && f_.in_transaction()) {
    f_.rollback();
  }
}


bool scoped_transaction::nested() const noexcept {
  return nested_;
}


void scoped_transaction::commit() {
  if (!nested_) {
    f_.commit();
  }
}


void scoped_transaction::rollback() {
  if (!nested_) {
    f_.rollback();
  }
}


} // namespace lde::cellfy::boox
