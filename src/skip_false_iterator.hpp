#ifndef UEP_UTILS_SKIP_FALSE_ITERATOR_HPP
#define UEP_UTILS_SKIP_FALSE_ITERATOR_HPP

#include <type_traits>

#include <boost/iterator/filter_iterator.hpp>

namespace uep { namespace utils {

/** Predicate that calls the bool conversion operator. */
template<typename T>
struct to_bool {
  bool operator()(const T &x) const {
    return static_cast<bool>(x);
  }
};

/** Iterator adaptor that skips over the values that convert to false. */
template<typename BaseIter>
using skip_false_iterator =
  boost::filter_iterator<
  to_bool<typename std::iterator_traits<BaseIter>::value_type>,
  BaseIter>;

}}

#endif
