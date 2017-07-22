#ifndef UEP_LAZY_XOR_HPP
#define UEP_LAZY_XOR_HPP

#include <stdexcept>
#include <unordered_set>

namespace uep {

/** Class used to delay the application of XORs on some other type.
 *  This class accumulates pointers to XOR-able objects, eliding when
 *  possible, and actually performs the XOR between the accumulated
 *  set of objects with a call to evaluate().
*/
template <class T>
class lazy_xor {
public:
  /* Build an empty lazy_xor. */
  lazy_xor() = default;
  /* Build a lazy_xor using the intitial value pointed to by initial.
   * The pointer must remain valid during the lifetime of the lazy_xor
   * object.
   */
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

  /** Cumulative XOR with another object.
   *  This only merges the set of pointers to the objects that will be
   *  XORed at eval time. It also cancels out identical pointers.
   */
  void xor_with(const lazy_xor &other) {
    for (auto i = other.to_xor.cbegin(); i != other.to_xor.cend(); ++i) {
      auto iret = to_xor.insert(*i);
      if (!iret.second) {
	to_xor.erase(iret.first);
      }
    }
  }

  /** Cumulative XOR with another object.
   *  The other pointer must remain valid for the entire lifetime of
   *  the lazy_xor object.
   */
  void xor_with(const T *other) {
    xor_with(lazy_xor<T>(other));
  }

  /** Perform the XOR between the underlying objects and return the
   *  result.  This method throws a runtime_error when called on empty
   *  objects
   */
  T evaluate() const {
    if (empty()) throw std::runtime_error("Cannot evaluate an empty lazy_xor");
    auto i = to_xor.cbegin();
    T e(*(*i++));
    for (; i != to_xor.cend(); ++i) {
      e ^= *(*i);
    }
    return e;
  }

  /** Return true when the set of objects to XOR is empty. */
  bool empty() const { return to_xor.empty(); }
  /** Return the size of the set of objects to XOR. */
  std::size_t size() const { return to_xor.size(); }

  /** Return the same as !empty(). */
  explicit operator bool() const { return !empty(); }
  /** Return the same as empty(). */
  bool operator!() const { return empty(); }

  //friend bool operator==(const lazy_xor<T> &lhs, const lazy_xor<T> &rhs);

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

/** Equality comparison between lazy_xors. To be equal they must store
 *  the same set of pointers.
 */
// template <class T>
// bool operator==(const lazy_xor<T> &lhs, const lazy_xor<T> &rhs) {
//   return lhs.to_xor == rhs.to_xor;
// }

// template <class T>
// bool operator!=(const lazy_xor<T> &lhs, const lazy_xor<T> &rhs) {
//   return !(lhs == rhs);
// }

}

#endif
