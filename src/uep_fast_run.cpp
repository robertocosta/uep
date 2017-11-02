#include "uep_fast_run.hpp"

#include "uep_encoder.hpp"
#include "uep_decoder.hpp"

using namespace uep;

/* Produce random packets of fixed size following the given subblock
 * configuration.
 */
class random_packet_source {
public:
  template<typename KsIter>
  explicit random_packet_source(KsIter ks_begin, KsIter ks_end,
				std::size_t packet_size,
				std::size_t block_count) :
    _ks(ks_begin, ks_end),
    _pktsize{packet_size},
    _block_count{block_count},
    _current_block{0},
    _current_subblock{0},
    _subblock_count{1},
    _rng{static_cast<unsigned char>(std::random_device{}())} {
      generate_next_p(0);
  }

  fountain_packet next_packet() {
    if (!has_packet()) throw std::runtime_error("No more packets");

    fountain_packet p = std::move(_next_p);

    ++_subblock_count;

    if (_subblock_count > _ks[_current_subblock]) {
      _subblock_count = 1;
      ++_current_subblock;
    }

    if (_current_subblock == _ks.size()) {
      _current_subblock = 0;
      ++_current_block;
    }

    generate_next_p(_current_subblock);

    return p;
  }

  bool has_packet() const {
    return _current_block < _block_count;
  }

private:
  std::vector<std::size_t> _ks;
  std::size_t _pktsize;
  std::size_t _block_count;
  std::size_t _current_block;
  std::size_t _current_subblock;
  std::size_t _subblock_count;

  std::independent_bits_engine<std::mt19937, 8, unsigned char> _rng;
  fountain_packet _next_p;

  void generate_next_p(f_uint priority) {
    _next_p = fountain_packet{};
    auto &buf = _next_p.buffer();
    buf.resize(_pktsize);
    for (char &c : buf) {
      c = _rng();
    }
    _next_p.setPriority(priority);
  }
};

/** Count the non-empty received packets. */
class nonempty_counter_sink {
public:
  template<typename KsIter>
  explicit nonempty_counter_sink(KsIter ks_begin, KsIter ks_end, std::size_t expected_blocks) :
    _ks(ks_begin, ks_end),
    _received(_ks.size(), 0),
    _exp_blocks{expected_blocks} {
  }

  void push(const fountain_packet &p) {
    _avg_per.clear();
    if (!p.buffer().empty()) {
      ++(_received.at(p.getPriority()));
    }
  }

  const std::vector<std::size_t> &received_counts() const {
    return _received;
  }

  std::size_t received_count(std::size_t priority) const {
    return _received.at(priority);
  }

  std::size_t total_received_count() const {
    return std::accumulate(_received.cbegin(), _received.cend(), 0);
  }

  const std::vector<double> &avg_error_rates() const {
    if (_avg_per.empty()) {
      for (std::size_t i = 0; i < _received.size(); ++i) {
	double tot = _ks[i] * _exp_blocks;
	double errs = tot - _received[i];
	_avg_per.push_back(errs / tot);
      }
    }
    return _avg_per;
  }

  double avg_error_rate(std::size_t priority) const {
    return avg_error_rates().at(priority);
  }

private:
  std::vector<std::size_t> _ks;
  std::vector<std::size_t> _received;
  std::size_t _exp_blocks;
  mutable std::vector<double> _avg_per;
};



simulation_results run_uep(const simulation_params &params) {
  random_packet_source src(params.Ks.begin(), params.Ks.end(),
			   params.L, params.nblocks);
  uep_encoder<> enc(params.Ks.begin(), params.Ks.end(),
		    params.RFs.begin(), params.RFs.end(),
		    params.EF,
		    params.c,
		    params.delta);
  markov2_distribution chan(params.chan_pGB, params.chan_pBG);
  uep_decoder dec(params.Ks.begin(), params.Ks.end(),
		  params.RFs.begin(), params.RFs.end(),
		  params.EF,
		  params.c,
		  params.delta);
  nonempty_counter_sink sink(params.Ks.begin(), params.Ks.end(),
			     params.nblocks);
  simulation_results results;

  results.dropped_count = 0;

  std::size_t n = (params.overhead + 1) * enc.K();
  std::list<fountain_packet> coded_block;
  for (;;) {
    // Load the encoder
    while (src.has_packet() && !enc.has_block()) {
      enc.push(src.next_packet());
    }

    if (!enc.has_block()) break;

    // Encode n packets
    while (coded_block.size() < n) {
      coded_block.push_back(enc.next_coded());
    }
    enc.next_block();

    // Drop some packets
    for (auto i = coded_block.begin(); i != coded_block.end();) {
      if (chan() == 1) { // Drop
	i = coded_block.erase(i);
	++results.dropped_count;
      }
      else {
	++i;
      }
    }

    // Decode
    dec.push(std::make_move_iterator(coded_block.begin()),
	     std::make_move_iterator(coded_block.end()));
    coded_block.clear();

    // Count decoded
    while (dec.has_queued_packets()) {
      sink.push(dec.next_decoded());
    }
  }

  // Finish last block
  if (!dec.has_decoded()) {
    dec.flush();
    while (dec.has_queued_packets()) {
      sink.push(dec.next_decoded());
    }
  }

  results.avg_pers = sink.avg_error_rates();
  results.rec_counts = sink.received_counts();
  results.err_counts.resize(params.Ks.size());
  for (std::size_t i = 0; i < results.err_counts.size(); ++i) {
    results.err_counts[i] = params.nblocks * params.Ks[i] - results.rec_counts[i];
  }
  results.avg_enc_time = enc.average_encoding_time();
  return results;
}
