#ifndef UEP_UTILS_HPP
#define UEP_UTILS_HPP

#include <algorithm>
#include <iostream>
#include <iterator>

#include <boost/iterator/filter_iterator.hpp>

namespace uep {
namespace utils {

/** Predicate that just converts the input to bool. */
template <class T>
struct to_bool {
  bool operator()(const T &x) const {
    return static_cast<bool>(x);
  }
};

/** Iterator adaptor that skips over the values that convert to false. */
template <class BaseIter>
using skip_false_iterator = boost::filter_iterator<
  to_bool<typename std::iterator_traits<BaseIter>::value_type>,
  BaseIter>;

template <class T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
  out << '[';
  for (auto i = v.cbegin(); i != v.cend(); ++i) {
    out << *i << ", ";
  }
  out << "\b\b]";
  return out;
}

}}

#endif
