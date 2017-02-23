#ifndef UEP_SKIP_ITERATOR_HPP
#define UEP_SKIP_ITERATOR_HPP

namespace uep {

/** Forward iterator adapter that skips elements.
 *  SkipPredicate must be a callable taking a const
 *  Iterator::value_type& and returning a bool. PastTheEndPredicate
 *  must be a callable taking a const Iterato& and returning a bool.
 *  When this iterator is advanced it skips all elements matching
 *  SkipPredicate, until it reaches an Iterator that matches
 *  PastTheEndPredicate.
 */
template <class Iterator, class SkipPredicate, class PastTheEndPredicate>
class skip_iterator {
public:
  typedef Iterator iterator_type;
  typedef SkipPredicate skip_predicate_type;
  typedef PastTheEndPredicate past_the_end_predicate_type;
  typedef typename std::iterator_traits<Iterator>::difference_type difference_type;
  typedef typename std::iterator_traits<Iterator>::value_type value_type;
  typedef typename std::iterator_traits<Iterator>::pointer pointer;
  typedef typename std::iterator_traits<Iterator>::reference reference;
  typedef std::forward_iterator_tag iterator_category;

  static_assert(std::is_convertible<
		typename std::iterator_traits<Iterator>::iterator_category,
		iterator_category>::value,
		"Iterator is not a forward iterator");

  explicit skip_iterator(Iterator base,
			 const SkipPredicate &skip_p,
			 const PastTheEndPredicate &end_p) :
    current(base), skip(skip_p), end(end_p) {
    while (!is_past_end() && is_skip())
      ++current;
  }
  skip_iterator() = default;
  skip_iterator(const skip_iterator &other) = default;
  //skip_iterator(skip_iterator &&other) = default;

  skip_iterator &operator=(const skip_iterator &other) = default;
  //skip_iterator &operator=(skip_iterator &&other) = default;

  ~skip_iterator() = default;

  Iterator base() const { return current; }
  SkipPredicate skip_predicate() const { return skip; }
  PastTheEndPredicate past_the_end_predicate() const { return end; }

  void swap(skip_iterator &other) {
    using std::swap;
    swap(current, other.current);
    swap(skip, other.skip);
    swap(end, other.end);
  }

  reference operator*() const {
    if (is_skip())
      throw std::runtime_error("The current element should be skipped");
    return *current;
  }

  pointer operator->() const {
    return current;
  }

  skip_iterator &operator++() {
    ++current;
    while (!is_past_end() && is_skip())
      ++current;
    return *this;
  }

  skip_iterator operator++(int) {
    skip_iterator i(*this);
    ++(*this);
    return i;
  }

  bool operator==(const skip_iterator &other) {
    // Don't compare the predicates? Same type -> equal
    return current == other.current;
  }

  bool operator!=(const skip_iterator &other) {
    return !(*this == other);
  }

private:
  Iterator current;
  SkipPredicate skip;
  PastTheEndPredicate end;

  bool is_past_end() const {
    return end(current);
  }

  bool is_skip() const {
    return skip(*current);
  }
};

template <class Iterator, class SkipPredicate, class PastTheEndPredicate>
void swap(skip_iterator<Iterator,SkipPredicate,PastTheEndPredicate> &lhs,
	  skip_iterator<Iterator,SkipPredicate,PastTheEndPredicate> &rhs) {
  lhs.swap(rhs);
}

template <class T>
struct equal_to_predicate {
  explicit equal_to_predicate(T to_) : to(to_) {}

  bool operator()(T i) const {
    return i == to;
  }

private:
  T to;
};

template <class T>
struct is_false_predicate {
  bool operator()(const T &t) const {
    return !static_cast<bool>(t);
  }
};

/** Function to construct a skip_iterator. */
template <class Iterator, class SkipPredicate, class PastTheEndPredicate>
skip_iterator<Iterator, SkipPredicate, PastTheEndPredicate>
make_skip_iterator(Iterator i, SkipPredicate sp, PastTheEndPredicate pep) {
  return skip_iterator<Iterator, SkipPredicate, PastTheEndPredicate>(i, sp, pep);
}

/** Function to construct a skip_iterator that stops when the base
 *  iterator is equal to end.
 */
template <class Iterator, class SkipPredicate>
skip_iterator< Iterator, SkipPredicate, equal_to_predicate<Iterator> >
make_skip_iterator(Iterator i, SkipPredicate sp, Iterator end) {
  return make_skip_iterator(i, sp, equal_to_predicate<Iterator>(end));
}

/** Function to construct a skip_iterator that skips elements which
 *  convert to false and stops when the base iterator compares equal
 *  to end.
 */
template <class Iterator>
skip_iterator<Iterator,
	      is_false_predicate<typename std::iterator_traits<Iterator>::value_type>,
	      equal_to_predicate<Iterator>>
  make_skip_iterator(Iterator i, Iterator end) {
  return make_skip_iterator(i, is_false_predicate<typename std::iterator_traits<Iterator>::value_type>(), end);
}

}

#endif
