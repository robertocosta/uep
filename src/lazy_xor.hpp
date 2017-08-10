#ifndef UEP_LAZY_XOR_HPP
#define UEP_LAZY_XOR_HPP

#include <forward_list>
#include <memory>
#include <stdexcept>
#include <unordered_set>

namespace uep {

/** Class used to delay the application of XORs on some other type.
 *  This class accumulates pointers to XOR-able objects and actually
 *  performs the XOR between them either upon a call to evaluate() or
 *  when the pointer list exceeds a maximum size.
*/
template <class T, std::size_t MAX_SIZE = 1>
class lazy_xor {
public:
  /* Build an empty lazy_xor. */
  lazy_xor() : size_(0) {}
  /* Build a lazy_xor using the intitial value pointed to by initial.
   * The pointer must remain valid during the lifetime of the lazy_xor
   * object.
   */
  explicit lazy_xor(const T *initial) {
    to_xor.push_front(initial);
    size_ = 1;
  }
  /** Construct a lazy_xor using a copy of the initial value. */
  explicit lazy_xor(const T &initial) {
    shared_to_xor.push_front(std::make_shared<const T>(initial));
    size_ = 1;
  }
  /** Construct a lazy_xor by moving from the initial value. */
  explicit lazy_xor(T &&initial) {
    shared_to_xor.push_front(std::make_shared<const T>(std::move(initial)));
    size_ = 1;
  }

  /** Swap this with the other lazy_xor. */
  void swap(lazy_xor &other) {
    using std::swap;
    swap(to_xor, other.to_xor);
    swap(shared_to_xor, other.shared_to_xor);
    swap(size_, other.size_);
  }

  /** Cumulative XOR with another object.  This only merges the set of
   *  pointers to the objects that will be XORed at eval time. If the
   *  list of pointers exceeds MAX_SIZE the evaluation will take place
   *  immediately.
   */
  void xor_with(const lazy_xor &other) {
    for (auto i = other.to_xor.cbegin(); i != other.to_xor.cend(); ++i) {
      to_xor.push_front(*i);
    }
    for (auto i = other.shared_to_xor.cbegin();
	 i != other.shared_to_xor.cend(); ++i) {
      shared_to_xor.push_front(*i);
    }
    size_ += other.size_;

    if (size_ > MAX_SIZE) {
      T eval_result = evaluate();
      to_xor.clear();
      shared_to_xor.clear();
      shared_to_xor.push_front(std::make_shared<const T>(std::move(eval_result)));
      size_ = 1;
    }
  }

  /** Cumulative XOR with another object.
   *  The other pointer must remain valid for the entire lifetime of
   *  the lazy_xor object.
   */
  void xor_with(const T *other) {
    xor_with(lazy_xor(other));
  }

  /** Perform the XOR between the underlying objects and return the
   *  result.  This method throws a runtime_error when called on empty
   *  objects
   */
  T evaluate() const {
    if (empty()) throw std::runtime_error("Cannot evaluate an empty lazy_xor");
    auto i = to_xor.cbegin();
    auto j = shared_to_xor.cbegin();

    T e;
    if (i != to_xor.cend()) e = *(*i++);
    else e = *(*j++);

    for (; i != to_xor.cend(); ++i) {
      e ^= *(*i);
    }
    for (; j != shared_to_xor.cend(); ++j) {
      e ^= *(*j);
    }

    return e;
  }

  /** Return true when the set of objects to XOR is empty. */
  bool empty() const { return size_ == 0; }
  /** Return the size of the set of objects to XOR. */
  std::size_t size() const { return size_; }

  /** Return the same as !empty(). */
  explicit operator bool() const { return !empty(); }
  /** Return the same as empty(). */
  bool operator!() const { return empty(); }

  //friend bool operator==(const lazy_xor<T> &lhs, const lazy_xor<T> &rhs);

private:
  /** Pointers to the objects to xor. */
  std::forward_list<const T*> to_xor;
  /** Shared pointers to the objects to xor. This is used because
   *  `to_xor` is not suitable to hold the automatic evaluation
   *  results.
   */
  std::forward_list<std::shared_ptr<const T>> shared_to_xor;
  std::size_t size_; /**< Combined size of the two lists. The
		      *   forward_list does not keep the size.
		      */
};

/** XOR-equal operator between lazy_xors. \sa lazy_xor::xor_with */
template <class T, std::size_t MAX_SIZE>
lazy_xor<T,MAX_SIZE> &operator^=(lazy_xor<T,MAX_SIZE> &lhs,
				 const lazy_xor<T,MAX_SIZE> &rhs) {
  lhs.xor_with(rhs);
  return lhs;
}

/** XOR operator between lazy_xors. \sa lazy_xor::xor_with */
template <class T, std::size_t MAX_SIZE>
lazy_xor<T,MAX_SIZE> operator^(const lazy_xor<T,MAX_SIZE> &lhs,
			       const lazy_xor<T,MAX_SIZE> &rhs) {
  lazy_xor<T,MAX_SIZE> copy(lhs);
  return copy ^= rhs;
}

/** Swap two lazy_xor objects. */
template <class T, std::size_t MAX_SIZE>
void swap(lazy_xor<T,MAX_SIZE> &lhs, lazy_xor<T,MAX_SIZE> &rhs) {
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
