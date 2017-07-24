#ifndef UEP_LT_PARAM_SET_HPP
#define UEP_LT_PARAM_SET_HPP

namespace uep {

/** Parameter set used to construct the LT encoders and decoders.
 *  Assumes a soliton_distribution as the underlying degree
 *  distribution.
 */
struct soliton_parameter_set {
  std::size_t K;
};

/** Parameter set used to construct the LT encoders and decoders.
 *  Assumes a robust_soliton_distribution as the underlying degree
 *  distribution.
 */
struct robust_lt_parameter_set {
  std::size_t K;
  double c;
  double delta;
};

}

#endif
