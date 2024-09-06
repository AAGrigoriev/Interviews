#pragma once


#include <vector>
#include <type_traits>

#include <boost/operators.hpp>
#include <boost/range/iterator_range.hpp>


namespace lde::cellfy::boox {


template<typename T>
class vector_2d final :
  public boost::equality_comparable<vector_2d<T>>,
  public boost::less_than_comparable<vector_2d<T>> {

  static_assert(std::is_default_constructible_v<T>, "T must be a default constructible");

private:
  using container = std::vector<T>;

public:
  using iterator       = typename container::iterator;
  using const_iterator = typename container::const_iterator;
  using row_ref        = boost::iterator_range<iterator>;
  using const_row_ref  = boost::iterator_range<const_iterator>;

public:
  vector_2d() = default;

  vector_2d(std::size_t n_rows, std::size_t n_columns) {
    resize(n_rows, n_columns);
  }

  bool operator==(const vector_2d& rhs) const noexcept {
    return data_ == rhs.data_;
  }

  bool operator<(const vector_2d& rhs) const noexcept {
    return data_ < rhs.data_;
  }

  iterator begin() noexcept {
    return data_.begin();
  }

  iterator end() noexcept {
    return data_.end();
  }

  const_iterator begin() const noexcept {
    return data_.begin();
  }

  const_iterator end() const noexcept {
    return data_.end();
  }

  const_iterator cbegin() const noexcept {
    return data_.cbegin();
  }

  const_iterator cend() const noexcept {
    return data_.cend();
  }

  iterator rbegin() noexcept {
    return data_.rbegin();
  }

  iterator rend() noexcept {
    return data_.rend();
  }

  const_iterator rbegin() const noexcept {
    return data_.rbegin();
  }

  const_iterator rend() const noexcept {
    return data_.rend();
  }

  const_iterator crbegin() const noexcept {
    return data_.crbegin();
  }

  const_iterator crend() const noexcept {
    return data_.crend();
  }

  std::size_t rows_count() const noexcept {
    return n_rows_;
  }

  std::size_t columns_count() const noexcept {
    return n_columns_;
  }

  void resize(std::size_t n_rows, std::size_t n_columns) {
    n_rows_ = n_rows;
    n_columns_ = n_columns;
    data_.resize(n_rows * n_columns);
  }

  void clear() {
    n_rows_ = 0;
    n_columns_ = 0;
    data_.clear();
  }

  bool contains(std::size_t row, std::size_t column) noexcept {
    return row < n_rows_ && column < n_columns_;
  }

  const T& at(std::size_t row, std::size_t column) const {
    return data_.at(row * n_columns_ + column);
  }

  T& at(std::size_t row, std::size_t column) {
    return data_.at(row * n_columns_ + column);
  }

  const_row_ref operator[](std::size_t row) const {
    return {
      data_.begin() + row * n_columns_,
      data_.begin() + row * n_columns_ + n_columns_,
    };
  }

  row_ref operator[](std::size_t row) {
    return {
      data_.begin() + row * n_columns_,
      data_.begin() + row * n_columns_ + n_columns_,
    };
  }

private:
  container   data_;
  std::size_t n_rows_    = 0;
  std::size_t n_columns_ = 0;
};


} // namespace lde::cellfy::boox
