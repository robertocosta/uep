#ifndef UEP_LT_PARAM_SET_HPP
#define UEP_LT_PARAM_SET_HPP

namespace uep {

struct robust_lt_parameter_set {
  std::size_t K;
  double c;
  double delta;
};

}

#endif
