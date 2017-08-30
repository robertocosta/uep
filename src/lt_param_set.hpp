#ifndef UEP_LT_PARAM_SET_HPP
#define UEP_LT_PARAM_SET_HPP

#include <limits>

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
  //std::string streamName;
};

struct streamTrace {
	unsigned int startPos;
	int len;
	int lid;
	int tid;
	int qid;
	int packetType; // 1: StreamHeader, 2: ParameterSet, 3: SliceData
	bool discardable;
	bool truncatable;
};

/** Parameters used to setup the control and data connections between
 *  the client and server.
 */
struct net_parameter_set {
  /** The value to set sendRate to in order to allow an unlimited
   *  rate.
   */
  static constexpr double unlimited_send_rate =
    std::numeric_limits<double>::infinity();
  /** The value to set fileSize to in order to send an unlimited stream. */
  static constexpr std::size_t unlimited_file_size = 0;

  std::string streamName; /**< The name of the file to stream. */
  bool ack; /**< Whether to enable the ACKs. */
  double sendRate; /**< The send rate for UDP packets from the serrver. */
  std::size_t fileSize; /**< The size of the file being streamed. */
  std::string tcp_port_num; /**< The TCP port where the control server listens. */
  std::string udp_port_num; /**< The UDP port where the data client listens. */
  std::vector<streamTrace> videoTraceAr;
};

/** Complete set of parameters that is exchanged between the server
 *  and the client.
 */
template <class EncPar>
struct all_parameter_set: public net_parameter_set,
			  public EncPar {
};

}

#endif
