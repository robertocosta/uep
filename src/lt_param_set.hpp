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

/** Parameter set used to add redoundancy
	Repeat factors: one for each priority
	Expanding factor: applied to the whole sequence
	*/
struct lt_uep_parameter_set {
	uint8_t RFM;	/* repeat factor for the most important part */
	uint8_t RFL;  /* repeat factor for the less important part */

	uint8_t EF;	/* expanding factor */
  std::vector<std::size_t> Ks; /**< Array of sub-block sizes. */
  std::vector<std::size_t> RFs; /**< Array of repetition factors to
				 *   apply to each sub-block.
				 */

  double c; /**< Coefficient c of the robust soltion distribution. */
  double delta; /**< Failure prob bound of the robust soliton
		 *   distribution.
		 */
};

}

#endif
