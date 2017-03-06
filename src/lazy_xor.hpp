#ifndef UEP_LAZY_XOR_HPP
#define UEP_LAZY_XOR_HPP

#include <stdexcept>
#include <unordered_set>

namespace uep {

template <class T>
class lazy_xor {
public:
  lazy_xor() = default;
  explicit lazy_xor(const T *initial) {
    to_xor.insert(initial);
  }

  lazy_xor(const lazy_xor &) = default;
  lazy_xor(lazy_xor &&) = default;
  
  lazy_xor &operator=(const lazy_xor &) = default;
  lazy_xor &operator=(lazy_xor &&) = default;
  
  void swap(lazy_xor &other) {
    using std::swap;
    swap(to_xor, other.to_xor);
  }

  void xor_with(const lazy_xor &other) {
    for (auto i = other.to_xor.cbegin(); i != other.to_xor.cend(); ++i) {
      auto iret = to_xor.insert(*i);
      if (!iret.second) {
	to_xor.erase(iret.first);
      }
    }
  }
  
  T evaluate() const {
    if (empty()) throw std::runtime_error("Cannot evaluate empty xor-set");
    auto i = to_xor.cbegin();
    T e(*(*i++));
    for (; i != to_xor.cend(); ++i) {
      e ^= *(*i);
    }
    return e;
  }

  bool empty() const { return to_xor.empty(); }
  std::size_t size() const { return to_xor.size(); }

  explicit operator bool() const { return !empty(); }
  bool operator!() const { return empty(); }
  
private:
  std::unordered_set<const T*> to_xor;
};

template <class T>
lazy_xor<T> &operator^=(lazy_xor<T> &lhs, const lazy_xor<T> &rhs) {
  lhs.xor_with(rhs);
  return lhs;
}

template <class T>
lazy_xor<T> operator^(const lazy_xor<T> &lhs, const lazy_xor<T> &rhs) {
  lazy_xor<T> copy(lhs);
  return copy ^= rhs;
}

template <class T>
void swap(lazy_xor<T> &lhs, lazy_xor<T> &rhs) {
  lhs.swap(rhs);
}

}

#endif
