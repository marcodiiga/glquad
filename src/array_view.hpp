#ifndef HEADER_ARRAYVIEW_HPP
#define HEADER_ARRAYVIEW_HPP

#include <vector>

namespace arv {

  template<typename T>
  class array_view {
  public:
    array_view(std::vector<T>& v)
      : data_(v.data()), first_element_(0), size_(v.size())
    {}

    array_view(std::vector<T>& v, size_t first_element, size_t size = 1)
      : array_view(v) {
      first_element_ = first_element;
      size_ = size;
    }

    // Todo other overloads

    array_view& operator=(T&& element) {
      if (size_ < 1)
        throw std::out_of_range("Too many elements specified");

      data_[first_element_] = element;
      return *this;
    }

    array_view& operator=(std::initializer_list<T>&& list) {
      if (size_ != -1 && list.size() > size_)
        throw std::out_of_range("Too many elements specified");

      size_t index = (first_element_ != -1) ? first_element_ : 0;
      for (auto&& el : list) {
        data_[index] = el;
        ++index;
      }
      return *this;
    }

  private:
    T* data_;
    size_t first_element_ = -1;
    size_t size_ = -1;
  };
}


#endif // HEADER_ARRAYVIEW_HPP