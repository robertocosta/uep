#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <iostream>

template <class T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
  out << '[';
  for (auto i = v.cbegin(); i != v.cend(); ++i) {
    out << *i << ", ";
  }
  out << "\b\b]";
  return out;
}

#endif
