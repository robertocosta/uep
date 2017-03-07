#ifndef UEP_FLATTEN_ITERATOR_HPP
#define UEP_FLATTEN_ITERATOR_HPP

#include <boost/iterator/iterator_adaptor.hpp>

namespace uep {

template <class BaseIter>
struct fl_iter_types {
  typedef typename std::iterator_traits<BaseIter> base_iter_traits;
  typedef typename base_iter_traits::value_type inner_container;
  typedef typename base_iter_traits::iterator_category outer_category;

  typedef typename base_iter_traits::reference inner_container_ref;
  typedef typename std::remove_reference<inner_container_ref>::type
  inner_container_cv;

  typedef decltype(inner_container_cv().begin()) inner_iterator;

  typedef typename std::iterator_traits<inner_iterator> inner_iter_traits;
  typedef typename inner_iter_traits::value_type value_type;
  typedef typename inner_iter_traits::reference reference;
  typedef typename inner_iter_traits::iterator_category inner_category;
};

template <class BaseIter>
class flatten_iterator : public boost::iterator_adaptor<
  flatten_iterator<BaseIter>,
  BaseIter,
  typename fl_iter_types<BaseIter>::value_type,
  std::forward_iterator_tag,
  typename fl_iter_types<BaseIter>::reference
  > {
  friend boost::iterator_core_access;

  static_assert(std::is_convertible<
		typename fl_iter_types<BaseIter>::outer_category,
		std::forward_iterator_tag>::value,
		"BaseIter is not a forward iterator");
  static_assert(std::is_convertible<
		typename fl_iter_types<BaseIter>::inner_category,
		std::forward_iterator_tag>::value,
		"BaseIter::value_type is not a forward iterator");

  typedef typename fl_iter_types<BaseIter>::inner_iterator inner_iter_t;
  typedef typename fl_iter_types<BaseIter>::reference reference_t;

public:
  flatten_iterator() :
    flatten_iterator::iterator_adaptor_(),
    outer_end(), inner_current(), inner_end() {}
  explicit flatten_iterator(BaseIter b, BaseIter end) :
    flatten_iterator::iterator_adaptor_(b),
    outer_end(end), inner_current(), inner_end() {
    BaseIter &outer_current = this->base_reference();
    if (outer_current != outer_end) {
      auto &c = *outer_current;
      inner_current = c.begin();
      inner_end = c.end();
      skip_empty();
    }
  }

  reference_t dereference() const {
    return *inner_current;
  }

  bool equal(const flatten_iterator &other) const {
    return this->base() == other.base() &&
      (inner_current == other.inner_current ||
       (inner_current == inner_end && other.inner_current == other.inner_end));
  }

  void increment() {
    BaseIter &outer_current = this->base_reference();
    if (inner_current != inner_end) {
      ++inner_current;
      skip_empty();
    }
    else {
      ++outer_current;
      if (outer_current != outer_end) {
	auto &c = *outer_current;
	inner_current = c.begin();
	inner_end = c.end();
	skip_empty();
      }
    }
  }

private:
  BaseIter outer_end;
  inner_iter_t inner_current, inner_end;

  void skip_empty() {
    BaseIter &outer_current = this->base_reference();
    while (inner_current == inner_end &&
	   outer_current != outer_end) {
      ++outer_current;
      auto &c = *outer_current;
      inner_current = c.begin();
      inner_end = c.end();
    }
  }
};

template <class BaseIter>
std::pair<flatten_iterator<BaseIter>,flatten_iterator<BaseIter>>
  flatten_range(BaseIter begin, BaseIter end) {
  return make_pair(flatten_iterator<BaseIter>(begin,end),
		   flatten_iterator<BaseIter>(end,end)
		   );
}

}

#endif
